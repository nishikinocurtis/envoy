# FTMesh Design and Implementation Documentation

## Backgrounds


## Motivations


## Envoy: Architecture Overview

### Listeners and Filters


### xDS Subscription


### 


## FTMesh: Additional Architecture Overview


## Design Choices


## Implementation Components

### `FastReconfigServer`


### `Storage`


### `FailoverManager`


### `StateReplicationFilter`


## To-do List

### Storage Module

- [x] `StorageImpl::makeHttpCall` (reference to `lua_filter.cc`)

- [x] `StorageImpl::add_shift_removeTargetClusters` modifying replication target (depend on target_clusters_)

- [ ] `StorageImpl::begin_endTargetImpl` considering monitoring ongoing replication requests

- [x] `StorageImpl::replicate` and overwriting functions (depend on makeHttpCall and target_clusters_)
(considering coroutine/multi-thread optimization)

- [x] `StorageImpl::recover` and overwriting functions (investigate how to make localhost call with makeHttpCall)
(maybe specify a special static cluster and endpoint)

- [x] `StorageImpl::write` (register States to the map and register a timer to cleanup)

- [ ] `StorageImpl::deactivate` (manually clean up)

- [ ] Rpds API .proto file composition and compiling

### Buffer Interface and OwnedImpl

- [x] Add capability of moving out from certain position, reduce copy as much as possible.

### StateReplicationFilter 

- [x] `decodeHeader` generate StateMetadata and put it into private member, 
record content-length and x-replicate-length. depend on configuration, drop the x-replicate headers or not.


- [ ] `decodeData` extract States by BufferMethod: moveOutFrom, which then mutates data

- [ ] `decodeTrailer` maybe do nothing special

- [ ] maintain internal members.

### Failover Manager

- [ ] maintain internal members (cluster manager)

- [ ] migrate makeHttpCall to issue recovery signal

- [ ] investigate how envoy currently do health check and integrate Failover to it.

### ~~ReplicateRecoverHandler~~

- [x] `onFailureRecovery`

- [x] `onStatesReplication`

- [x] register handlers to the FastReconfigServerImpl

### Istio

- [ ] Rpds API Server, centralize failure recovey decision





