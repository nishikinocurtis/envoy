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

### ~~Storage Module~~

- [x] `StorageImpl::makeHttpCall` (reference to `lua_filter.cc`)

- [x] `StorageImpl::add_shift_removeTargetClusters` modifying replication target (depend on target_clusters_)

- [ ] `StorageImpl::begin_endTargetImpl` considering monitoring ongoing replication requests

- [x] `StorageImpl::replicate` and overwriting functions (depend on makeHttpCall and target_clusters_)
(considering coroutine/multi-thread optimization)

- [x] `StorageImpl::recover` and overwriting functions (investigate how to make localhost call with makeHttpCall)
(maybe specify a special static cluster and endpoint)

- [x] `StorageImpl::write` (register States to the map and register a timer to cleanup)

- [x] `StorageImpl::deactivate` (manually clean up)

- [x] Rpds API .proto file composition and compiling

### ~~Buffer Interface and OwnedImpl~~

- [x] Add capability of moving out from certain position, reduce copy as much as possible.

### ~~StateReplicationFilter~~ 

- [x] `decodeHeader` generate StateMetadata and put it into private member, 
record content-length and x-replicate-length. depend on configuration, drop the x-replicate headers or not.


- [x] `decodeData` extract States by BufferMethod: moveOutFrom, which then mutates data

- [x] `decodeTrailer` maybe do nothing special

- [x] maintain internal members.

### ~~Failover Manager~~

- [x] maintain internal members (cluster manager)

- [x] migrate makeHttpCall to issue recovery signal

- [x] investigate how envoy currently do health check and integrate Failover to it.

### ~~ReplicateRecoverHandler~~

- [x] `onFailureRecovery`

- [x] `onStatesReplication`

- [x] register handlers to the FastReconfigServerImpl

### Istio

- [ ] Rpds API Server, centralize failure recovery decision

## To-do List Stage 2 Debugging and Evaluation

### Feature fixing

- [x] Failover Manager internal logics
- [ ] debug logs + variable printing at all functions
- [ ] socket feedback mechanism and configuration format
- [ ] critical connection maintaining

### Micro benchmarking

- [ ] generate listener filter config and test out runtime re-routing
- [ ] generate http request workload with designated headers and contents, test time
- [ ] generate failure signal with fixed resource id and fixed target and test delivery time

prioritized implementations:

- rr_config handler

- states_replication_filter

- onFailure signal handler

less prioritized implementations before poster:

- failover manager(which connects to health checker)
- rpds and rcds(we don't do dynamic update currently)


