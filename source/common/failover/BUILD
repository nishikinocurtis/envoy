load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "failover_lib",
    hdrs = ["failover_manager_impl.h"],
    srcs = ["failover_manager_impl.cc"],
    deps = [
        "//envoy/failover:failover_interface",
    ],
)