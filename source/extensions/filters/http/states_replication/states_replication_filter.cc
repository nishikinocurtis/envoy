//
// Created by qiutong on 7/11/23.
//

#include <string>
#include <chrono>

#include "source/common/storage/storage_impl.h"
#include "source/extensions/filters/http/states_replication/states_replication_filter.h"

#define NEW_IMPL


namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

#define METADATA_FLAG_MASK ((1u << 16) - 1)

#ifndef NEW_IMPL

Http::FilterHeadersStatus StatesReplicationFilter::decodeHeaders(Http::RequestHeaderMap &headers, bool end_stream) {
  time_counter_ = std::chrono::high_resolution_clock::now();
  auto resource_id_exist = headers.get(Http::LowerCaseString("x-ftmesh-resource-id")).size();

  if (!resource_id_exist || end_stream) {
    is_attached_ = false;
    return Http::FilterHeadersStatus::Continue;
  }

  std::string resource_id =
      std::string{headers.get(Http::LowerCaseString("x-ftmesh-resource-id"))[0]->value().getStringView()};

  auto content_length =
      std::stoull(std::string{headers.get(Http::LowerCaseString("content-length"))[0]->value().getStringView()}, nullptr);
  // ENVOY_LOG(debug, "content-length string: {}", headers.getByKey("content-length").value_or("0").data());

  ENVOY_LOG(debug, "HTTP Request captured by StatesReplicationFilter");


  ENVOY_LOG(debug, "Entering states replication");

  // populate other fields.
  Envoy::States::StorageMetadata metadata;
  metadata.resource_id_ = resource_id;
  // do bitwise and with uint16mask to avoid overflow.
  metadata.recover_port_ =
      std::stoul(
          std::string{headers.get(Http::LowerCaseString("x-ftmesh-recover-port"))[0]->value().getStringView()}, nullptr)
          & METADATA_FLAG_MASK;
  metadata.flags = std::stoul(
      headers.get(Http::LowerCaseString("x-ftmesh-flags"))[0]->value().getStringView().data(), nullptr)
          & METADATA_FLAG_MASK;
  metadata.ttl_ = std::stoul(
      headers.get(Http::LowerCaseString("x-ftmesh-ttl"))[0]->value().getStringView().data(), nullptr);

  metadata.recover_uri_ = headers.get(Http::LowerCaseString("x-ftmesh-recover-uri"))[0]->value().getStringView();
  metadata.svc_id_ = headers.get(Http::LowerCaseString("x-ftmesh-svc-id"))[0]->value().getStringView();
  metadata.pod_id_ = headers.get(Http::LowerCaseString("x-ftmesh-pod-id"))[0]->value().getStringView();
  metadata.method_name_ = headers.get(Http::LowerCaseString("x-ftmesh-method-name"))[0]->value().getStringView();
  metadata.svc_ip_ = headers.get(Http::LowerCaseString("x-ftmesh-svc-ip"))[0]->value().getStringView();
  metadata.svc_port_ = headers.get(Http::LowerCaseString("x-ftmesh-svc-port"))[0]->value().getStringView();

  states_position_ = std::stoull(
      std::string{headers.get(Http::LowerCaseString("x-ftmesh-states-position"))[0]->value().getStringView()}, nullptr);

  auto new_content_length = states_position_;
  ENVOY_LOG(debug, "mutating headers, old content length: {}, states_position: {}, new content length: {}",
            content_length, states_position_, new_content_length);
  headers.setContentLength(new_content_length);

  ENVOY_LOG(debug, "Metadata extracted, resource-id: {}, states_position: {}",
            metadata.resource_id_, states_position_);
  state_obj_ = std::make_unique<Envoy::States::RawBufferStateObject>(metadata);
  is_attached_ = true;

  // drop the resource-id header
  headers.remove(Http::LowerCaseString("x-ftmesh-resource-id"));
  return Http::FilterHeadersStatus::StopIteration;
}

// TODO: make it stateful and capable with longer streams: recoding truncate_length and do minus each time.
Http::FilterDataStatus StatesReplicationFilter::decodeData(Buffer::Instance &data, bool end_stream) {
  if (!is_attached_) {
    return PassThroughDecoderFilter::decodeData(data, end_stream);
  } else {
    ENVOY_LOG(debug, "start moving buffer");
    state_obj_->getObject().truncateOut(data, states_position_);
    auto resource_id = state_obj_->metadata().resource_id_;
    auto callback = decoder_callbacks_;
    auto storage_mgr = Envoy::States::StorageSingleton::getInstance();
    storage_mgr->write(std::move(state_obj_), callback->dispatcher());
    ENVOY_LOG(debug, "states buffer captured, continue on next filter, clearing stateful counters");
    storage_mgr->replicate(resource_id);
    is_attached_ = false;
    states_position_ = 0;
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - time_counter_);
    ENVOY_LOG(info, "internal microbenchmark time {}", static_cast<double>(duration.count() / 1000.0));
    return Http::FilterDataStatus::Continue;
  }
}

