//
// Created by qiutong on 7/11/23.
//

#pragma once

#include "source/extensions/filters/http/common/pass_through_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

class StatesReplicationFilter : public Http::PassThroughDecoderFilter {
  // record content-length and replication-length at decodeHeader

  // need to truncate the body

  // moveOut the buffer and move into Storage

  // call Storage Replicate and continue

};

}
}
}
}
