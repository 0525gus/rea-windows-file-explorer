**Current update (2026-10-21 23:39 KST):** The approved local Qt 6.8.3 SDK is installed and actual C++20/Widgets/Test/Concurrent OFFSCREEN smoke checks passed. See [qt-setup.md](qt-setup.md). The read-only pre-install audit below is preserved as historical evidence.

# Development environment audit

Audited 2026-10-21, Asia/Seoul. Input: `docs/requirements.md`, SHA-256 `76b659ec0594607e02b1818683ccc826a7af2c8dff6583888852d351e3890c88`.

Windows already has MSVC, CMake and Ninja. A Windows Qt6 C++ SDK was **not found within the inspected locations**. Ubuntu WSL2 already has the Qt6 Widgets development components needed for an initial Linux build attempt. No software was installed and no persistent system configuration was changed. Starting Ubuntu for the read-only probes changed its runtime state from Stopped to Running.

## Scope and evidence

This audit read the full requirements and the REA skill, then inspected tools through current PATH, selected toolchain variables, named install roots, relevant registry entries, executable version probes, and WSL package/configuration queries. It did not open Explorer++ source, run REA/MCP analysis, compile this project, or launch a Qt GUI. The coordinating investigation owns REA/Ghidra/JDK verification.

Exact diagnostic command strings, outputs or labeled summaries, probe failures, source URLs, and the selected Qt package XML are retained in [environment-audit.json](environment-audit.json). A version query is tool availability evidence; it is not a build or application success claim.

## Observed Windows environment

| Component | Observed version / state | Path or boundary |
|---|---|---|
| OS | Windows 11 Home, x64, build 26200; WSL reports 10.0.26200.9711 | CIM / WSL version output |
| Physical memory | 34,281,861,120 bytes | CIM inventory; application peak usage unmeasured |
| VS Build Tools | 2026, 18.9.2; installation 18.9.12120.119, complete and launchable | `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` |
| MSVC x64 | Compiler 19.51.36256; toolset directory 14.51.36231 | `VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\cl.exe` below BuildTools |
| CMake / CTest / CPack | 4.3.1-msvc1 | `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` below BuildTools |
| Ninja | 1.13.2 | `Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` below BuildTools |
| Windows SDK | Include/Lib directory 10.0.26100.0; installed package 10.1.26100.8249 | `C:\Program Files (x86)\Windows Kits\10` |
| Resource / manifest tools | rc.exe and mt.exe 10.0.26100.8249 present | SDK `bin\10.0.26100.0\x64` |
| Git | 2.55.0.windows.3 | `C:\Program Files\Git\cmd\git.exe` |
| Python | 3.14.7 | `C:\Python314\python.exe` |
| clang-format / clang-tidy | 22.1.3 | `C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\Llvm\x64\bin` |
| Performance tooling | xperf.exe and wpr.exe 10.0.26100.8249 present; not executed | SDK `Windows Performance Toolkit` |
| CDB | Not found at selected SDK x64 path | Other debugger locations unsearched |
| Windows Qt6 | Not found in audited roots / PATH / relevant registry | Whole-disk absence not established |

MSVC, CMake, Ninja, Qt and LLVM tools were not on the current shell PATH. MSVC/CMake/Ninja/LLVM were found by their explicit installed paths. No PATH change is required for this audit. `vcvars64.bat` exists at `VC\Auxiliary\Build\vcvars64.bat` below BuildTools; it has not been applied or used for a compile test here.

The selected toolchain variables `QTDIR`, `QT_HOME`, `QT_HOST_PATH`, `Qt6_DIR`, `CMAKE_PREFIX_PATH`, `VCPKG_ROOT`, `VSINSTALLDIR`, `VCToolsInstallDir`, and `WindowsSdkDir` were unset. No ambient environment dump was retained.

Qt search boundaries: `C:\Qt`, `C:\dev\Qt`, Program Files Qt roots, `C:\Users\hhh\Qt`, the user's Local Programs Qt root, named VS trees, and named vcpkg installed roots. The Qt roots and vcpkg roots were absent. `HKCU\Software\QtProject` had QMake / OrganizationDefaults children but exposed no usable SDK prefix. A Qt-related registry key alone is insufficient evidence of a development kit.

## Observed Linux environment

| Component | Observed result |
|---|---|
| WSL / WSLg | 3.0.3.0 / 1.0.79 |
| Distribution | Existing Ubuntu 24.04.1 LTS, WSL2, x86_64 |
| Kernel | 6.18.54.1-microsoft-standard-WSL2 |
| CMake | 3.28.3 at `/usr/bin/cmake` |
| Ninja | 1.11.1 at `/usr/bin/ninja` |
| GNU C++ | 13.3.0 at `/usr/bin/g++` |
| Qt6 | 6.4.2; qt6-base-dev and qt6-base-dev-tools installed |
| Modules confirmed by pkg-config | Core, Gui, Widgets, Test, Concurrent: all 6.4.2 |
| CMake configs checked as real files | Qt6, Widgets, Test, Concurrent under `/usr/lib/x86_64-linux-gnu/cmake` |
| Qt paths | Prefix `/usr`; qmake6 / qtpaths6 at `/usr/bin` |
| Other tools | git, python3, pkg-config present |
| Display | DISPLAY and WAYLAND_DISPLAY set; `/mnt/wslg` present |
| Missing on this PATH | clang++, clang-format, clang-tidy, Xvfb |
| Actual project build / tests / GUI | Not run by this audit |

Existing Linux tooling is sufficient to **attempt** a Widgets/Test/Concurrent CMake build without installation. Display variables do not prove GUI rendering. Linux and Windows have different Qt versions; cross-platform results must report this difference. Experiment A and B must share identical tooling within each OS.

## Concrete Windows Qt proposal — not executed

