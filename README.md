# Static B+-Tree with Google Highway

In this repository we implement a static B+-Tree as defined in [this algorithmica.org article](https://en.algorithmica.org/hpc/data-structures/s-tree/#b-tree-layout-1) using [Google Highway](https://github.com/google/highway).

##  Setup a repository & discover available hardware architectures on your device

We first start by initializing a bazel repository with sensible defaults (see .bazelversion,.gitignore, WORKSPACE).
We populate our MODULE.bazel with the dependencies needed:

```
bazel_dep(name = "googletest", version = "1.17.0.bcr.2")
bazel_dep(name = "google_benchmark", version = "1.9.5")
bazel_dep(name = "highway", version = "1.3.0")
```
The first two are optional, but since we want to test and benchmark our implementations they are highly recommended.

### Listing architectures

With this setup we can query which architectures are available via highway:

```sh
$ bazel run -c opt @highway//:list_targets                                                                 
INFO: Analyzed target @@highway+//:list_targets (32 packages loaded, 321 targets configured).
INFO: Found 1 target...
Target @@highway+//:list_targets up-to-date:
  bazel-bin/external/highway+/list_targets
INFO: Elapsed time: 1.128s, Critical Path: 0.01s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Running command line: external/bazel_tools/tools/test/test-setup.sh ../highway+/list_targets
exec ${PAGER:-/usr/bin/less} "$0" || exit 1
Executing tests from @@highway+//:list_targets
-----------------------------------------------------------------------------
Compiler: GCC 1303
Config: emu128:0 scalar:0 static:0 all_attain:0 is_test:0
Have: constexpr_lanes:1 runtime_dispatch:1 auxv:1 f16 type:1/ops1 bf16 type:1/ops1
Compiled HWY_TARGETS:   AVX10_2 AVX3_SPR AVX3_ZEN4 AVX3_DL AVX3 AVX2 SSE4 SSSE3 SSE2
HWY_ATTAINABLE_TARGETS: AVX10_2 AVX3_SPR AVX3_ZEN4 AVX3_DL AVX3 AVX2 SSE4 SSSE3 SSE2 SCALAR
HWY_BASELINE_TARGETS:   SSE2 SCALAR
HWY_STATIC_TARGET:      SSE2
HWY_BROKEN_TARGETS:     LASX LSX
HWY_DISABLED_TARGETS:  
Current CPU supports:   AVX3_ZEN4 AVX3_DL AVX3 AVX2 SSE4 SSSE3 SSE2 EMU128 SCALAR
```

As we can see, there is a fairly modern CPU from AMD in my machine, supporting AVX3.