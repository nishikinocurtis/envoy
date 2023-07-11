load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//envoy/registry",
        "//envoy/storage:storage_interface",
        "//source/common/storage:storage_lib",
        "//source/extensions/filters/http/common:pass_through_filter_lib",
        # and some api definition.
    ]
)