#else

Http::FilterHeadersStatus StatesReplicationFilter::decodeHeaders(Http::RequestHeaderMap &headers, bool end_stream) {
  // there will be two additional headers: x-ftmesh-mode, x-ftmesh-length
  // local-pos means there's only the state from the service, the buffer need to be flushed
  // sync-pos means all the state need to be replicated, the buffer need to be populated

  states_position_ =
      std::stoull(std::string{headers.get(
          Http::LowerCaseString("x-ftmesh-length"))[0]->value().getStringView()}, nullptr);
  state_mode_ = // 0 == local, 1 == sync
      std::stoul(std::string{headers.get(
          Http::LowerCaseString("x-ftmesh-mode"))[0]->value().getStringView()}, nullptr);

  auto state_length = std::stoul(std::string{headers.ContentLength()->value().getStringView()}, nullptr)
                      - states_position_;

  auto storage_mgr = Envoy::States::StorageSingleton::getInstance();

  // need to simulate the ring buf siz here, to determine the header
  // auto write_status = storage_mgr->validate !! race condition, better done at once
  // not so frequently, as usually only two target.
  // also, don't bother validating if the request is flowing to target: simply flush all.
  // all these need to be done in decoder header:
  // then no race, just save the scene (as a marker), and flush until that marker: no inconsistency.
  buf_status_ = 0;
  if (state_mode_ & 1) {
    // populate buffer and trigger force flush
    storage_mgr->validate_write_lsm(state_length); // return -1 here
    buf_status_ = -1;
    headers.setContentLength(states_position_);
  } else if (target_no_ = storage_mgr->validate_target(headers.Host()->value().getStringView()) /* target node in list */) {
    headers.setCopy(Http::LowerCaseString("x-ftmesh-mode"), "1");
    // simulate, populate buffer and trigger attached flush
    buf_status_ = storage_mgr->validate_write_lsm(state_length);
    // how to trigger: validate the host
    // if host match: content-length state_position + flush size
    headers.setContentLength(states_position_ + buf_status_);
  } else {
    // simulate, populate buffer and trigger force flush, consider merge with branch 1.
    // otherwise: force flush, separate request
    storage_mgr->validate_write_lsm(state_length); // return -1 here
    buf_status_ = -1;
    headers.setContentLength(states_position_);
  }

  is_attached_ = true;

  // always populate the buffer with body
  // but only flush when local and cluster match
}

Http::FilterDataStatus StatesReplicationFilter::decodeData(Buffer::Instance &data, bool end_stream) {
  if (!is_attached_) {
    return PassThroughDecoderFilter::decodeData(data, end_stream);
  } else {
    ENVOY_LOG(debug, "start moving buffer");
    state_obj_->getObject().truncateOut(data, states_position_);
    auto resource_id = state_obj_->metadata().resource_id_;
    auto callback = decoder_callbacks_;
    auto storage_mgr = Envoy::States::StorageSingleton::getInstance();
    // storage_mgr->write(std::move(state_obj_), callback->dispatcher());
    // auto write_status = storage_mgr->write_lsm(std::move(state_obj_), callback->dispatcher());
    if (~buf_status_) { // not -1: attach and replicate, state_mode must be 0
      ASSERT(state_mode_ == 0);
      // state_mode_ == 1 (sync) and not exceeding: populate and drop all
      // state_mode_ == 0 (local) and not exceeding: by host, if host match, attach the ring buf to the end
      // otherwise: populate and drop all
      auto to_be_synced = storage_mgr->write_lsm_attach(std::move(state_obj_), callback->dispatcher(), target_no_ - 1);
      data.move(*to_be_synced); // header already set before
    } else { // buf_status_ == -1, no need to attach
      // state_mode_ == 1 (sync) and exceeding: flushed, drop all here
      // state_mode_ == 0 (local) and exceeding: also by host: if host match, flush here
      // otherwise, drop all.
      storage_mgr->write_lsm_force(std::move(state_obj_), callback->dispatcher());

      // headers.setContentLength(states_position_);
    }
    // storage_mgr->replicate(resource_id);
    is_attached_ = false;
    states_position_ = 0;

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - time_counter_);
    ENVOY_LOG(info, "internal microbenchmark time {}", static_cast<double>(duration.count() / 1000.0));

    return Http::FilterDataStatus::Continue;
  }
}

#endif

Http::FilterTrailersStatus StatesReplicationFilter::decodeTrailers(Http::RequestTrailerMap &trailers) {
  return PassThroughDecoderFilter::decodeTrailers(trailers);
}

} // namespace States
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy