load("@my_deps//:requirements.bzl", "requirement")

py_binary(
    name = "plot",
    srcs = ["plot.py"],
    imports = ["."],
    deps = [
        requirement("pandas"),
        requirement("seaborn"),
        requirement("tabulate"),
    ],
)
