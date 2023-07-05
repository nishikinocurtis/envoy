load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_cc_library(
    name = "fast_reconfig_lib",
    srcs = ["fast_reconfig.cc"],
    hdrs = ["fast_reconfig.h"],
    deps = [
        ":fast_reconfig_filter_lib",
    ]
)

envoy_cc_library(
    name = "fast_reconfig_filter_lib",
    srcs = ["fast_reconfig_filter.cc"],
    hdrs = ["fast_reconfig_filter.h"],
    deps = [
        "//source/server/admin:utils_lib",
        "//source/server:server_endpoint_listener_lib",
    ]
)