//
// Created by qiutong on 7/11/23.
//

#include "envoy/storage/storage.h"

namespace Envoy {
namespace States {

class RecoverHandler {
  // read body from AdminStream, extract resource_id,
  // call Storage.recover()
};

class ReplicateHandler {
    // read header from AdminStream, create StateObject
    // read body, call StateObject.move
    // call Storage.write()
};

}
}
