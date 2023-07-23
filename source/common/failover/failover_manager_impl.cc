//
// Created by qiutong on 7/23/23.
//

#include "failover_manager_impl.h"

#include "source/common/storage/storage_impl.h"

namespace Envoy {
namespace Failover {

FailoverManagerImpl::FailoverManagerImpl(Event::Dispatcher &dispatcher, const LocalInfo::LocalInfo &local_info,
                                         Upstream::ClusterManager &cm)
                                         : storage_manager_(States::StorageSingleton::getInstance()),
                                          cm_(cm) {
  // other members to be initialized
}


}
}