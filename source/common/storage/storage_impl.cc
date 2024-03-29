//
// Created by qiutong on 7/10/23.
//

#include <iostream>

#include "source/common/storage/storage_impl.h"

#include "envoy/http/codes.h"

#include "source/common/common/assert.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/http/message_impl.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/config/utility.h"
#include "source/common/protobuf/utility.h"

#define BENCHMARK_MODE
#define SINGLE_REPLICA

namespace Envoy {
namespace States {

RawBufferStateObject::RawBufferStateObject(StorageMetadata &metadata) {
  metadata_ = metadata;
  buf_ = std::make_unique<Buffer::OwnedImpl>();
}

RawBufferStateObject::RawBufferStateObject(Envoy::States::StorageMetadata &metadata, Buffer::Instance &data) :
  RawBufferStateObject(metadata) {
  buf_->move(data);
}

void RawBufferStateObject::writeObject(Buffer::Instance &obj) {
  buf_->move(obj);
}

// no need for RawBufferStateObject to find if the rhs is typed or not
// always rely on the getObject interface to get a buffer.
void RawBufferStateObject::move(Envoy::States::StateObject &rhs) {
//  if (rhs.metadata().flags & TYPED) {
//    // can only move raw buffer.
//    // ENVOY_LOG(debug, "losing typed information")
//    buf_->move(rhs.getObject());
//  } else {
//    buf_->move(rhs.getObject());
//  }
  // refresh cancel timer
  metadata_ = rhs.metadata();
  buf_->move(rhs.getObject());
}

Buffer::Instance &RawBufferStateObject::getObject() {
  return *buf_;
}

RpdsApiImpl::RpdsApiImpl(const xds::core::v3::ResourceLocator* rpds_resource_locator,
                         const envoy::config::core::v3::ConfigSource& rpds_config,
                         std::shared_ptr<Storage> &&str_manager,
                         Upstream::ClusterManager &cm,
                         Init::Manager &init_manager, Stats::Scope &scope,
                         ProtobufMessage::ValidationVisitor& validation_visitor)
                         : Envoy::Config::SubscriptionBase<Replicator>(validation_visitor, "name"),
                           str_manager_(std::move(str_manager)), scope_(scope.createScope("storage.rpds.")),
                           cm_(cm), init_target_("RPDS", [this]() { subscription_->start({}); }) {
  // create subscription, must give both resource_locator and rpds_config.
  const auto resource_name = getResourceName();
  subscription_ = cm_.subscriptionFactory().collectionSubscriptionFromUrl(
      *rpds_resource_locator, rpds_config, resource_name, *scope_, *this, resource_decoder_);

  init_manager.add(init_target_);
}

void RpdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &resources,
                                 const std::string &version_info) {
  version_info_ = version_info;
  // call the str_manager_->shiftTargetClusters
  auto replicator_config =
      dynamic_cast<const Replicator&>(resources[0].get());
  auto new_target_list = std::make_unique<std::list<std::string>>();
  auto new_priority_ups = std::make_unique<std::set<std::string>>();
  auto new_host_list = std::make_unique<std::list<std::string>>();

  str_manager_->beginTargetUpdate();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
  // TODO: Add upstream update logic, fix sync target logic.
  for (const auto& target_to_add : replicator_config.target_cluster_names()) {
    new_target_list->push_back(target_to_add);
  }
  for (const auto& ups : replicator_config.priority_upstream_names()) {
    new_priority_ups->insert(ups);
  }
  for (const auto& target_host : replicator_config.target_host_names()) {
    new_host_list->push_back(target_host);
  }
#pragma clang diagnostic pop
  str_manager_->shiftTargetClusters(std::move(new_target_list), std::move(new_priority_ups), std::move(new_host_list));

  str_manager_->endTargetUpdate();
}

void RpdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &added_resources,
                                 const Protobuf::RepeatedPtrField<std::string>& removed_resources,
                                 const std::string &system_version_info) {
  version_info_ = system_version_info;

  //TODO: TBI
  str_manager_->beginTargetUpdate();
  // call the str_manager_->removeTargetCluster
  // std::vector<std::string> removed_targets;
  for (const auto& target_to_remove : removed_resources) {
    // removed_targets.push_back(target_to_remove);
    str_manager_->removeTargetCluster(target_to_remove);
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
  // std::vector<std::string> added_targets;
  for (const auto& target_to_add : added_resources) {
    std::string target;
    // cast from Message
    // added_targets.push_back(target);
    str_manager_->addTargetCluster(target);
  }
#pragma clang diagnostic pop

  // call the str_manager_->addTargetCluster, remember to drain current traffics.
  str_manager_->endTargetUpdate();
}

void RpdsApiImpl::onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                                       const Envoy::EnvoyException*) {
  // logic adopted from LdsApiImpl.
  ASSERT(Envoy::Config::ConfigUpdateFailureReason::ConnectionFailure != reason);
  ENVOY_LOG(debug, "RpdsAPI config update failed");
  init_target_.ready();
}

StorageImpl::StorageImpl(Event::Dispatcher &dispatcher, Server::Instance& server,
                         const xds::core::v3::ResourceLocator*,
                         const envoy::config::storage::v3::Storage&, const LocalInfo::LocalInfo &local_info,
                         Upstream::ClusterManager &cm,
                         std::unique_ptr<std::list<std::string>>&& target_cluster,
                         std::unique_ptr<std::list<std::string>>&& target_host,
                         std::unique_ptr<std::list<std::string>>&& static_upstream,
                         uint32_t lsm_ring_buf_siz
                         )
                         : dispatcher_(dispatcher), server_(server),
#ifdef RPDS
                         rpds_api_(std::make_shared<RpdsApiImpl>(rpds_resource_locator, storage_config.rpds_config(), shared_from_this(), cm, server_.initManager(),
                                    *server_.stats().rootScope(), server_.messageValidationContext().dynamicValidationVisitor())),
#endif
                         target_clusters_(std::move(target_cluster)),
                         target_hosts_(std::move(target_host)),
                         priority_upstreams_(nullptr),
                         static_upstreams_(std::move(static_upstream)),
                         ring_buf_(std::make_unique<Buffer::OwnedImpl>()), max_buf_(lsm_ring_buf_siz), latest_(0),
                         watermark_(0.75 * max_buf_), wm_proportion_(watermark_),
                         local_info_(local_info), cm_(cm), recover_info_callback_(std::make_unique<RecoverInfoCallback>(*this)) {
  // [SOLVED] need rpds_api_ initialization parameters: get init_manager, scope, and validation visitor from here.
  ring_buf_->reserveSingleSlice(lsm_ring_buf_siz, false);
  memset(progress_, 0, sizeof progress_);

#ifdef BENCHMARK_MODE
  ENVOY_LOG(debug, "Storage Impl launched without rpds_api_");
  // target_clusters_ = std::make_unique<std::list<std::string>>();
  // target_clusters_->emplace_back("cluster_0");
  // target_clusters_->emplace_back("rp_cluster_1");
#endif
}

int32_t StorageImpl::validate_target(const std::string_view& host) const {
  int iter = 0;
  // printf("entering target validation: %s\n", std::string{host}.);
  std::cout << "entering target validation: " << host << std::endl;
  for (const auto& h : *target_clusters_) { // change with the implementation in StateReplicationFilter
    iter++;
    ENVOY_LOG(debug, "comparing target {} and {}", h, host);
    if (!host.compare(h)) {
      return iter;
    }
  }
  return 0;
}

