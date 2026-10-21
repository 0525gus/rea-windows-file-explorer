# P0 validation ledger

Started 2026-10-22, Asia/Seoul. This ledger describes the current automated checks against the public C++20/Qt6 contracts and the full [requirements](../requirements.md). The latest reviewed actual Windows run at **00:24 KST passed 4/4 suites**, with **41 QtTest cases passed and 1 skipped**, including suite initialization/cleanup cases. Windows link creation lacked privilege; link/cycle behavior remains unverified there. The real stale-entry deletion/replacement checks and all-row ordered-model checks passed in this run. The earlier Qt toolchain smoke is not a product P0 result.

No original Explorer++ source was read, no experiment-A implementation/context was used, and no new REA operation was performed for these tests. These automated tests do not invoke successful real default-app delegation. Separately, the root investigator's actual desktop B observation opened a Korean file through Notepad; that distinct evidence is linked below. Windows/Linux runs are coordinated by the root investigator. The original source gate remains locked; the Windows adapter-only check returned PermissionDenied without inspecting descendants.

## Current suite and evidence scope

| Suite | Meaningful assertions | Evidence classification | Current actual result |
|---|---|---|---|
| `domain_tests` | Empty/history bounds, identity-based repeated visit, successful branch truncation, literal Unicode case folding, numeric names with directories first, both size/date directions, unknown versus zero/max values, source-name/key ties independent of permutation | General contract/API-based new product policy; not proof of original internals | Windows 8 passed, 0 failed/skipped, including init/cleanup |
| `session_tests` | Successful-only history commit, failure/incomplete/pending state preservation, native failure facts, late success/failure rejection after supersede, independent tabs, identity-sensitive refresh selection, safe fake open delegation, delayed-open/closed-tab/shutdown ownership | Mock application semantics and lifecycle; cannot establish real OS behavior | Windows 10 passed, 0 failed/skipped, including init/cleanup; cleanup drain result is checked |
| `platform_tests` | Real `makePlatform()` complete empty listing; **all 1,200 bulk names** and Unicode names versus pinned manifest; every known size/time/type/identity; progress; missing/invalid/cancel errors; actual numeric comparison | Real production boundary integration, per actually executed OS; Windows implementation's REA/API contribution separately classified | Windows 11 passed / 1 link-privilege skip / 0 failed, including deleted/replaced stale-entry checks |
| Windows source-gate denial | Adapter-only request of `verification/source`; success/NotFound are failures, PermissionDenied required | Actual NTFS denial behavior | Windows PASS in named QtTest log; Linux skips Windows-only ACL case |
| Link/cycle fixture | New owned temporary fixture, real directory symlink and cycle, one-level complete listing, link flags and exact finite counts | Real filesystem boundary; unavailable link privilege is a recorded skip, never a pass | Windows SKIP: required privilege not held; no link/cycle success claim |
| `ui_tests` | Actual OFFSCREEN QTest address/keyboard/buttons/double-click/tab-close events, four columns, every bulk/Unicode model name, exact search sets, sort header events, failure preservation, resize/asynchronous close | Real Qt event/model integration; OS adapter exercised, desktop rendering separate | Windows 12 passed, 0 failed/skipped, including full-order oracle for all six choices and mixed directory/file fixture |

The fixture manifest SHA-256 is pinned to `278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1`. Missing fixtures or a different manifest fail rather than being skipped. Expected name/size/time values come from that manifest, not a second filesystem enumerator or a count-only assertion. This does not re-verify each file's content hash; fixture generator `--check` remains a separate fixture integrity check.

Session tests deliberately use a controlled adapter that can finish successfully after receiving stop. This verifies generation ownership rather than assuming cancellation prevents late completion. Its fixture releases blocked work even on an early assertion failure, so test teardown cannot deadlock on the controlled worker. The delayed-open case separately checks that an old open failure cannot poison a newly committed folder and that an already-started open remains owned through close/shutdown until it returns. Real default-app execution is deliberately absent: fake-open success proves only delegation and file-versus-folder routing. NotFound on fake open proves application error handling, not external deletion on a real filesystem.

## P0 requirement checklist

