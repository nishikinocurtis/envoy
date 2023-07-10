//
// Created by qiutong on 7/10/23.
//

#include "envoy/storage/storage.h"

namespace Envoy {
namespace States {

class RawBufferStateObject : public StateObject {
};

class StorageImpl : public Storage {
public:

  void timedCleanUp() override;

private:
  std::unordered_map<std::string, std::unique_ptr<StateObject>> states_repo_;
};

} // namespace States
} // namespace Envoy
