# Approved local Qt setup and Windows environment smoke

The user approved the project-local Qt addition on **2026-10-21 (Asia/Seoul)**. Qt **6.8.3 MSVC2022 x64** is now available at `C:\dev\Windows_fileexplore\tools\qt\6.8.3\msvc2022_64`. An actual **C++20 / Qt Widgets / Qt Test / Qt Concurrent** executable configured, compiled, linked and passed OFFSCREEN tests with the existing MSVC 2026 toolchain.

This is an environment fixture, not the reconstructed Explorer++ application. It does not establish any P0/P1 feature, source-derived accuracy, desktop UI fidelity or deployment completion. No Explorer++ source was accessed. No unrelated software was installed, and user/machine PATH and registry were not changed.

## Installed inputs and identities

Only the two approved official binary archives were acquired from the [Qt 6.8.3 repository](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/). Package build: `6.8.3-0-202503201308`.

| Archive | Bytes | Official SHA-1 | Computed SHA-256 |
|---|---:|---|---|
| [qtbase](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/6.8.3-0-202503201308qtbase-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z) | 39,484,220 | `ab0bd2551618f7128ef30c6d74fe45b6a76e2296` | `41688269fac0565db956c66d9eecae777d16197e0c02cd81b88640c1f5d73d3f` |
| [qttools](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/6.8.3-0-202503201308qttools-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z) | 25,405,796 | `25956e4f72a3b054235a2c6a00c88ba7e7927971` | `76ac7b8c86ca72b6608f1d41ee848aeab19663536990b4968ce52db8ec980bc3` |

Total archive payload: **64,890,016 bytes (~61.9 MiB)**. Official SHA-1 sidecars were fetched again before downloading; both sizes and digests matched. The cache is `tools\qt\downloads`. URL/redirect/header details, download times and listing identities are retained in [qt-acquisition.json](../../artifacts/toolchain-smoke/qt-acquisition.json).

The existing `C:\Windows\System32\tar.exe` (bsdtar/libarchive 3.8.9) listed **4,370 qtbase members** and **571 qttools members**. Every member was checked before extraction: relative paths only, no parent escape, backslash, drive/ADS colon or NUL; verbose entries were regular files/directories, with no symlink or special entry. No overlapping regular file names were found across the two lists.

A fresh workspace staging directory, `tools\qt\.staging-20261021T232946`, was checked to resolve under the intended Qt root. The final prefix did not exist. Extraction and required-file validation succeeded, then the staging directory was renamed into the new prefix without overwriting an existing SDK.

The resulting SDK contains **4,591 regular files / 384,010,023 bytes**. SDK plus retained archive payload is 448,900,039 bytes, excluding filesystem allocation overhead, manifests and build output. [qt-extraction.json](../../artifacts/toolchain-smoke/qt-extraction.json) records exact extraction argv, durations, required files and qmake/qtpaths output.

Required Core/Gui/Widgets/Test/Concurrent libraries, headers/CMake configurations, qwindows/qoffscreen plugins, qmake/qtpaths and windeployqt are present. Archive inspection established that **windeployqt is in qtbase**, correcting the earlier proposal's unverified expectation that deployment tooling would require qttools. Both archives were nevertheless within the explicitly approved two-archive scope.

qmake and qtpaths return the final local prefix and Qt 6.8.3, without adding qt.conf or repairing embedded paths. windeployqt --version returns 6.8.3. It warns that translations/catalogs.json is missing; qttranslations was excluded from this approved subset. Deployment can explicitly use `--no-translations` when that matches the later package's intended scope. No further component was installed.

## Reproducible build command and child environment

The working [build-windows.ps1](../../scripts/build-windows.ps1) uses the existing:

- MSVC compiler **19.51.36256**, toolset directory 14.51.36231, through BuildTools' vcvars64.bat.
- CMake/CTest **4.3.1-msvc1** and Ninja **1.13.2** under the same BuildTools installation.
- Python **3.14.7** already installed at `C:\Python314\python.exe`.
- Approved project-local Qt prefix above.