| Requirement | Automated direct coverage being prepared | Remaining proof before P0 completion |
|---|---|---|
| FR-01 UI | Named address/tree/list/tab/status widgets, actual model columns, resize | Actual Windows displayed UI, focus/accessibility, screenshots against reference, sidebar lazy browsing behavior |
| FR-02 folders/drives | Real adapter fixture list, app navigation, initial Drives load | Actual selected-drive/tree interactions and failure/edge cases on each platform |
| FR-03 path/double-click | Alt+D → address/Enter path; session directory activation; actual QTest directory double-click case | Executed Qt results and real desktop run |
| FR-04 history/up | Successful/failed/mock pending history, branch, refresh; real buttons | Original comparison and complete real keyboard/history edge scenarios |
| FR-05 metadata | Manifest size/date for all bulk/Unicode entries; type and 4 displayed columns | Exact displayed values/unknown formatting and original comparison in real window |
| FR-06 sort | Complete domain oracle with directions/ties/unknowns, actual adapter numeric compare, UI header order checks | Real all-row full-order fixture comparison for each direction and original behavior differences |
| FR-07 search | Literal Unicode/regex-symbol/space domain cases; real UI Korean 2-result and 0-result sets | Original scope difference remains explicit; measured responsiveness and real desktop keyboard path |
| FR-08 tabs | Independent mock history/sort/search/selection; real Ctrl+T/W and two paths | Real tab click/switch plus cancellation/close stress in displayed app |
| FR-09 default application | Fake delegation/error routing; separate actual desktop B-UI-002 opens Korean text through Notepad | Original-vs-B parity, other error/association scenarios and packaged execution; fake tests alone never satisfy this gate |
| FR-10 errors | Missing/invalid/incomplete/cancel/permission session preservation, real missing/invalid, Windows real ACL adapter check | Actual desktop recoverable prompts, real external deletion/replacement and unavailable permissions on other OSs |

## Fourteen required scenarios

| Scenario | Current coverage boundary | Status |
|---|---|---|
| T-01 empty folder | Real adapter and Qt model exact zero/complete | Windows automated PASS |
| T-02 ≥1,000 files | All 1,200 names+metadata and UI model rows; progress trace | Windows names/metadata/model PASS; UI event latency/CPU/memory still unmeasured |
| T-03 Korean/special names | Manifest names/size/time + literal UI search + model values; separate desktop Korean file launch | Windows automated PASS and observed Korean Notepad launch; full glyph/rendering comparison remains separate |
| T-04 nonexistent path | Real NotFound and retained UI/session state | Windows automated PASS |
| T-05 access denied | Windows actual locked source adapter check; mocked state semantics | Windows adapter/session PASS; actual Linux denial not yet asserted |
| T-06 history | Domain + async session + real button tests | Windows automated PASS; original complete comparison separate |
| T-07 independent tabs | Session values and actual Ctrl+T/W/path/tab-close events | Windows automated PASS |
| T-08 name/size/date sorting | Contract-specific domain ordering + header events | Windows full all-row/all-direction oracle PASS; original matching separate |
| T-09 search correctness | Exact literal Unicode/zero/full result sets | Windows automated PASS for current-folder contract |
| T-10 copy/move collisions | P1; no test or operation added here | Pending P1 implementation/verification |
| T-11 external deletion | New real adapter stale-entry test deletes/replaces only owned temporary files, requires NotFound/Changed before delegation, verifies neighbor and pinned source fixture file/manifest unchanged | Windows real adapter deleted/replaced stale-entry cases PASS |
| T-12 rename failure | P1; no operation added here | Pending P1 |
| T-13 symlink/cycles | Actual owned temporary one-level link fixture; privilege failure explicit skip | Windows SKIP (privilege unavailable); recursive operation cycles remain P1/P2 checks |
| T-14 exit/restart | Mock retained browse/open worker + normal Qt asynchronous close | Windows automated shutdown/close PASS; actual process restart/package/active final cleanup not yet covered |

Tests intentionally do not mutate P1 fixtures or system/user data. Symlinks and disposable directories are confined to a new `test-data/runtime-platform-links-XXXXXX` directory managed by QTemporaryDir. No chmod/ACL or locked-source changes occur.

## Execution results to append from actual evidence

### Initial actual Windows run and test-boundary corrections

The [CTest log](../../build-windows/Testing/Temporary/LastTest.log) records 2026-10-22 00:07 KST, Windows/Qt 6.8.3: domain failed (0.37 s), session passed (1.71 s), platform passed (1.49 s), UI failed (2.56 s). Those initial CTest processes produced no captured stdout; later named QtTest logs, reviewed below, provided the individual assertions and skip reason.

