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
        "@highway//:hwy_test_util",
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
        "@googletest//:gtest",
        "@highway//:hwy_test_util",
        "@highway//:nanobenchmark",
    ],
)
