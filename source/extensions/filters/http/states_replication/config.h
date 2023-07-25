//
// Created by qiutong on 7/11/23.
//

#pragma once

#include "source/extensions/filters/http/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

class StatesReplicationFilterFactory {
  // it seems we really don't need some specific config
  // just inherits from FactoryBase and register the filter
  // declare a simple .proto in api/envoy/extensions/...
};

}
}
}
}