void StorageImpl::write(std::shared_ptr<StateObject>&& obj, Event::Dispatcher& tls_dispatcher) {
  StorageMetadata metadata = obj->metadata();

  // decide update or create
  // no matter which path, remember to refresh the removal timer.
  if (auto existing = states_.find(metadata.resource_id_); existing != states_.end()) {
    // update instead of insert
    // cancel old timer
    // do some remove event,
    // move new object
    // auto timer_old = ttl_timers_.find(metadata.resource_id_);
    // std::chrono::milliseconds ttl(metadata.ttl_ * 1000);
    // timer_old->second->disableTimer();
    // timer_old->second->enableTimer(ttl);
    auto& buf = existing->second->getObject();
    buf.drain(buf.length());
    existing->second->move(*obj);
    ttl_counter_[metadata.resource_id_] += 1;
  } else {
    auto inserted_pair = states_.insert(std::make_pair(metadata.resource_id_, std::move(obj)));
    // register the iterator to different categories.
    by_pod_.insert(std::make_pair(metadata.pod_id_, inserted_pair.first->second));
    ttl_counter_.insert(std::make_pair(metadata.resource_id_, 1u));
  }
  // register new timer.
  // timer = add_event(cb, timeout)
  // cb written here, [this, metadata]() -> { this->states_.remove(metadata.resource_id); }
  auto timer_ptr = tls_dispatcher.createTimer([this, metadata]() {
    this->ttl_counter_[metadata.resource_id_] -= 1;
    if (ttl_counter_[metadata.resource_id_] == 0) {
      this->states_.erase(metadata.resource_id_);
      ttl_counter_.erase(metadata.resource_id_);
    }
  });
  std::chrono::milliseconds ttl(metadata.ttl_ * 1000);
  timer_ptr->enableTimer(ttl);

  // register to a unordered_map.
  ttl_timers_[metadata.resource_id_] = std::move(timer_ptr);
}

int32_t StorageImpl::validate_write_lsm(uint64_t siz, int32_t target) const {
  if (target == -1) {
    return -1;
  } else {
    return (latest_ + siz - progress_[target] + max_buf_) % max_buf_;
  }
}

std::unique_ptr<Buffer::Instance> StorageImpl::write_lsm_attach(Buffer::Instance& obj,
                                                                Event::Dispatcher &, int32_t target) {
  // two things need to be maintained:
  // each target's progress
  // total length
  // auto& buf_obj = obj->getObject();
#ifdef TIME_EVAL
  std::cout << "latest_ before mod: " << latest_ << " obj_len: " << obj.length() << " wm_: " << watermark_ << std::endl;
#endif
  if ((latest_ <= watermark_ && latest_ + obj.length() >= watermark_) ||
    (latest_ > watermark_ && (latest_ + obj.length()) % max_buf_ >= watermark_)) { // TODO: modify the condition
    // flush all
    ring_buf_->move(obj);
    latest_ = (latest_ + obj.length()) % max_buf_;
    watermark_ = (watermark_ + wm_proportion_) % max_buf_;
    progress_[0] = progress_[1] = latest_;

    replicate(target ? "all-buffer/0" : "all-buffer/1");
    return nullptr; // this branch should return 0: no attach
  } else {
    latest_ = (latest_ + obj.length()) % max_buf_;
    ring_buf_->move(obj);

    auto returned_buf = std::make_unique<Buffer::OwnedImpl>();
#ifdef SINGLE_REPLICA
    watermark_ = (latest_ + wm_proportion_) % max_buf_;
    auto copy_siz = (latest_ + max_buf_ - progress_[target]) % max_buf_;
#ifdef TIME_EVAL
    std::cout << "normal attach: " << copy_siz << std::endl;
#endif
    ring_buf_->copyOutToBuffer(0, copy_siz, *returned_buf.get());
    ring_buf_->drain(copy_siz);
#else
    // here also modify the condition
    watermark_ = (progress_[target ^ 1] + wm_proportion_) % max_buf_;
    ring_buf_->copyOutToBuffer(progress_[target], (latest_ - progress_[target] + max_buf_) % max_buf_, *returned_buf.get());
#endif

    // auto dest_slice = returned_buf->reserveSingleSlice(latest_ - progress_[target], true);
    // auto copy_temp = new char[latest_ - progress_[target]]; // this impl needs to be refined as repeatedly allocating new slices in the heap.
    // which is also copied out later.
    // modify Buffer so that it can be used as a copy destination.
     // copy out to returned_buf
    // returned_buf->add(copy_temp, latest_ - progress_[target]);
    // ring_buf_->move(buf_obj);
    progress_[target] = latest_;


    // delete copy_temp[];

    return std::move(returned_buf); // this branch should return returned_buf.length().
  }
}

void StorageImpl::write_parse(Buffer::Instance& obj, Event::Dispatcher& tls_dispatcher) {
  std::cout << "siz len: " << sizeof(uint32_t) << " buf len: " << obj.length();
  auto metadata_len_mark = new char[sizeof(uint32_t)];
  obj.copyOut(0, sizeof(uint32_t), metadata_len_mark);

  StorageMetadata mt;
  uint32_t mt_len;
  memcpy(&mt_len, metadata_len_mark, sizeof(uint32_t));

  std::cout << " metadata len: " << mt_len  << std::endl;

  auto metadata = new char[mt_len];
  obj.copyOut(sizeof(uint32_t), mt_len, metadata);
  std::string metadata_str(metadata);

  obj.drain(sizeof(uint32_t) + mt_len);

  std::istringstream iss(metadata_str);
  std::string line;
  std::vector<std::string> result;

  while (std::getline(iss, line, '\n')) {
    result.push_back(line);
  }

  parseMetadata(mt, result);
  std::cout << mt.resource_id_ << std::endl;
  std::cout << mt.recover_port_ << std::endl;
  std::cout << mt.recover_uri_ << std::endl;
  auto state_obj = std::make_shared<RawBufferStateObject>(mt);
  state_obj->writeObject(obj);

  write(std::move(state_obj), tls_dispatcher);

  delete[] metadata_len_mark;
}

void StorageImpl::parseMetadata(Envoy::States::StorageMetadata &mt, const std::vector<std::string> &vec) {
  mt.recover_port_ = std::stoul(vec[0]);
  mt.flags = std::stoul(vec[1]);
  mt.ttl_ = std::stoul(vec[2]);
  mt.recover_uri_ = vec[3];
  mt.resource_id_ = vec[4];
  mt.svc_id_ = vec[5];
  mt.pod_id_ = vec[6];
  mt.method_name_ = vec[7];
  mt.svc_ip_ = vec[8];
  mt.svc_port_ = vec[9];
}

int32_t StorageImpl::write_lsm_force(Buffer::Instance& obj, Event::Dispatcher &) {
  if ((latest_ <= watermark_ && latest_ + obj.length() >= watermark_) ||
      (latest_ > watermark_ && (latest_ + obj.length()) % max_buf_ >= watermark_)) {
    // flush
    ring_buf_->move(obj);
    latest_ = (latest_ + obj.length()) % max_buf_;
    watermark_ = (watermark_ + wm_proportion_) % max_buf_;
    progress_[0] = progress_[1] = latest_;

    replicate("all-buffer");
    return 1;
  } else {
    // insert only
    ring_buf_->move(obj);
    latest_ = (latest_ + obj.length()) % max_buf_;
    return 0;
  }
}

void StorageImpl::replicate(const std::string &resource_id) {
  // TODO: modify logic, so that the replication happens in the HTTP filter
  // TODO: add a latest log pool to be replicated.
  // TODO: new msg should not use headers as metadata field, as there will be multiple logs
  // TODO: leave the parsing work to StorageImpl.

  if (resource_id.find("all-buffer") != std::string::npos) {
    Http::AsyncClient::RequestOptions options;

    auto excluded = -1;

    if (resource_id.length() != 8) {
      excluded = resource_id.back() == '0' ? 0 : 1;
    }

    int iter = 0;
    uint64_t drain_siz = 0;

    for (const auto& target : *target_clusters_) {
      if (iter == excluded) { iter++; continue; }

      auto headers = Http::RequestHeaderMapImpl::create();
      headers->setPath("/replicate_single");
      auto flush_buf = std::make_unique<Buffer::OwnedImpl>();
      ring_buf_->copyOutToBuffer(progress_[iter], latest_ - progress_[iter], *flush_buf);
      makeHttpCall(target, std::move(headers),
                   *flush_buf, options, *this);

      drain_siz = std::max(drain_siz, uint64_t(latest_ - progress_[iter])); // need to deal with ring_buf_
      progress_[iter] = latest_;

      iter++;
    }

    watermark_ = (latest_ + wm_proportion_) % max_buf_;
    ring_buf_->drain(drain_siz);

    return;
  }

  // makeHttpCall
  auto it = states_.find(resource_id);

  Http::AsyncClient::RequestOptions options;
  // build options

  if (it != states_.end()) {
    for (const auto& target : *target_clusters_) { // always async
      auto headers = Http::RequestHeaderMapImpl::create();
      headers->setPath("/replicate_single");
      headers->setCopy(Http::LowerCaseString("x-ftmesh-resource-id"), it->second->metadata().resource_id_);
      // populate, filling in metadata,
      // filling in host and path from configuration
      makeHttpCall(target, std::move(headers),
                   it->second->getObject(), options, *this);
    }
  } else {
    // report not found
    ENVOY_LOG(debug, "resource x not found for replication");
  }

}

// this should not be async, may wait for socket info
void StorageImpl::recover(const std::string& resource_id) {
  // makeHttpCall, combine with recover port and path
  // depending on if the port+path is a handshake port
  auto it = states_.find(resource_id);

  if (it != states_.end()) {
    auto metadata = it->second->metadata();
    Http::AsyncClient::RequestOptions options;

    if (metadata.flags & HANDSHAKE) {
      // handshake logic TBI
      ENVOY_LOG(debug, "non-final port recovery not implemented");
      return;
    } else {
      // makeHttpCall, transfer the resource.
      auto headers = Http::RequestHeaderMapImpl::create();
      // populate, fill in host and path from metadata
      headers->setHost("127.0.0.1:" + std::to_string(metadata.recover_port_));
      headers->setCopy(Http::LowerCaseString("x-ftmesh-resource-id"), metadata.resource_id_);
      headers->setPath(metadata.recover_uri_);
      // how to be aware of which cluster I am sending to?
      // actually I'm just sending to localhost...
      auto deliver_target = local_info_.clusterName();
#ifdef BENCHMARK_MODE
      deliver_target = "cluster_1"; // for benchmarking use only.
#endif
      std::cout << "requesting " << deliver_target << " at " << metadata.recover_port_ << metadata.recover_uri_ << std::endl;
      makeHttpCall(deliver_target, std::move(headers), it->second->getObject(), options, *recover_info_callback_.get());
      // TODO: wait for response: port to new port mapping, return the response body directly to caller
      return;
    }
  } else {
    ENVOY_LOG(debug, "resource {} not found for recovery", resource_id);
    std::cout << "resource: " << resource_id << "not found" << std::endl;
  }
}

void StorageImpl::bench_recover(const std::string &resource_id, const std::string &bench_marker,
                                std::chrono::time_point<std::chrono::high_resolution_clock> clock) {
  clock_keeper_[bench_marker] = clock;
  auto it = states_.find(resource_id);

  if (it != states_.end()) {
    auto metadata = it->second->metadata();
    Http::AsyncClient::RequestOptions options;

    if (metadata.flags & HANDSHAKE) {
      // handshake logic TBI
      ENVOY_LOG(debug, "non-final port recovery not implemented");
    } else {
      // makeHttpCall, transfer the resource.
      auto headers = Http::RequestHeaderMapImpl::create();
      // populate, fill in host and path from metadata
      headers->setHost("127.0.0.1:" + std::to_string(metadata.recover_port_));
      headers->setCopy(Http::LowerCaseString("x-ftmesh-bench-marker"), bench_marker);
      //headers->setCopy(Http::LowerCaseString("x-ftmesh-resource-id"), metadata.resource_id_);
      headers->setPath(metadata.recover_uri_);
      // how to be aware of which cluster I am sending to?
      // actually I'm just sending to localhost...
      auto deliver_target = local_info_.clusterName();
#ifdef BENCHMARK_MODE
      deliver_target = "cluster_1"; // for benchmarking use only.
#endif
      makeHttpCall(deliver_target, std::move(headers), it->second->getObject(), options, *this);
      return;
    }
  } else {
    ENVOY_LOG(debug, "resource {} not found for recovery", resource_id);
  }
}

