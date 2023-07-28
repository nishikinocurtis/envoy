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
    visibility = [
        "//source/server:__subpackages__",
    ],
    deps = [
        ":fast_reconfig_filter_lib",
        ":listener_reconfig_lib",
        ":recover_handler_lib",
        "//envoy/http:filter_interface",
        "//envoy/network:filter_interface",
        "//envoy/network:listen_socket_interface",
        "//envoy/server:fast_reconfig_interface",
        "//envoy/server:instance_interface",
        "//envoy/server:listener_manager_interface",
        "//envoy/storage:storage_interface",
        "//source/common/http:date_provider_lib",
        "//source/common/http:default_server_string_lib",
        "//source/common/http:header_map_lib",
        "//source/common/http:headers_lib",
        "//source/common/http:utility_lib",
        "//source/common/http/http1:codec_stats_lib",
        "//source/common/http/http2:codec_stats_lib",
        "//source/common/network:listen_socket_lib",
        "//source/common/stats:isolated_store_lib",
        "//source/extensions/listener_managers/listener_manager:lds_api_lib",
        "//source/extensions/request_id/uuid:config",
        "//source/server:null_overload_manager_lib",
        "//source/server:server_endpoint_filter_lib",
        "//source/server:server_endpoint_listener_lib",
        "//source/server:server_endpoint_socket_lib",
    ],
)

envoy_cc_library(
    name = "fast_reconfig_filter_lib",
    srcs = ["fast_reconfig_filter.cc"],
    hdrs = ["fast_reconfig_filter.h"],
    deps = [
        "//envoy/server:fast_reconfig_interface",
        "//envoy/server:instance_interface",
        "//envoy/server:listener_manager_interface",
        "//source/common/buffer:buffer_lib",
        "//source/common/http:codes_lib",
        "//source/common/http:header_map_lib",
        "//source/server:server_endpoint_filter_lib",
        "//source/server:utils_lib",
    ],
)

envoy_cc_library(
    name = "reconfig_handler_base_lib",
    hdrs = ["reconfig_handler_base.h"],
    deps = [
        "//envoy/config:subscription_interface",
        "//envoy/server:fast_reconfig_interface",
        "//envoy/server:instance_interface",
    ],
)

envoy_cc_library(
    name = "listener_reconfig_lib",
    srcs = ["listener_handler.cc"],
    hdrs = ["listener_handler.h"],
    deps = [
        ":reconfig_handler_base_lib",
        "//source/common/config:decoded_resource_lib",
        "//source/common/protobuf",
        "//source/extensions/listener_managers/listener_manager:lds_api_lib",
    ],
)

envoy_cc_library(
    name = "recover_handler_lib",
    srcs = ["recover_handler.cc"],
    hdrs = ["recover_handler.h"],
    deps = [
        "//envoy/server:instance_interface",
        "//envoy/storage:storage_interface",
        "//source/common/storage:storage_lib",
    ],
)
