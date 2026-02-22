cc_library(
    name = "static_btree",
    hdrs = ["static_btree.hh"],
    deps = ["@highway//:hwy"],
)

cc_test(
    name = "static_btree_test",
    srcs = ["static_btree_test.cc"],
    deps = [
        ":static_btree",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "static_btree_benchmark",
    srcs = [
        "benchmark_helpers.hh",
        "static_btree_benchmark.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":static_btree",
        "@google_benchmark//:benchmark_main",
    ],
)

cc_binary(
    name = "nano_benchmark",
    srcs = [
        "benchmark_helpers.hh",
        "nano_bench.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":static_btree",
        "@highway//:nanobenchmark",
    ],
)
