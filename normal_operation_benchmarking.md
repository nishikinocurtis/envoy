# HTTP Normal Operation Overhead Micro-benchmarking Guide

## Environment Preparation

1. Set up a 2-machine LAN on cloudlab, without k8s for simplicity, namely machine 0 and 1.

2. Follow [building guide](bazel/README.md) to setup building environment, 
using clang-libc++ combination (stdlibc++ has some issues). Using docker sandbox or not is OK. 

3. Build envoy proxy (`bazel build envoy` from http-request-overhead branch).

4. Set up client on machine 0, set up server on machine 1.

5. Modify the `clusters` section in `conf.yaml`, let the endpoint IP be the IP of machine 1,
port be the socket port to which the server application is listening.

6. Make batched requests (e.g. 100/1k/10k times, with sleep or parallelization, respectively)
from client, with reference to the following format:

```bash
curl -X POST http://127.0.0.1:10729/fib?n=3 \
	 -H "x-ftmesh-resource-id: d9175a23-65b0-4e78-9802-1d29d0a019d6" \
	 -H "x-ftmesh-states-position: 0" \
	 -H "x-ftmesh-recover-port: 20231" \
	 -H "x-ftmesh-flags: 0" \
	 -H "x-ftmesh-ttl: 600" \
	 -H "x-ftmesh-recover-uri: /fib/recover" \
	 -H "x-ftmesh-svc-id: curl-test" \
	 -H "x-ftmesh-pod-id: curl-test.local.1" \
	 -H "x-ftmesh-method-name: delegateFib" \
	 -H "x-ftmesh-svc-ip: 127.0.0.1" \
	 -H "x-ftmesh-svc-port: 44031" \
	 -d "d9175a23-65b0-4e78-9802-1d29d0a019d6"
```

The request body can be anything, try different sizes for experiment. But make sure the useful
data are placed first, followed by things you want to replicate as states. And specify the location
where the states start (indexing from 0, every byte counts for 1 position). The data starting from that position
will be truncated by envoy and store in itself.

We measure the average time from client sending the request to the response is received,
and compare the time difference between whether the state is attached, and what size the states are.