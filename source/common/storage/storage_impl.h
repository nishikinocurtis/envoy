//
// Created by qiutong on 7/10/23.
//

#include "envoy/storage/storage.h"

namespace Envoy {
namespace States {

class RawBufferStateObject : public StateObject {
public:
  RawBufferStateObject(StorageMetadata& metadata);
  RawBufferStateObject(StorageMetadata& metadata, Buffer::Instance& data);

  void writeObject(Buffer::Instance& obj) override;
private:
  std::unique_ptr<Buffer::Instance> buf_;
};

class StorageImpl : public Storage {
public:

  void write(std::unique_ptr<StateObject> obj) override;



private:
  std::unordered_map<std::string, std::unique_ptr<StateObject>> states_;
  //
};

} // namespace States
} // namespace Envoy
