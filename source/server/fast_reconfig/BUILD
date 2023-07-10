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
        "//source/server:server_endpoint_listener_lib",
        "//source/server:server_endpoint_filter_lib",
        "//source/server:null_overload_manager_lib",
        "//source/server:server_endpoint_socket_lib",
        "//source/common/network:listen_socket_lib",
        "//envoy/http:filter_interface",
        "//envoy/network:filter_interface",
        "//envoy/network:listen_socket_interface",
        "//envoy/server:instance_interface",
        "//envoy/server:listener_manager_interface",
        "//source/extensions/listener_managers/listener_manager:lds_api_lib",
    ]
)

envoy_cc_library(
    name = "fast_reconfig_filter_lib",
    srcs = ["fast_reconfig_filter.cc"],
    hdrs = ["fast_reconfig_filter.h"],
    deps = [
        "//envoy/server:instance_interface",
        "//envoy/server:listener_manager_interface",
        "//source/server:utils_lib",
        "//source/server:server_endpoint_filter_lib",
        "//source/common/http:codes_lib",
        "//source/common/http:header_map_lib",
        "//source/common/buffer:buffer_lib",
    ]
)

envoy_cc_library(
    name = "listener_reconfig_lib",
    srcs = ["listener_handler.cc"],
    hdrs = ["listener_handler.h"],
    deps = [
        "//source/common/protobuf:protobuf",
        "//source/extensions/listener_managers/listener_manager:lds_api_lib",
    ]
)