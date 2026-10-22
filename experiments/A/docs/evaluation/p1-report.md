# Independent A implementation and evidence — Phase 5

Status: P0 and P1 implementation and expanded Windows/Linux automated validation complete; independent initial implementation/report freeze prepared before source-based review. No REA call, original-source read, or B design/code read occurred. This report describes independent general-API and AI-inference work. It does not claim internal Explorer++ implementation accuracy.

## Functional scope

FR-01–10 remains implemented and covered by the frozen P0 evidence plus the final regression suites. The P0 packaged app's direct Windows double-click produced `한글_특수문자_é_😀.txt - 메모장` in actual Notepad, and its missing-path recovery/tab/normal-close behavior was observed with the computer-use skill. The exact P0 package digest and UI action record are in `p0-direct-ui.json`. P1 extends the same portable UI/application/platform contracts.

| Requirement | Implemented behavior and evidence |
|---|---|
| FR-11 Copy | Multiple regular files and directory trees, Unicode paths, byte progress, content digest checks; source modification time preserved |
| FR-12 Move | Native no-replace rename first, staged copy/remove fallback; preflight conflict dialog supports Skip all/Replace files/Cancel; actual same-volume and replacement/skip tests |
| FR-13 Delete | Normal Delete requests recycle with no destructive fallback; explicit permanent-delete confirmation is implemented and actually tested only on owned disposable files. Native recycle execution is unverified because its trash location lies outside the arm's disposable workspace |
| FR-14 Rename | One file/folder leaf name, invalid/occupied/missing paths rejected without overwriting; real adapter and GUI dialogs/shortcut tests |
| FR-15 New folder | Validated single leaf under current location; actual filesystem and GUI dialog checks |
| FR-16 Multi-select | Ctrl/Shift extended selection and Ctrl+A, shared selection across Details/List/Icons, batch copy/move dispatch |
| FR-17 Drag/drop | Local URLs only, view-owned input path, incoming Copy/Move drop events to current folder or folder row; outgoing URI MIME supports OS apps. Actual cross-application desktop drag gesture remains unverified |
| FR-18 Views | Details, compact List and Icons use the same facts/filter/sort/selection model; selection in the one-column list is preserved |
| FR-19 Shortcuts | Navigation/tab/search shortcuts plus view-scoped Ctrl+A/C/X/V, F2, Delete, Shift+Delete and Ctrl+Shift+N; text edits keep their own clipboard commands |
| FR-20 Status | Folder/filtered/selected counts and bytes, asynchronous operation phase/item/byte progress, mixed-item results and recovery paths |

## Fourteen mandatory scenarios

| # | Scenario | Evidence and limits |
|---|---|---|
| 1 | Empty folder | Real immutable empty fixture and GUI zero-row navigation |
| 2 | 1,000+ entries | Exact 1,200 names and metadata; real workload benchmark plus offscreen event heartbeat |
| 3 | Unicode/special names | Exact six source fixture names/metadata, GUI filter, copy hashes/modified time, direct Windows default-app document title |
| 4 | Missing path | Real NotFound/NotDirectory, GUI preserved committed listing/history, direct packaged error banner/recovery |
| 5 | Permission denied | Actual Windows ACL-denied enumeration; Linux equivalent permission fixture unavailable. No ACL changes |
| 6 | Back/Forward | Core/UI/direct behavior; failed history cursor retained, refresh no duplicates, forward truncation |
| 7 | Independent tabs | Paths/history/filter/sort/selection; new This PC; close/late completion ignored |
| 8 | Sort | Name/size/date ascending/descending using typed facts; deterministic folder grouping |
| 9 | Search accuracy | Unicode query returns the exact two names; current-folder scope, clear/refresh state |
| 10 | Copy/move conflicts | Actual initial collision detection, skip and confirmed replacement; no mutation before unresolved initial conflicts; race/lock/source-change/mixed results tested |
| 11 | External deletion | Real disposable deletion between enumeration and open; missing-file error and empty refresh; source removed between batch items yields partial result |
| 12 | Rename failure | Invalid leaf, occupied target, missing source preserve remaining original data; normal GUI rename succeeds |
| 13 | Links/cycles | Linux actual directory cycle is listed as one link and mutation preflight refuses it without traversal. Windows symlink creation privilege unavailable; no elevation/settings changes |
| 14 | Exit/restart | Two offscreen window lifecycles, pending browse close, manager shutdown, real package initial-directory smoke; direct P0 app normal close confirmed. P1 actual desktop replay awaits root's blackbox review |

These rows are coverage statements, not a manufactured 14/14 cross-platform equivalence score. The original-side observations do not cover every P1 scenario, so only actually shared observations can count in a later behavior-match denominator.

## Safety and maintainability

All mutation tests create directories beneath the arm's own build directory. The common fixture is immutable. Replacement stages payload in an exclusively created sibling workspace, preserves an existing destination until commit, retains errors/partial results, and removes a move source only after copy completion and source revalidation. Directory merging and link/reparse mutation are explicit Unsupported outcomes. Permanent deletion cannot proceed without the request's confirmation flag and GUI confirmation.

Cleanup failure retains a native directory lease and session ownership record. A real Windows sharing-lock test forces cleanup failure, substitutes a different directory at the same name and confirms retry refuses it; restoring the original identity permits retry. Mixed outcomes and cancellation during final cleanup are exercised. Recovery paths survive in operation results and the UI offers an explicit session retry action. An app restart does not infer ownership from path spelling; retained paths then require manual recovery.

