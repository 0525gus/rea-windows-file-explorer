# Post-freeze GUI comparison harness

This validation-only program links the **existing A MSVC Release static libraries** and frozen public headers. It does not rebuild or change either initial implementation. Phase 6 freeze authority is `../phase6-freeze-verification.json`, B commit `64032d766cf634fe00b45aa0d8c951878e0ef5c3`. `a-gui-benchmark-manifest.json` records source/header/library/executable identities and limitations after building. The imported library digests are checked during CMake configuration and manifest creation.

Build, without running a benchmark:

```powershell
.\scripts\build-windows.ps1 -SourceDirectory verification/benchmarks -BuildDirectory build-comparison
C:\Python314\python.exe verification/benchmarks/write_manifest.py --build-directory build-comparison
```

The later supervisor invocation uses `build-comparison/comparison_a_gui.exe --path C:\dev\Windows_fileexplore\test-data\fixture-v1\read-only\bulk --benchmark <new-absolute-json-path>`. Output is exclusive-create: choose a fresh path for every run. Root must run A and B sequentially using the same Qt QPA mode/plugin, host, dataset and process-family supervisor. The harness is prepared for measurement; building does not execute it.

Like frozen B's `src/main.cpp` benchmark, this creates and **shows** a MainWindow, starts workload/heartbeat clocks after window construction/show and before navigation, uses a **5 ms PreciseTimer**, measures first listing plus **five immediate warm refreshes**, records per-sample and overall heartbeat gaps, and hashes the compact QJson array of all displayed names. All 1,200 model/proxy names must match the pinned bulk hash. Slow GUI events use the same 20 ms threshold/100-event cap. The complete workload has a 30 s Qt deadline; an externally blocked event loop still needs the existing owned-process supervisor timeout. No OS cache flush occurs; first listing is not claimed as cold cache.

A's frozen app selects Fusion style, so this harness does too. A exposes no public global worker-idle method. Its ready boundary requires the correct committed/current listing location, no current loading, no mutation busy state, and exact rendered model/proxy names. This limitation is recorded rather than accessing private watcher/pool state. A does not expose B's separate derive/render timings, so those fields are null. Both programs' public completion timers can include different internal work; compare completed GUI tasks and retain these boundaries.

The older A `tests/benchmark.cpp` hides the window and performs three warm refreshes. Its prior outputs remain separate and are not directly compared with this six-sample shown-GUI procedure. No direct native desktop interaction or benchmark run is performed by this build task. App-local runtime DLLs are copied from the existing Qt SDK; plugins remain caller-selected through the same child environment used for B. Process-family wall/CPU/peak committed memory are captured later by `scripts/run-build-stage.py`, not estimated from Qt sample times.

Actual build on 2026-10-22 09:11 KST: the command above completed CMake configure and C++20/MSVC Release compile/link successfully, without compiler warnings in retained stdout. Logs are `build-comparison/logs/20261022T091150988`. Configure: 6.455612 s wall, 3.328125 s family CPU, 258,052,096 bytes peak family committed memory. Build: 6.935580 s wall, 5.546875 s family CPU, 354,193,408 bytes peak family committed memory. The existing Job runner reported 2/1 residual **owned** helper processes respectively and terminated them after its bounded final drain; these are not benchmark process outcomes. No benchmark was run.
