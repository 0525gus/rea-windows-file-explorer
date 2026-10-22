# Phase 4 P0 checkpoint — Experiment A

Status: implementation, automated P0 checks and direct packaged Windows UI/default-app checks complete. P1 remains gated. No REA, other-arm design/code, or original source access occurred. This is a no-REA implementation, not an internal-behavior reconstruction claim.

## Executed validation

Windows: actual C++20/MSVC build with Qt 6.8.3; CTest 3/3 suites pass. Functional cases excluding QtTest init/cleanup: **28 pass, 0 fail, 1 skip** (29 declared). Latest build/test logs: `build-windows/logs/20261022T001811025`. Windows ACL-denied directory enumeration returns AccessDenied. Symlink creation test skips because Windows reports "A required privilege is not held by the client"; no elevation, ACL or system setting change was attempted.

Linux: actual WSL Ubuntu/GCC build with Qt 6.4.2; CTest 3/3 suites pass. Functional cases: **28 pass, 0 fail, 1 skip**. The skipped Windows ACL fixture is explicitly not a Linux permission test. Linux creates an A-owned directory cycle symlink, lists it as one link entry, does not recurse and removes it. GUI tests use offscreen Qt; no Linux desktop/default-association launch is claimed.

QtTest evidence is in `build-{windows,linux}/test-reports/{core,platform,ui}.{txt,xml}`. Tests cover pure history, transactional failed navigation, late results, closing a pending tab, worker shutdown, mock default-open dispatch, real empty/1,200/Unicode listings and exact metadata sums, missing/not-directory/ACL errors, external delete, cancellation, tree routing, typed sorting/directions, filter and selection retention, independent tabs, shortcuts, double-click, resizing and two window lifecycles. Mock results prove controller behavior, not native default-app execution.

Clang-format 22.1.3 dry-run/Werror passed over all owned C++ headers/sources/tests. Clang-tidy 22.1.3 passed the six production translation units; final two changed units were rerun and passed. One analyzer check (`clang-analyzer-optin.core.EnumCastOutOfRange`) is explicitly disabled because it flags valid bitmask combinations in the installed MSVC standard-library filesystem headers; no other analyzer findings are suppressed. Build uses /W4 on MSVC and -Wall/-Wextra/-Wpedantic on GCC. Logs are in `artifacts/clang-*.log`.

Windows package: `artifacts/package/explorer_a.exe`. Existing windeployqt copied runtime dependencies locally. The package's hidden initial-directory smoke returned 0 with child PATH restricted to package and Windows directories, then exited cleanly. Render returned 0 and `artifacts/p0-window.png` was visually inspected: tabs, navigation/address/search row, flat command strip, sidebar, details table and status in a dark theme; all six Unicode names remain represented, long text elides in the view. This render uses our hidden native-platform QWidget; no other application's screenshot was taken.

## P0 and mandatory scenario coverage

| Feature | Checkpoint result |
|---|---|
| FR-01 UI | Implemented; actual own render and offscreen navigation/tree/tab/table/status tests pass |
| FR-02 folder browsing | Actual drives/empty/1,200-entry/Unicode adapter checks pass |
| FR-03 path/double-click | GUI address Enter and real fixture folder double-click pass |
| FR-04 history | Back/Forward/Up, failed history cursor, refresh and forward truncation pass |
| FR-05 metadata | Exact six names, 376 total bytes, valid dates/types; displayed details pass |
| FR-06 sort | Typed name/size/date and direction; folders stay first; pass |
| FR-07 search | Current-folder Unicode substring yields exact two observed names; clear/refresh pass |
| FR-08 tabs | Create/switch/close/default This PC, independent state and late completion pass |
| FR-09 default open | Implemented OS adapter; mock UI dispatch and missing-file handling pass. Actual packaged app double-click produced Notepad title 한글_특수문자_é_😀.txt - 메모장; direct UI evidence in p0-direct-ui.json |
| FR-10 errors | Real missing/not-directory/Windows ACL denial/external-delete checks and retained UI state pass |

