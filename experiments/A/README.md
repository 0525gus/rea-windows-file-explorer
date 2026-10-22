# Explorer A — independent no-REA reconstruction

C++20/Qt6 Widgets application. This arm uses the requirements, shared executable runtime observations and official framework APIs. No REA analysis or original Explorer++ source was used. Scope is FR-01–20. The independent implementation now includes P0 and P1; initial source/report freeze precedes any source-based accuracy review.

## Build and run

Windows (existing MSVC/Qt SDK, no installation):

```powershell
& C:\dev\Windows_fileexplore\scripts\build-windows.ps1 -SourceDirectory C:\dev\Windows_fileexplore\experiments\A -BuildDirectory C:\dev\Windows_fileexplore\experiments\A\build-windows -RunTests
& .\scripts\run-owned-stage.ps1 -Stage package
.\artifacts\package\explorer_a.exe C:\dev\Windows_fileexplore\test-data\fixture-v1\read-only
```

Linux (existing WSL Ubuntu, Qt 6.4.2):

```sh
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel 2
QT_QPA_PLATFORM=offscreen ctest --test-dir build-linux --output-on-failure
./build-linux/explorer_a /path/to/folder
```

The executable accepts an initial directory, `--smoke` (hidden initial listing then exit), and `--render output.png` (render its own QWidget and exit). Normal launch defaults to This PC. The local package was tested on this Windows host with SDK paths removed from its child PATH. It depends on the host's existing MSVC runtime; this PoC does not install anything.

## Usage

Address Enter or folder double-click navigates. Back/Forward/Up, F5 and independent tabs retain history. Ctrl+L focuses Address; Ctrl+F focuses current-folder filename search; Ctrl+T/W creates/closes tabs; Ctrl+Tab cycles. Search is a Unicode case-insensitive substring filter of the current directory; it does not silently recurse. Column headers and Sort menu support name, byte-size and modified-date sorting. Open/Enter/double-click sends a selected file to the OS default association. Details columns, metadata, selected count/bytes, loading progress and recoverable errors are shown.

Dark layout uses own widget code and generic Qt/system icons; the reference image is not included as UI artwork. New folder, rename, copy/cut/paste, destination selection, conflict decisions, safe deletion, Details/List/Icons, and local URL drag/drop share the same application operation manager. Ctrl/Shift multi-selection is preserved across views. File commands: Ctrl+A/C/X/V, F2, Delete, Shift+Delete and Ctrl+Shift+N. Normal Delete requests the system recycle operation and reports failure without permanent fallback; Shift+Delete requires an explicit confirmation. Replace files also requires an explicit choice. Link mutation and directory merging are refused.

## Evidence

- `docs/architecture/`: independent pre-implementation design and ADR.
- `docs/evaluation/p1-report.md`: final implemented scope, checks and limitations; `p0-report.md` and `artifacts/p0-freeze` preserve the earlier checkpoint.
- `docs/evaluation/p1-test-summary.json`: functional-case counts excluding QtTest init/cleanup.
- `build-windows/test-reports/`, `build-linux/test-reports/`: QtTest text and JUnit XML.
- `build-windows/logs/`: actual hidden build/test commands, logs and process-family resource measurements.
- `artifacts/p1-window.png`: own-window native-platform QWidget render, not a desktop screenshot. The original P0 render is retained separately.
- `artifacts/benchmark-*.json`: complete 1,200-entry workloads; first process and warm refresh are separate.

Actual P0 desktop default-app launch was confirmed through the packaged app: the distinct Unicode file appeared in Notepad. Later P1 cross-application drag gestures and native recycle integration remain unverified; offscreen incoming drop events and outgoing URL MIME were tested. Root owns final blackbox comparison. Windows real symlink creation is unavailable without an ungranted privilege; Linux exercised a real cycle link. macOS has no build/run environment and is unverified. No persistent user settings are written.

