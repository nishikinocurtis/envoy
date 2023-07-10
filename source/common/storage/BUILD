load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "storage_lib",
    hdrs = ["storage_impl.h"],
    srcs = ["storage_impl.cc"],
    deps = [
        "//envoy/storage:storage_interface",
    ]
)