vcvars runs in a hidden child cmd process seeded with selected system/toolchain prerequisites and a bounded PATH. Its resulting environment remains in memory. The Qt/compiler/build-tool paths are applied only to dedicated child processes. The ambient PATH is not copied wholesale; unrelated environment values are not written to logs or a configuration file. CMake's normal diagnostics see only this bounded build environment. CMake's install prefix is also workspace-local; no `cmake --install` operation ran.

Commands actually executed, in order:

```powershell
& 'C:\Python314\python.exe' 'C:\dev\Windows_fileexplore\artifacts\toolchain-smoke\setup-qt.py'
& 'C:\Python314\python.exe' 'C:\dev\Windows_fileexplore\artifacts\toolchain-smoke\extract-qt.py'

# Successful fresh configure and first compilation:
& 'C:\dev\Windows_fileexplore\scripts\build-windows.ps1' -SourceDirectory 'C:\dev\Windows_fileexplore\artifacts\toolchain-smoke' -BuildDirectory 'C:\dev\Windows_fileexplore\artifacts\toolchain-smoke\build' -RunTests -Fresh

# After adding an explicit QtTest XML logger, incremental verification:
& 'C:\dev\Windows_fileexplore\scripts\build-windows.ps1' -SourceDirectory 'C:\dev\Windows_fileexplore\artifacts\toolchain-smoke' -BuildDirectory 'C:\dev\Windows_fileexplore\artifacts\toolchain-smoke\build' -RunTests
```

Acquisition and extraction scripts refuse an existing archive or SDK destination rather than overwriting it. Do not rerun acquisition against this populated cache. The build helper is reusable; `-Fresh` asks CMake to refresh that selected build directory's configure state. Exact native configure/build/ctest argv are in each stage's `.metrics.json`, without environment snapshots.

## Actual smoke coverage and outputs

The fixture is [CMakeLists.txt](../../artifacts/toolchain-smoke/CMakeLists.txt) plus [smoke.cpp](../../artifacts/toolchain-smoke/smoke.cpp), using `find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Widgets Test Concurrent)`, C++20 and AUTOMOC.

The [QtTest XML](../../artifacts/toolchain-smoke/build/qt-test-results.xml) explicitly records:

| Test slot | Actual outcome |
|---|---|
| cpp20Library | PASS: C++20 std::span over std::array, expected reduction |
| widgetsUnicodeAndSignals | PASS: real QLineEdit, Korean/Unicode string roundtrip, QSignalSpy and OFFSCREEN platform assertion |
| concurrentExecution | PASS: QtConcurrent task completion and result 42 |
| initTestCase / cleanupTestCase | PASS / PASS |

CTest reports **1/1 passed**; the XML contains **3 application test slots plus init/cleanup**, not five independent product scenarios. It reports Qt/QTest 6.8.3, dynamic x64 release libraries built by MSVC2022. This actual compile/link/run demonstrates interoperability for the fixture with installed MSVC2026; it does not expand Qt's official tested-toolchain support claims.

The test executable is `artifacts\toolchain-smoke\build\rea_qt_toolchain_smoke.exe`, **29,184 bytes**, SHA-256 `91a5876034ec1c43d1454d4a061734049862ecc21096b7549c31104b4a5bfe6e`. Source/script/output hashes are retained in [setup-summary.json](../../artifacts/toolchain-smoke/setup-summary.json). Build and toolchain artifacts are ignored by Git; no SDK/binary was added to tracked source.

No desktop window was shown or automated. The XML has an OFFSCREEN font-directory warning: Qt does not ship fonts under lib/fonts. String/signal assertions still passed, but glyph shaping/rendering and Windows-native appearance were not checked. The fixture does not claim visual Unicode fidelity.

## Measured resource use

[run-build-stage.py](../../scripts/run-build-stage.py) creates the native process suspended, assigns it to an owned Windows Job Object, then resumes it hidden. CPU totals and peak memory include that command and descendants. It retains process/job handles as authority, and never kills by a guessed PID/path. Completion waits for the root handle, closes redirection handles, allows a bounded final drain, then terminates only residual members of that owned job. Every successful recorded stage reached **zero active job processes**. Initial configure/build left residual console hosts; the runner records their cleanup counts.

