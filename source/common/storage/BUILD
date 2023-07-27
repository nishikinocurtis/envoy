load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "storage_lib",
    srcs = ["storage_impl.cc"],
    hdrs = ["storage_impl.h"],
    deps = [
        "//envoy/event:timer_interface",
        "//envoy/http:async_client_interface",
        "//envoy/server:instance_interface",
        "//envoy/storage:storage_interface",
        "//envoy/upstream:cluster_manager_interface",
        "//source/common/buffer:buffer_lib",
        "//source/common/common:logger_lib",
        "//source/common/config:subscription_base_interface",
        "//source/common/config:utility_lib",
        "//source/common/config:xds_resource_lib",
        "//source/common/http:message_lib",
        "//source/common/init:target_lib",
        "//source/common/protobuf",
        "//source/common/protobuf:utility_lib",
    ],
)