void StorageImpl::deactivate(const std::string &resource_id) {
  // first cancel the timer
  auto timer_old = ttl_timers_.find(resource_id);
  if (timer_old != ttl_timers_.end()) {
    timer_old->second->disableTimer();
    states_.erase(resource_id);
  } else {
    // report not found
    ENVOY_LOG(debug, "target resource x to be deactivated not found.");
  }
}

Http::AsyncClient::Request* StorageImpl::makeHttpCall(
    const std::string& target, std::unique_ptr<Http::RequestHeaderMap>&& headers,
    Buffer::Instance& data, const Http::AsyncClient::RequestOptions& options,
    Http::AsyncClient::Callbacks& callbacks) {
  if (headers->Path() == nullptr) {
    // not responsible for filling replicate endpoint
    return nullptr;
  }
  if (headers->Host() == nullptr) {
    // fill in cluster
    headers->setHost(target);
  }
  if (headers->Method() == nullptr) {
    // fill in POST
    headers->setMethod("POST");
  }

  const auto thread_local_cluster = cm_.getThreadLocalCluster(target);
  if (thread_local_cluster == nullptr) {
    ENVOY_LOG(debug, "Cannot find replication target cluster x");
    return nullptr;
  }

  Http::RequestMessagePtr message(new Http::RequestMessageImpl(std::move(headers)));
  auto body_length = data.length();
  if (body_length != 0) {
    message->body().add(data);
    message->headers().setContentLength(body_length);
  }

  return thread_local_cluster->httpAsyncClient().send(std::move(message), callbacks, options);
}

std::shared_ptr<Storage> StorageSingleton::ptr_ = nullptr;

// TODO: construct with local address
void StorageImpl::RecoverInfoCallback::onSuccess(const Http::AsyncClient::Request &,
                                                 Http::ResponseMessagePtr &&response) {
  // assume response is text/html <ip/domain>:port.
  // auto new_svc_address = response->bodyAsString();
  // auto sep_pos = new_svc_address.find_last_of(":");
#ifdef TIME_EVAL
  auto enter_now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  printf("state recovery replied: %lld\n", enter_now);
#endif

  // if (sep_pos != std::string::npos) {
    auto resource_id = response->headers().get(Http::LowerCaseString("x-ftmesh-resource-id"));
    // auto location = new_svc_address.substr(0, sep_pos);
    // auto port = std::stoi(new_svc_address.substr(sep_pos+1));
    auto origin_obj = parent_.states_.find(std::string{resource_id[0]->value().getStringView()});
    if (origin_obj != parent_.states_.end()) {
      auto& obj_val = origin_obj->second->metadata();
      std::stringstream msg_ss("");
      msg_ss << obj_val.svc_id_ << "\n"
             << obj_val.svc_ip_ << "\n"
             << obj_val.svc_port_ << "\n"
             << "10.214.96.108" << "\n" // replace with local address here
             << 10729 << "\n";
      auto msg = msg_ss.str();

      std::cout << msg;

      Http::AsyncClient::RequestOptions options;

      // send msg to others;
      std::for_each(parent_.static_upstreams_->begin(), parent_.static_upstreams_->end(),
                    [&, this](const std::string& cluster) -> void {
        auto headers = Http::RequestHeaderMapImpl::create();
        headers->setHost("127.0.0.1:9903");
        headers->setPath("/rr_endpoint");
        auto data = Buffer::OwnedImpl(msg);

        std::cout << "requesting: " << cluster << std::endl;
        this->parent_.makeHttpCall(cluster, std::move(headers), data, options, parent_);
      });
    }
  // }

}

}
}