Mandatory scenarios 1–9, 11 and 14 have relevant automated evidence. Scenario 5 has real Windows ACL evidence but Linux denied-access remains unverified. Scenario 13 has actual Linux link/cycle listing evidence; Windows link creation is unavailable, and recursive mutation behavior belongs to P1. Scenarios 10 (copy/move collisions) and 12 (rename failure) are explicitly P1 pending. Scenario 14 has two offscreen window create/close cycles plus a deployed process smoke; normal desktop close/relaunch remains root review. The fourteen-scenario project scope is unchanged.

## Resource observations

Real immutable bulk folder: 1,200 entries. Latest Windows process: UI initialization 398 ms; first listing 114 ms; warm refreshes 112,107,112 ms. Observed 5 ms timer maximum gaps were 5–6 ms, with 21–23 ticks while each load was pending. Complete four-listing process-family run: elapsed 1.0459758 s, CPU 0.9375 s, peak simultaneous committed family memory 39,473,152 bytes (not resident RSS). Raw result: `artifacts/benchmark-windows.json` and `artifacts/metrics/benchmark-20261022T001923*.metrics.json`.

Linux earlier equivalent four-listing workload: first 1,574 ms; warm 1,553/1,556/1,653 ms, all 1,200 entries, timer maximum gap 5 ms. `/usr/bin/time -v`: elapsed 6.50 s, user 0.12 s + system 0.72 s, maximum resident set 31,772 KiB. This is WSL reading `/mnt/c`; it is not a native-Linux-disk result. Linux maximum RSS is the reported process/child maximum, not a simultaneous process-family peak. No Linux process-family memory metric was available. Raw files: `artifacts/benchmark-linux*`.

"First-process-listing" means a new process's first request; OS filesystem-cache coldness is unknown. These measurements do not establish desktop paint latency or equivalence to original Explorer++ timings. Root owns original workload measurements and cross-arm comparisons. No source-level accuracy percentage, token use, API cost or deployment ID is fabricated.

## Limitations and remaining scope

P1 FR-11–20 is not implemented at this checkpoint except base multi-selection/status and existing navigation shortcuts; it remains full declared scope after root authorization. P2 dark theme is implemented; preview, native menus, favorites, advanced search, watcher, cancellation/undo, settings and shell special folders are unimplemented. No macOS machine: source guards were reviewed, but compilation/runtime remain unverified. Common filesystem and UI code use Qt/C++ APIs; Windows native headers and ShellExecuteW occur only inside the platform adapter.

Current search intentionally matches names in the current directory; the original's separate search dialog observed a checked recursive option on a folder without children, which does not establish recursive equivalence. Invalid paths retain the prior committed listing and show an inline recovery error, a deliberate requirement-driven usability extension to the observed original path attempt.

Session `/root/experiment_a` inherited the parent's model/reasoning. Exact deployed model ID, billing and total token usage are unknown. The tool invocation trace is the source for call counting; this arm has no exported aggregate counter. REA call count is exactly zero. A/B separation relies on the fresh context and whitelist instruction under a shared account; it is not enforced by per-arm ACLs. Original-source access remains gated by root and actual ACL. Input/source hashes and phase timestamps are retained independently.

## Direct packaged UI addendum

Under root-granted exclusive UI ownership, the packaged app navigated to the immutable Unicode fixture, double-clicked the distinct safe text document, and the subsequent window inventory returned actual Notepad title "한글_특수문자_é_😀.txt - 메모장". No content was edited. A missing path retained the six rows and showed the visible file-not-found error. F5 recovered; Ctrl+T and Ctrl+W preserved the existing tab. Normal Alt+F4 close was followed by a settled inventory containing zero own app windows. Full action facts and package hash are in p0-direct-ui.json. Initial input geometry failure and stale immediate close inventory are explicitly retained alongside settled outcomes. UI ownership was returned to root before Phase 5.