Memory below is **peak simultaneous committed job memory (PeakJobMemoryUsed), not resident working set/RSS**. CPU is cumulative user plus kernel time. Elapsed time includes the runner's bounded final drain/cleanup; Python/PowerShell/vcvars orchestration overhead is outside each native stage's metrics.

| Run / stage | Elapsed seconds | Family CPU seconds | Peak family committed bytes | Exit |
|---|---:|---:|---:|---:|
| Fresh configure, 23:36 | 5.707 | 3.438 | 254,730,240 | 0 |
| First actual compile/link, 23:36 | 8.148 | 5.281 | 373,088,256 | 0 |
| First CTest, 23:36 | 0.587 | 0.141 | 11,571,200 | 0 |
| Incremental configure, 23:37 | 0.500 | 0.484 | 16,629,760 | 0 |
| Incremental build, 23:37 | 0.316 | 0.250 | 25,530,368 | 0 |
| CTest with XML logging, 23:37 | 0.174 | 0.094 | 11,382,784 | 0 |

Successful fresh stage elapsed sum: **14.443 seconds**. The later complete helper invocation was observed by the shell tool as **3.681 seconds**, including bootstrap/orchestration. These are environment-smoke measurements, not product performance targets or A/B results.

The first successful compile is a first build in this fixture, but OS/tool caches were already warm from earlier probes/configuration. No cold OS-cache baseline was obtained. Acquisition took **120.114 seconds**, extraction/identity/path probes **8.383 seconds**; their CPU and process-family peak memory were not measured. AI tokens/API cost were not available to this worker and are not estimated.

## Failed attempts and structural corrections

The initial build attempt reached CMake configure but the metric wrapper waited on residual console-host lifetime and hit **180.019 seconds**. This is an instrumentation failure, not a Qt product/runtime failure; incomplete CPU/peak metrics were not invented. Its logs/metrics remain under `build/logs/20261021T233149287`.

The wrapper was changed to use the root process handle's terminal state, then finalize only its owned Job Object with a bounded drain and explicit active-member accounting. The initial environment bootstrap also exposed an unqualified vswhere lookup warning. Its producing boundary was corrected: a bounded child environment includes the already installed Visual Studio Installer path, and vcvars diagnostics now fail explicitly.

An intermediate correction failed before configure because .NET argv quoting escaped embedded quotes for cmd.exe's /c syntax. The helper now uses an explicit fixed cmd command string for that one batch/environment operation, while every native build argument remains an argument-list value. No arbitrary source/build path is interpolated into shell text.

The first CMake diagnostic log reflected the inherited ambient PATH. After fixing the environment-producing boundary, the successful `--fresh` configure replaced that generated configure state with the bounded environment. The verification checked that the unrelated ambient application/tool paths were absent. No ambient environment dump is retained by the helper.

## SDK licensing inventory and remaining verification

The selected binary archives contain these actual COPYING license files:

- `lib/cmake/Qt6/3rdparty/extra-cmake-modules/COPYING-CMAKE-SCRIPTS`
- `lib/cmake/Qt6/3rdparty/kwin/COPYING-CMAKE-SCRIPTS`

`QtPublicSbomLicenseHelpers.cmake` is also present, but it is a CMake helper, not a license text. The selected archives do not contain a top-level Qt LGPL/GPL/commercial license text in the inspected filename inventory. This inventory is not a complete redistribution-notice package.

Official module declarations identify the applicable licensing alternatives for [Qt Widgets](https://doc.qt.io/qt-6.8/qtwidgets-index.html#licenses), [Qt Test](https://doc.qt.io/qt-6.8/qttest-index.html#licenses-and-attributions) and [Qt Concurrent](https://doc.qt.io/qt-6.8/qtconcurrent-index.html#licenses). Keep the chosen license terms and required third-party notices with any later redistribution; this setup did not claim to complete that later packaging task.

Still unverified here: actual reconstructed product, native desktop appearance, package-on-clean-machine execution, full translation/font support, cancellation regressions for the build instrumentation, and a cold performance baseline. Linux's previously detected Qt6.4.2 environment was not modified or built by this setup.

Experiment A and B must use the same installed Windows SDK/compiler/flags/fixtures. Providing both arms the same toolchain preserves the intended distinction in REA information access.