The [domain diagnostic](../../build-windows/logs/domain-diagnostic.txt) showed **7 pass / 1 fail / 0 skip**, including init/cleanup. The failed expectation equated `Straße` with `STRASSE`, an unstated multi-character transliteration requirement. The public search contract explicitly uses QString::toCaseFolded with literal matching. [Qt documents the case-fold operation](https://doc.qt.io/qt-6.8/qstring.html#toCaseFolded) without promising that transliteration; the actual installed Qt6.8.3 diagnostic returned no match. The test now preserves Korean, literal metacharacter/space checks and metadata checks, adds accented `CAFÉ` → `café` coverage, tests case-insensitive `STRAßE`, and explicitly records `STRASSE` as nonmatching under this API contract. No production search code was changed to accommodate the test.

The [UI diagnostic](../../build-windows/logs/ui-diagnostic.txt) showed **8 pass / 1 fail / 0 skip**, including init/cleanup. The failing test changed address text but sent Return while focus could remain on the table; the table's Return shortcut could consume it. This did not establish an application navigation failure. The test now invokes Alt+D and requires actual address focus before entering the missing path, then checks the error banner and preserved committed state. A separate three-tab regression now closes an inactive tab through its real Qt close button and requires the selected tab/path/model to remain unchanged. At that checkpoint both edits awaited a new Windows run. The root corrected the production inactive-tab selection boundary, and both checks passed in the later official runs recorded below.

OFFSCREEN diagnostics reported missing Qt font-directory and unsupported `raise()` warnings; they do not establish desktop visual/Unicode glyph rendering. No failure was hidden or marked passed after editing alone.

### Reviewed successful Windows run — 00:11 KST

The root-run CTest command retains named text/JUnit logs. I read both log types and the [CTest execution log](../../build-windows/Testing/Temporary/LastTest.log), rather than infer assertions from its green summary. Qt6.8.3 on Windows11 reported:

| Suite | Named text / XML evidence | Passed | Failed | Skipped | QtTest duration |
|---|---|---:|---:|---:|---:|
| Domain | [text](../../build-windows/domain-results.txt), [XML](../../build-windows/domain-results.xml) | 8 | 0 | 0 | 0.004 s |
| Session | [text](../../build-windows/session-results.txt), [XML](../../build-windows/session-results.xml) | 10 | 0 | 0 | 1.728 s |
| Platform | [text](../../build-windows/platform-results.txt), [XML](../../build-windows/platform-results.xml) | 9 | 0 | 1 | 1.121 s |
| UI | [text](../../build-windows/ui-results.txt), [XML](../../build-windows/ui-results.xml) | 10 | 0 | 0 | 2.361 s |

All totals include each suite's initialization/cleanup; they are not 37 independent feature scenarios. The one skip is explicitly `directorySymlinkAndCycleAreFiniteAndIdentified`: Windows lacked symlink privilege. `realDeniedSourceGateRemainsUnread` **passed**, as did all 1,200 manifest names/metadata, late-open ownership and the three-tab inactive-close regression. Named files are overwritten by later runs; current log timestamps and scope must be rechecked when citing them.

The separate [desktop runtime record](../observations/rea-p0-runtime.json) is explicitly Computer Use observation, not REA evidence: B-UI-001 at 2026-10-22 00:14:54 KST shows the Unicode directory's actual desktop address/list/metadata; B-UI-002 at 00:15:06 KST records double-clicking `한글_파일.txt`, its 33 B selection and the resulting Notepad window `한글_파일.txt - 메모장`. This establishes that observed B association launch only; it is not proof of all P0 cases, original parity or clean-machine deployment.

### Added checks — executed in the 00:24 KST run

`staleEntryOpenFailsBeforeDelegation` has deleted and same-path replacement rows. The replacement is created while the original exists and its distinct adapter identity is verified before renaming, preventing an inode/file-index reuse setup from accidentally exercising successful launch. Only a new owned QTemporaryDir is mutated; a neighbor hash, a pinned read-only source fixture file hash and manifest hash must remain unchanged.

The prior adjacent-value UI sort checks were replaced with full ordered-name equality for **all 1,200 rows in six sort choices** (name/size/date × ascending/descending), plus the manifest's mixed directory/file alpha folder. The oracle uses pinned manifest facts and zero-padded ASCII filenames; it does not call production sortEntries or the adapter comparator. Size/date ties keep names ascending even when the primary order descends. The manifest's synthetic blanket “reverse ascending” note is not used because that would reverse ties contrary to the current explicit product contract. Directory timestamps are unfixed in the manifest; the one-directory mixed fixture checks directory precedence without inventing within-directory timestamp facts. Both full-order data rows passed in the latest actual Windows run.

The existing independent-tab session case was strengthened for the root's clarified search policy: refreshing the same location preserves its current query/result set; a successful navigation to another location clears that query and exposes the new listing; the other tab's path/history remain unchanged. This addition passed in the latest Windows session suite and is a new product UX contract, not an original Explorer++ internals claim.

Record OS, Qt/compiler/build hash, exact build/CTest command, timestamp, totals, each failed/skipped test reason, log/XML paths, and elapsed/CPU/peak-memory definitions. A suite's green summary must not hide a required skipped scenario. OFFSCREEN runs cannot prove native Windows layout, glyph rendering, actual OS association launch, or original-behavior fidelity.

Observed source-derived limitations before execution: the session destructor has a defensive pool wait; normal GUI close must first finish the asynchronous shutdown, and the UI tests assert that completion before destruction. The public header has a shared progress counter but no `progressChanged` signal; tests verify actual counter values without assuming that absent signal. Neither observation by itself is an executed regression result.

### Current actual Windows gate evidence — 00:24 KST

The current authoritative run is [logs/20261022T002446646](../../build-windows/logs/20261022T002446646/test.stdout.log), with successful configure/build/test stage metric records and named test logs. The earlier 00:11 numbers above are historical observations; the shared named files now contain the newer run.

| Suite | Passed | Failed | Skipped | QtTest duration | Current evidence |
|---|---:|---:|---:|---:|---|
| Domain | 8 | 0 | 0 | 0.003 s | [text](../../build-windows/domain-results.txt), [JUnit](../../build-windows/domain-results.xml) |
| Session | 10 | 0 | 0 | 1.892 s | [text](../../build-windows/session-results.txt), [JUnit](../../build-windows/session-results.xml) |
| Platform | 11 | 0 | 1 | 1.140 s | [text](../../build-windows/platform-results.txt), [JUnit](../../build-windows/platform-results.xml) |
| UI | 12 | 0 | 0 | 3.447 s | [text](../../build-windows/ui-results.txt), [JUnit](../../build-windows/ui-results.xml) |

Totals include 8 suite init/cleanup cases: **41 pass / 0 fail / 1 skip** is 4 complete suites, not 41 independent requirements. The platform symlink/cycle row is still skipped for missing Windows privilege. Actual source ACL denial, both external deleted/replaced identity rows, all 1,200 size/date/name rows in each direction, directory precedence, query reset/preservation, independent tabs and late-worker ownership all passed within their stated scopes.

CTest reports 6.65 s wall time; the owned test-job metric records 6.725 s elapsed, 3.703125 cumulative family CPU seconds and 23,826,432 peak family committed bytes (not working-set/RSS). This was a warm/incremental environment, not a cold baseline or product responsiveness benchmark. The build stage in this particular final run was no-work incremental (0.190 s), so it must not be presented as a clean compile time. [Configure](../../build-windows/logs/20261022T002446646/configure.metrics.json), [build](../../build-windows/logs/20261022T002446646/build.metrics.json) and [test](../../build-windows/logs/20261022T002446646/test.metrics.json) exact argv/results remain available.

Intermediate failures remain recorded: [00:20 build output](../../build-windows/logs/20261022T002059928/build.stdout.log) shows Windows SDK include-order compilation failure after formatting. The producing include boundary was corrected to keep Windows.h first and `.clang-format` now preserves include blocks. The next [00:22 test metrics](../../build-windows/logs/20261022T002137555/test.metrics.json) record CTest exit 8/33.37 s because UI startup timed out after deployment copied Qt DLLs beside tests and changed plugin discovery. The root set a child-only `QT_PLUGIN_PATH` to the selected SDK's plugins; [the focused diagnostic](../../build-windows/logs/ui-fullsort-diagnostic.txt) then passed 12 cases and the full 00:24 official CTest run passed all suites. These are actual repaired build/runtime-boundary failures, not erased failures or a reason to relabel the skipped link test as success.

The [Phase 4 gate](../architecture/phase4-validation.md) carries this bounded P0 result into Phase 5. All P1 operations, Linux/macOS verification, unavailable Windows links, performance targets/comparison, full original fidelity, packaging and the independent A/B/source-final evaluation remain required work.
