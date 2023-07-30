load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "states_replication_filter_lib",
    srcs = ["states_replication_filter.cc"],
    hdrs = ["states_replication_filter.h"],
    deps = [
        "//envoy/http:codes_interface",
        "//envoy/http:filter_interface",
        "//envoy/storage:storage_interface",
        "//source/common/storage:storage_lib",
        "//source/extensions/filters/http/common:pass_through_filter_lib",
    ],
)

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//envoy/registry",
        "//envoy/server:filter_config_interface",
        "//source/common/router:config_lib",
        "//source/extensions/filters/http/common:factory_base_lib",
        ":states_replication_filter_lib",
        "@envoy_api//envoy/extensions/filters/http/states_replication/v3:pkg_cc_proto",
        # and some api definition.
    ],
)