Requirement §14.6 requires prior approval for external software installation or system changes. The proposed addition is a workspace-local **Qt 6.8.3 Windows x64 MSVC2022 SDK subset**, extracted under:

- SDK: `C:\dev\Windows_fileexplore\tools\qt\6.8.3\msvc2022_64`
- Archive cache: `C:\dev\Windows_fileexplore\tools\qt\downloads`

Both paths were absent at audit time. C: had 79,257,694,208 free bytes at the prefix check.

The official [Qt repository metadata](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/Updates.xml) identifies package `qt.qt6.683.win64_msvc2022_64`, build `6.8.3-0-202503201308`. The metadata fetched in memory had SHA-256 `b3aaf7ad9c12edd792c1afbe675646fdafa83d9e00854109bf463d300778a21e`. Only metadata, directory listings, archive HEAD responses, checksum sidecars and official documentation were fetched; no archive payload or installer was downloaded.

| Selected archive | HEAD Content-Length | Official SHA-1 |
|---|---:|---|
| [qtbase](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/6.8.3-0-202503201308qtbase-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z) | 39,484,220 bytes | `ab0bd2551618f7128ef30c6d74fe45b6a76e2296` |
| [qttools](https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_683/qt6_683/qt.qt6.683.win64_msvc2022_64/6.8.3-0-202503201308qttools-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z) | 25,405,796 bytes | `25956e4f72a3b054235a2c6a00c88ba7e7927971` |

Combined download: **64,890,016 bytes (~61.9 MiB)**. SHA-1 values come from each archive's official `.sha1` sidecar; local SHA-256 identities should also be recorded after an approved download.

Expected use: QtCore, QtGui, QtWidgets, QtTest, QtConcurrent, CMake package configurations and base build tools from qtbase; deployment tooling such as windeployqt from qttools. Exact archive contents and executable dependencies remain unverified until an approved download can be listed and inspected. Qt's documented CMake component names are [Widgets](https://doc.qt.io/qt-6.8/qtwidgets-index.html), [Test](https://doc.qt.io/qt-6.8/qttest-index.html) and [Concurrent](https://doc.qt.io/qt-6.8/qtconcurrent-index.html).

The **full** installer package lists Qt Creator, documentation and examples as dependencies and contains seven archives. Its aggregate declared size is 222,292,292 compressed bytes / 2,054,629,621 expanded bytes. Those are not the selected subset's sizes. Per-archive expanded sizes are not present in this metadata and have not been measured. Selective extraction must be recorded as a minimal SDK subset, not a full installer dependency closure.

The existing `C:\Windows\System32\tar.exe` reports bsdtar/libarchive 3.8.9 with liblzma 5.8.3. [Upstream libarchive documentation](https://github.com/libarchive/libarchive/blob/master/README.md) lists 7-Zip reading. Reading these specific Qt archives with the installed tar remains untested. No separate extractor installation is proposed. No 7z/7za/7zr executable was found on the inspected Windows PATH or standard 7-Zip roots; none was found in Ubuntu PATH.

[Qt 6.8's support table](https://doc.qt.io/qt-6.8/supported-platforms.html) names MSVC2022 for Windows 11 x64. [Microsoft's ABI guidance](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017?view=msvc-170) includes compatibility between VS2022 and VS2026, subject to linker/runtime restrictions. Using the installed MSVC19.51 with these Qt binaries is therefore a reasonable inference to validate with an actual C++20 configure/link/runtime test; this is not a claim that Qt tested this exact compiler. VC runtime 14.51.36247 is registered, but actual dependency resolution has not been tested.

After approval, the concrete procedure is:

1. Download only the two named official archives into the new workspace cache. Verify their official checksums, sizes and computed SHA-256.
2. List members with existing tar before extraction; reject absolute or escaping member paths. Verify required SDK CMake files, headers, DLLs, import libraries and build/deployment tools.
3. Extract into a new staging directory below `tools\qt`; refuse overwriting any pre-existing destination. Check relocatability with explicit qmake/qtpaths queries and record any necessary SDK-local prefix configuration.
4. Use the existing MSVC/CMake/Ninja paths and SDK only in child build environments. Do not change user or machine PATH or registry.
5. Run C++20/Qt6 Widgets/Test/Concurrent build, test, GUI and deployment smoke checks. Record failures and request a revised plan if additional components are genuinely required.

Qt Creator, source/docs/examples, QML/Quick, WebEngine, MinGW and mobile kits are outside this proposal. No replacement compiler/build tools, Node/npm/Java/Ghidra installation, or system-wide environment change is proposed.

For experiment parity, A and B must use the same SDK, compiler, build flags, fixtures and runtime inputs. Toolchain setup must not give one arm a different environment. REA information access is the intended independent variable.

## Failures and unresolved checks

The first inventory invocation had a PowerShell parser error; the corrected array-based inventory succeeded. The first WSL package format was expanded by sh and failed with `Bad substitution`; a single-quoted format fixed the producer. A PowerShell argument-splat mistake split version arguments into characters; explicit subprocess argument arrays fixed it. The malformed MSVC probe reached the linker with B/v operands and failed before an executable; explicit checks found no B.exe/B.obj/B.ilk/B.pdb. A malformed SDK path array was corrected before drawing SDK conclusions.

The proper `cl /Bv` probe printed version information then returned D8003 / exit 2 because no source input was supplied. It was not a successful compilation. Raw probe failures are retained instead of being reported as passing checks.

Outstanding: Windows Qt addition approval and actual SDK verification; actual Windows/Linux builds and GUI behavior; packaged deployment; exact selected-archive expanded footprint; MSVC/Qt interoperability smoke test; application CPU/peak-memory measurements. This audit does not satisfy Phase 1's REA function/pseudocode gate or any implementation gate.