A single producing model owns file facts and URL MIME; both views consume it. A single view input boundary handles keyboard and drop commands; selection is gathered from name-column indexes so list selections are not lost through full-table-row assumptions. UI contains no native filesystem or Win32 mutation calls. Native platform APIs stay in `src/platform/`. Qt/Core contracts remain mockable and P0 fake-adapter lifecycle cases remain part of final testing.

## Platform and fidelity limits

Windows and Linux were actually compiled and tested. macOS code guards and API boundaries were reviewed but no macOS build/runtime environment exists. WSL reads `/mnt/c`, so its timings are not native Linux disk performance. Linux first tries renameat2(RENAME_NOREPLACE); actual DrvFS rejected that flag, so only that unsupported-filesystem case uses Qt's documented non-overwrite rename contract. This fallback is not claimed to resist an adversarial concurrent destination race. Actual cross-volume moves, native recycle, desktop external drag gestures, and macOS behavior remain unverified.

The application does not offer directory merges, link mutation, thumbnails/preview, native context menus, favorites, advanced/recursive search, filesystem watching, user settings, undo/redo or Shell special locations. P2 dark mode is implemented. Worker cancellation exists to support safe shutdown/testing; full end-user cancellation UX is not claimed as a completed P2 feature.

## Experiment integrity and timing

A used a fresh context, whitelist inputs, no REA skill/output, no B materials and no original source. Shared Windows account does not enforce separate per-arm ACLs; behavioral isolation is instruction/context based. Original source access remains denied until the root's Phase 6 authorization after both independent freezes.

Session identity: `/root/experiment_a`; parent model/reasoning inherited without override. Exact deployment ID, total tokens, billing/API cost and model inference resource usage are unavailable. REA calls: zero. Phase boundaries and input hashes are retained. Externally reported usage-limit interruptions have unknown starts/durations; wall-clock phase durations include them and must not be presented as active development time. Supervised Windows build/test/runtime records retain actual CPU, elapsed time and peak simultaneous process-family committed memory; failed attempts are kept. Linux benchmark resource data uses `/usr/bin/time` process maximum RSS and is not the same metric as Windows Job Object committed memory. Root's later common-procedure comparison supersedes any arm-specific preflight timing as a cross-arm conclusion.

## Final executed checks and artifacts

Windows final C++20/MSVC Qt 6.8.3 build and CTest: **5/5 suites pass**, **51 functional cases pass, 0 fail, 2 skip** from 53 cases excluding QtTest init/cleanup. Skips: browse-cycle link creation and mutation-cycle link creation require a Windows privilege not available to this user. Latest final build/test resource/log directory: `build-windows/logs/20261022T082332086`.

Linux final GCC/Qt 6.4.2 build and CTest: **5/5 suites pass**, **50 functional cases pass, 0 fail, 3 skip**. Skips: Windows ACL denial fixture, Windows mandatory sharing-lock copy fixture, and Windows cleanup sharing-lock/identity-retry fixture. Linux actual cycle enumeration and mutation refusal pass. Text/JUnit evidence and exact totals are in `build-{windows,linux}/test-reports/` and `p1-test-summary.json`; source tests do not modify the immutable fixture.

Expanded executed cases include move conflicts (initial refusal, Skip, confirmed Replace), copy timestamp preservation, existing-directory merge refusal, Windows locked-source copy preserving both source and prior destination, mixed item success/failure, cancellation while copying/finalizing, and actual Windows retained-workspace identity/cleanup retry.

Clang-format 22.1.3 dry-run/Werror passed all owned C++ headers/sources/tests. Clang-tidy 22.1.3 explicitly returned 0 across all nine production translation units (`artifacts/clang-tidy-p1-status.json`, corresponding log). The MSVC-standard-library enum bitmask analyzer exclusion documented in the P0 report remains the only check exclusion. Final GCC build log contains no compiler warnings/errors.

`artifacts/package/explorer_a.exe` is the final launchable Windows package, SHA-256 `2da22246ab877bcb97528ca7c59f84a68f1d53406f48400697ed56567d9cd7b0`. Existing windeployqt staged Qt dependencies locally. Final package smoke returned 0 with SDK paths removed from child PATH. It relies on the host's already present MSVC runtime; nothing was installed. `artifacts/p1-window.png` is a visually inspected native-platform grab of our own hidden QWidget: modern dark tab/navigation/search/flat command/sidebar/details/status layout. No original/other-app pixels were extracted as assets. Final P1 direct desktop replay is not substituted by this render.

Final same-arm bulk preflight, run after compiler/static-analysis completion, retained 1,200 entries on every request. Windows: first request 97 ms; warm refreshes 96/95/97 ms; maximum observed 5 ms timer gap. Complete process-family elapsed 0.9594494 s, CPU 0.8125 s, peak simultaneous committed memory 39,190,528 bytes. Linux WSL `/mnt/c`: first request 1,492 ms; warm 1,517/1,548/1,550 ms; max timer gap 5 ms; `/usr/bin/time` elapsed 6.29 s, CPU 0.08+0.76 s, maximum RSS 32,388 KiB. These are arm-specific preflight metrics, not the final common-procedure A/B comparison. First-process does not mean cold OS cache; desktop frame/paint latency was not measured. Raw `benchmark-p1-*` JSON/resource files and supervised Windows metric JSON are retained.

The initial freeze manifest binds copied source, public contracts, reports, tests, package and selected evidence. P0's earlier snapshot remains separate. The root may conduct blackbox comparison after both independent initial freezes; original source stays inaccessible until explicit Phase 6 authorization.
