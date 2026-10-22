# Phase 6 functional comparison from retained evidence

Both arms initially had executed evidence for19 core feature routes; FR-17 native external interoperability was incomplete at that checkpoint. Postfreeze B now has user-assisted native Copy endpoints in both OS-app directions, in addition to actual Qt Move/drop and native internal Copy. Its core FR-17 route is therefore corroborated, giving bounded core-route evidence in20 feature categories; A external interoperability remains unverified. Neither19/20 nor20/20 is a full implementation-verification or fidelity percentage. The fixed14 mandatory scenarios remain6 narrowly shared endpoints,2 partial comparisons and6 original-side unknowns; no14/14 original equivalence is claimed.

This successor audit used retained records and frozen A/B policy code. It performed no new UI actions, compiler/test runs, REA queries or Explorer++ source reads. Initial documents and implementations remain unchanged. The JSON companion preserves45 input hashes, evidence locators and count definitions.

## Functional requirements

| ID | Original observed boundary | A evidence | B evidence | Comparison / limit |
|---|---|---|---|---|
| FR-01 Basic UI | Explorer++ shows address, tree, list, tabs and status; its legacy layout is not the UI target. | Historical native window and final render show tabs, full path/search, Home/This PC tree, details table and status. | Native observations and final image show breadcrumbs, known folders, compact commands/table/status and details pane. | Both provide the required components. B follows observed Win11 breadcrumb/sidebar/details structure more closely; no objective visual or UX percentage. References: A-UI, A-IMAGE, A-RENDER, B-IMAGE, WIN11, B-MATRIX |
| FR-02 Folder browsing | OBS-001 shows five folders; OBS-002-settled zero items; OBS-007 1200 items; OBS-005 six Unicode entries. | Real emptyFolder, bulkComplete, unicodeAndMetadata and rootsAndCapabilities pass; Qt address/folder activation passes. | Real empty/manifest1200/Unicode tests and native tree traversal pass. | Core counts agree. Original bulk UIA truncated names, so complete original1200-name/metadata equality remains unknown. References: ORIG, A-PLATFORM, A-QTUI, B-PLATFORM, B-QTUI, B-NATIVE0 |
| FR-03 Path and doubleclick | OBS-001 address Enter and OBS-002 folder double-click settle to expected locations. | A-UI-001 native address entry and Qt addressAndFolderActivation pass. | B-UI-001 native address entry, Qt realDirectoryDoubleClick and B-P1-UI-008 native ancestor click pass. | Core routes supported. B has interactive breadcrumbs; final A uses a plain full-path field. This audit performed no new interactions. References: ORIG, A-UI, A-QTUI, B-NATIVE0, B-NATIVE1, B-QTUI |
| FR-04 Back/Forward/Up | OBS-003-settled/016/017 show particular Back/Forward/Up paths and forward state. | Core backForwardUpRefresh/historyTransaction use a controlled adapter; Qt tab/navigation shortcuts pass. | Actual Qt navigationButtonsBackForwardAndUp and session history/branch/failure tests pass. | Compatible semantics with different sequences and modalities. No synchronized identical full history/failure transcript across three apps. References: ORIG, A-CORE, A-QTUI, B-QTUI, B-SESSION |
| FR-05 Name/size/type/modified | OBS-008/011 display six names, sizes25/185/46/40/47/33 bytes and local times11:00-11:05. | Actual metadata and Qt six-name/four-column tests pass; final image uses TXT file, exact bytes and local dates. | Manifest metadata, native observations and six-entry snapshots pass; image uses Korean Shell type text and exact bytes. | Source facts compatible; localized labels and formatting differ. Actual Win11 rounds these small files to1KB; no identical formatting claim. References: ORIG, A-PLATFORM, A-QTUI, A-IMAGE, B-PLATFORM, B-PKG1-MODEL, B-IMAGE |
| FR-06 Name/size/date sorting | OBS-009 size ascending, OBS-010-settled date ascending and OBS-011 name order; other directions/ties/folder rules incomplete. | Typed Unicode sorting tests pass for three columns/directions; controlled folder grouping and final image name order retained. | Full manifest permutations for1200 files and directory rows, unknowns/ties and final image name order retained. | Both sort. Original complete permutation/tie/locale/folder semantics are not paired; six-name visual agreement is narrower. References: ORIG, A-QTUI, A-IMAGE, B-QTUI, B-DOMAIN, B-IMAGE |
| FR-07 Current-folder name search | OBS-013 separate Search dialog returns exact two names for 한글; Search Subfolders checked, no child folders. | Production Qt asserts exact same two names and clear to six; source uses case-insensitive contains. | B-UI-003 native exact two names plus Qt two/zero/clear-six; source uses case-folded literal filtering. | Same query result. Both inline current-folder filters differ from original dialog/default recursion/unchecked Case Insensitive. No full search-policy equivalence. References: ORIG, A-QTUI, A-SOURCE-SEARCH, B-NATIVE0, B-QTUI, B-SOURCE-SEARCH |
| FR-08 Create/switch/close tabs | OBS-004 creates This PC; OBS-006/018 retain separate paths; OBS-019 closes one tab and retains another. | A-UI-005/006 native create/close retains Unicode; core/Qt independent histories, filter, sort and selection pass. | Qt independent empty/Unicode paths, invalid-input preservation, inactive three-tab close and session separation pass. | Retained folders and close are compatible. A native evidence is historical P0; B full route here is Qt. Original full per-tab history remains unobserved. References: ORIG, A-UI, A-CORE, A-QTUI, B-QTUI, B-SESSION |
| FR-09 OS associated open | OBS-014 unsuccessful attempt excluded; OBS-015 café.txt produces Notepad. | A-UI-002 opens 한글_특수문자_é_😀.txt in Notepad on historical P0 package. | B-UI-002 opens 한글_파일.txt in Notepad; stale deleted/replaced identities refuse delegation in tests. | All three demonstrate association on different files, not identical input/bytes or native address mapping. Final A P1 native replay is absent. References: ORIG, A-UI, A-REPORT, B-NATIVE0, B-PLATFORM |
| FR-10 Denied/invalid/deleted errors | OBS-012 invalid path retains Unicode list and input; no dialog in queried windows. No original denied/deleted-open execution. | Actual ACL, NotFound/NotDirectory and externalDeletion tests; native A-UI-003/004 banner and F5 recovery. | Actual source-gate ACL denial, stale deleted/replaced-open refusal and Qt displayed deletion/NotFound/F5 recovery. | List preservation agrees. A/B explicit error feedback differs from bounded original no-dialog observation; original denied/deleted policy unknown. References: ORIG, A-PLATFORM, A-UI, B-PLATFORM, B-QTUI |
| FR-11 Single/multiple copy | OBS-022 copies one Unicode payload; source/destination SHA03f072c3c1de30d1bc58a760067da35d6cc4b559b8370f0f0fd1aa43fc924b12. | Actual Unicode/multiple/tree copy hashes and time preservation, Qt copy/cut/paste pass; no final P1 native copy replay. | Actual multiple/tree/mixed copy hashes; B-P1-UI-006 native same SHA as original; cancellation/cleanup ownership passes. | Original/B share one native payload result. A automated correctness on owned fixtures does not establish a same-payload desktop comparison. References: ORIG, A-OP, A-MGMT, B-NATIVE1, B-OP |
| FR-12 Move/conflicts | No original runtime move/collision; OBS-022 is nonconflicting copy. | Initial preflight refusal, Skip/confirmed Replace files, actual moves and preservation tests pass. | Mixed per-item move/conflict, KeepBoth retry and race-created destination preservation pass. | Source-derived difference: A Skip/Replace/Cancel before initial mutation; B may finish nonconflicting items then retry conflicts as KeepBoth. Original collision equivalence unknown. References: ORIG, A-OP, A-MGMT, A-SOURCE-UI, A-SOURCE-OPS, B-OP, B-SOURCE-UI |
| FR-13 Recycle or safe delete | No original destructive/recycle runtime action. | Normal recycle requested without destructive fallback; native recycle unverified. Confirmed permanent owned deletion/cancel tested. | Separate actual Windows opt-in owned recycle pass; default recycle skipped; no permanent fallback. Desktop confirmation/restore not fully verified. | Both provide safe routes. Actual exercised boundaries differ: A confirmed permanent deletion versus B recycle. Original outcome equivalence unknown. References: ORIG, A-REPORT, A-MGMT, A-OP, B-P1, B-RECYCLE, B-OP |
| FR-14 Single file/folder rename | OBS-021 F2 inline rename to 이름변경-ui.txt preserves SHA03f072c3c1de30d1bc58a760067da35d6cc4b559b8370f0f0fd1aa43fc924b12. | Real adapter/QInputDialog success and invalid/occupied/missing-source preservation; no final P1 desktop replay. | B-P1-UI-005 native F2 modal rename to same name/SHA; Qt file/folder Unicode and occupied-folder retry preserve nested hashes. | Original/B share a native filename/content result. Both A/B modal entry differs from original inline editor; original failure/permission/locking behavior unknown. References: ORIG, A-OP, A-MGMT, A-SOURCE-UI, B-NATIVE1, B-QTUI, B-SOURCE-UI |
| FR-15 New folder | OBS-020/021 create native folders and inline editing; repeated transient clicks create two defaults including New Folder(2). | Validated single-leaf QInputDialog, actual mkdir and duplicate refusal pass. | B-P1-UI-003/004 Ctrl+Shift+N modal creates Unicode folder; Qt duplicate Conflict passes. | A/B modal name-before-create differs from original default-create then inline editing/automatic naming. Core creation supported, UX/collision policy differs. References: ORIG, A-MGMT, A-OP, A-SOURCE-UI, B-NATIVE1, B-QTUI, B-OP, B-SOURCE-UI |
| FR-16 Ctrl/Shift multi-select batches | No original complete Ctrl/Shift batch transcript. | Actual Qt Ctrl/Shift selection, shared views and batch copy/move pass. | Actual Qt Ctrl-click/Shift-click and multi-copy/cut/paste hashes pass. | Implemented/tested both; native desktop full selection/clipboard interoperability and original equivalence remain partial. References: A-MGMT, A-OP, B-QTUI, B-OP |
| FR-17 Internal and OS-app drag/drop | No original drag/drop transfer recorded. | Qt local-URL Copy/Move, internal folder Move and outgoing MIME pass; native external gesture unverified. | Actual production Qt Drop Copy/Move and refusal/acknowledgment tests, plus native in-window Copy, are retained. Postfreeze user-assisted B-to-Windows-Explorer outbound Copy has retained source/destination25-byte SHA-2566afc64f7a71d8276c4549cb9428703bbbdc9e5477a192706ad392252840bfbb1. Windows-Explorer-to-B inbound Copy has retained source/destination26-byte SHA-25625e689280b19bcdec01bd1ea22e75a38a3cd908f25dc9b9613a07a4a0a92b8de. Both baseline-matching Copy directions are supported; gestures/modifiers were not automatically captured. | Current B core drag/drop route is corroborated by actual Qt Move, native internal Copy and user-assisted native Copy endpoints in both OS-app directions. A native external interoperability remains unverified. The exact FR-17 text is 드래그 앤 드롭 / 내부 및 OS 애플리케이션 간 이동 (requirements.md:203); it does not enumerate every modifier/native Move variant as a separate mandatory acceptance test. Therefore missing native Move/modifier variants are remaining coverage limits, not a reason to deny the now-supported B core route. Source-removing native Move, conflicts, general modifiers and original drag/drop equivalence remain unproved; normal Copy does not establish a Shift bug. Initial19 core-route inventory is preserved separately from postfreeze B corroboration. References: A-MGMT, A-REPORT, B-QTUI, B-NATIVE1, B-P1, B-DRAG-ATTEMPT, B-USER-COPY, B-USER-INBOUND |
| FR-18 Icons/List/Details | Original Details observed; three-mode switching/selection not directly recorded. | Actual Qt shared model/selection in three modes passes; final render Details. | B-P1-UI-001/002 native Icons/List retain six names; Qt three modes/per-tab selection pass. | Both implement modes; B has additional native captures. Original three-mode equivalence unknown; icons do not establish thumbnails. References: ORIG, A-MGMT, A-IMAGE, B-NATIVE1, B-QTUI |
| FR-19 Navigation/management shortcuts | Original Alt+Up/Left/Right, Ctrl+T/Tab/W, F2 and Ctrl+C/V observed; not every focus mapping. | Qt navigation/tab, Ctrl+A/C/X/V, F2, Delete, Shift+Delete, Ctrl+Shift+N and text-edit focus routes tested. | Qt core routes plus native Ctrl+Shift+N, F2 and Ctrl+C/Paste observed. | Common routes supported; no universal original keyboard/focus-policy equivalence. References: ORIG, A-QTUI, A-MGMT, A-REPORT, B-QTUI, B-NATIVE1 |
| FR-20 Count/selection/progress status | Original empty0, bulk1200 and Unicode6 status plus copy completion; long progress not measured. | Folder/filter/selection bytes, asynchronous progress/shutdown/mixed-result tests; final render six items/376 bytes. | Model/selection/item/byte/session status, native Copy1 succeeded and completion/conflict Qt tests. | Core status supported; no matched long-duration displayed progress original comparison. Wording/global job policies differ. References: ORIG, A-QTUI, A-OP, A-IMAGE, B-QTUI, B-SESSION, B-NATIVE1 |

## Fourteen mandatory scenarios

The six result-supported rows establish only their named endpoint, combining original native observations with identified A/B native or production Qt/adapter tests. No uniform three-app replay exists. T-06/T-08 and original-unverified rows are excluded from the count.

| ID | Scenario | Original | A | B | Adjudication / remaining boundary |
|---|---|---|---|---|
| T-01 | Empty folder | OBS-002-settled: empty path,0 items/0 bytes. | Actual emptyFolder and GUI zero rows. | Actual emptyFolderIsComplete and keyboard address to empty folder. | shared_narrow_postcondition_supported: Same narrow endpoint: known empty fixture yields zero rows. No uniform three-app replay or status wording equivalence. References: ORIG, A-PLATFORM, A-QTUI, B-PLATFORM, B-QTUI |
| T-02 | 1000+ entries | OBS-007:1200 items/939KB; UIA names truncated. | bulkComplete exact1200 facts; UI heartbeat test uses controlled facts, real benchmark separately retained. | Exact1200 model names/metadata and manifest order; real profiling separately retained. | shared_narrow_postcondition_supported: Same narrow endpoint: bulk fixture yields1200 entries. Original full1200 names/metadata and fair responsiveness comparison absent. References: ORIG, A-PLATFORM, A-QTUI, B-PLATFORM, B-QTUI |
| T-03 | Unicode/special names | OBS-005/008/011: six exact fixture names. | Real Unicode metadata tests, historical A-UI-001 and final six-name render. | Manifest six-name tests, B-UI-001 and final six-entry snapshots. | shared_narrow_postcondition_supported: Same narrow endpoint: six fixture names/count retained. Long row may elide visually; all normalization/long-path/font cases unverified. References: ORIG, A-PLATFORM, A-UI, A-IMAGE, B-PLATFORM, B-NATIVE0, B-PKG1-MODEL |
| T-04 | Nonexistent path | OBS-012 retains committed Unicode listing/input; no dialog in queried windows. | Real missing-path error; A-UI-003/004 explicit banner and F5 recovery. | Invalid/missing-path tests and Qt listing/history/input preservation. | shared_narrow_postcondition_supported: Same narrow endpoint: invalid address does not replace committed list. Feedback differs: A/B show errors; original audio/long-term feedback unknown. References: ORIG, A-UI, A-PLATFORM, A-QTUI, B-PLATFORM, B-QTUI |
| T-05 | Permission denied | No actual original denied-folder attempt retained. | Windows deniedRealAcl passes; Linux ACL fixture skipped. | Windows realDeniedSourceGateRemainsUnread passes; Windows-only ACL case skipped on Linux. | original_side_unverified: Both handle actual Windows denial; original comparison impossible. No original runtime case or complete Linux/macOS equivalent. References: ORIG, A-PLATFORM, B-PLATFORM, B-P1 |
| T-06 | Back/Forward history | OBS-003-settled/016/017 specific paths and forward state. | Core backForwardUpRefresh controlled adapter and Qt navigation/tab tests pass. | Actual Qt navigationButtonsBackForwardAndUp and session branch/failure pass. | partial_comparison: Compatible history semantics with different test sequences/modalities. No identical three-app route/history transcript; excluded from narrow matched count. References: ORIG, A-CORE, A-QTUI, B-QTUI, B-SESSION |
| T-07 | Independent tabs | OBS-004/006/018/019 retain separate folders and close one. | A-UI-005/006 native create/close retains Unicode; actual Qt empty/Unicode tabs. | Qt empty/Unicode independent tabs, invalid input preservation and inactive-tab close. | shared_narrow_postcondition_supported: Same narrow endpoint: second-tab actions preserve another committed folder. Original full tab histories/filter/sort/selection not observed. References: ORIG, A-UI, A-CORE, A-QTUI, B-QTUI, B-SESSION |
| T-08 | Name/size/date sorting | OBS-009 size ascending,010-settled date ascending,011 name order. | Typed sorting asserts size endpoints/date ordering/reversal; final image name order. | Complete manifest permutations on bulk/folder fixtures; final image name order. | partial_comparison: Partial compatible order evidence; no full original three-key oracle. Original complete permutations/ties/folder/locale rules absent; excluded from narrow count. References: ORIG, A-QTUI, A-IMAGE, B-QTUI, B-DOMAIN, B-IMAGE |
| T-09 | Search accuracy | OBS-013: exact two names for 한글; no child folders. | Actual production Qt exact same two names and clear to six. | Native B-UI-003 and Qt same two names,zero/clear to six. | shared_narrow_postcondition_supported: Same narrow endpoint: identical query/current fixture returns exact two names. Dialog/recursion/case policy differs; result does not establish policy equivalence. References: ORIG, A-QTUI, B-QTUI, B-NATIVE0 |
| T-10 | Copy/move duplicate conflicts | No original collision/move; OBS-022 only nonconflicting copy. | Initial refusal,Skip/confirmed Replace and concurrent source/destination preservation tested. | Mixed outcomes,KeepBoth and race-created destination preservation tested. | original_side_unverified: Different implemented safety policies; original outcome unknown. No original conflict confirmation/result; actual cross-volume move unverified both. References: ORIG, A-OP, A-MGMT, B-OP, B-QTUI |
| T-11 | External deletion | No original displayed-file external deletion/open case. | Actual adapter externalDeletion and source deletion between batch items; no named full displayed-file Qt recovery sequence. | Actual displayed file remove,Return gives NotFound/banner,F5 restores exact empty listing on Windows/Linux. | original_side_unverified: B has stronger retained actual GUI recovery boundary; original unknown. Do not equate A adapter case to B full GUI recovery; no original transcript. References: ORIG, A-PLATFORM, A-OP, B-PLATFORM, B-QTUI, B-LINUX-UI |
| T-12 | Rename failure | OBS-021 successful file rename only. | Invalid/occupied/missing source preserve data; normal GUI rename passes. | Qt occupied-folder Conflict preserves nested hashes then Unicode retry succeeds; adapter invalid/file conflicts too. | original_side_unverified: Both failure boundaries tested; B includes full folder collision UI/retry. Original failure/permission/locking UX unobserved. References: ORIG, A-OP, A-MGMT, B-OP, B-QTUI |
| T-13 | Symlink/cycle | No original link/cycle runtime case. | Windows two privilege skips; Linux actual cycle facts/no traversal and mutation refusal. | Windows three ordinary-link skips but actual FSCTL junction/nested cycle refusal; Linux no-follow passes. | original_side_unverified: Distinct actual coverage; skips are not passes. Other reparse/platform cases and original policy unverified. References: ORIG, A-PLATFORM, A-OP, A-LINUX-PLATFORM, A-LINUX-OP, B-PLATFORM, B-OP, B-P1 |
| T-14 | Exit/restart | No original app close/restart; tab close is not app restart. | Pending browse/worker shutdown and two offscreen windows; native historical normal close settles no own windows. Final --render exits0. | Actual production-copy Qt close/cancel/drain and fresh MainWindow same process; two separate final package processes snapshot six/exit0. | original_side_unverified: Bounded lifecycles supported; original restart comparison unknown. No crash recovery or operation-time OS restart. Windows copy-close observed0 copied bytes,Linux262144; separate actual byte-cancellation test remains distinct. References: ORIG, A-CORE, A-QTUI, A-OP, A-UI, A-RENDER, B-QTUI, B-PKG1, B-PKG2, B-PKG1-MODEL, B-PKG2-MODEL |

## Policy differences and native evidence

Source-derived frozen A/B policies differ: A offers Skip all / Replace files / Cancel after initial transfer preflight; B can complete nonconflicting items and retries the conflict subset as KeepBoth. Both preserve originals in their named race/cancellation tests. Original collision execution is absent, so neither policy is called original-equivalent. References: A-SOURCE-UI, A-SOURCE-OPS, B-SOURCE-UI; actual assertions: A-OP, A-MGMT, B-OP, B-QTUI.

Both A/B use QInputDialog for new folder and rename. Original native OBS-020/021 creates a default folder and starts inline editing; collision naming is observed. Original Search is a separate dialog with Search Subfolders checked and Case Insensitive unchecked, whereas A/B provide inline case-insensitive current-folder filters. Exact two-name results in the observed fixture do not establish equal recursive/case policy.

Original/B native rename and copy share resulting filename/content evidence, SHA-256 `03f072c3c1de30d1bc58a760067da35d6cc4b559b8370f0f0fd1aa43fc924b12`. A final P1 management is supported by Qt/adapter execution; final P1 desktop action replay is absent. A native open/tab/error/normal-close observations belong to a historical P0 package with SHA-256 `8845ddb1213b62c3b36a0d4dba6a8249516ff41b3adf397f4c334d948b93cd98`, distinct from final A package reported in A-REPORT.

The earlier automated root native drag attempt established no transfer. Target activation, z-order and routing were unknown; both owned file hashes remained unchanged. This historical attempt is retained and is not a proved product failure. Reference: B-DRAG-ATTEMPT.

A later user-assisted B-to-Windows-Explorer Copy result is supported by the user reporting manual copy after the requested Ctrl-drag and root filesystem verification: `app-source/app-target/outbound.txt` and `os-source/outbound.txt` are simultaneously present, each25 bytes, each SHA-256 `6afc64f7a71d8276c4549cb9428703bbbdc9e5477a192706ad392252840bfbb1`, matching the outbound baseline. The gesture itself was not automatically captured. The earlier top-level source is now inside `app-target`; the intermediate movement was not captured, so its gesture/mode is unknown. That record establishes the outbound Copy endpoint. The later inbound record below establishes the reverse Copy endpoint; historical scope wording is retained only in its original record. Reference: B-USER-COPY.

The user subsequently clarified that inbound.txt had already been placed in B. Root verified `os-source/inbound.txt` and `app-source/app-target/inbound.txt` simultaneously present, each26 bytes with baseline SHA-256 `25e689280b19bcdec01bd1ea22e75a38a3cd908f25dc9b9613a07a4a0a92b8de`; the destination had previously been absent. Thus native Copy endpoint evidence exists in both directions. Gesture/modifier details were not automatically captured. A source-removing native Move is not proved, the user chose to leave that variant unverified, and ordinary Copy does not establish a Shift bug. Reference: B-USER-INBOUND.

The exact FR-17 wording is “드래그 앤 드롭 / 내부 및 OS 애플리케이션 간 이동” ([requirements.md:203](../requirements.md#L203)). It does not list every modifier/native Move variant as an independent mandatory test. Current B core-route evidence combines actual Qt Move/drop, native internal Copy and both native external Copy directions. Remaining native Move/conflict/modifier coverage limits prevent a holistic fidelity claim; they do not erase the established core routes. Initial19-route inventories remain historical and later corroboration receives no retroactive initial REA credit.

## Visual reference and package boundaries

Actual host Windows11 is the visual reference. A shows dark tabs, a plain full-path field, text commands, Home/This PC sidebar and large details table; six entries and376 bytes. B shows breadcrumbs, compact icon commands, known folders, compact rows and a right details pane, closer to the observed reference structure. Both elide long names. Capture mode/dimensions and localization/date/size formatting differ; no pixel or usability score is calculated.

Root A final postfreeze --render returned0 in 0.670146s, with 0.515625s family CPU and 42,680,320bytes peak family committed memory;0 active owned processes remained. This establishes render/exit only, not interaction replay or comparative performance. B final initial/restart records are separate package processes, each with a six-entry snapshot and exit0. They do not establish crash recovery or OS restart during an operation.

## Verification inventory and limits

A Windows51 functional passes/2skips/0failures and Linux50/3/0 exclude init/cleanup. B Windows70/4/0 and Linux69/5/0 include10 init/cleanup each, yielding60 and59 functional passes. Different test designs, data rows and fixtures make these inventories unsuitable as feature scores. B default recycle skip is separate from its actual Windows owned-recycle opt-in pass. A native recycle remains unverified; confirmed owned permanent deletion is tested. Windows link privilege skips, B actual junction/cycle passes and Linux actual cycles retain their distinct scopes.

No macOS execution, full native external drag equivalence, original permission/conflict/external-deletion/rename-failure/cycle/restart results or full14-scenario original comparison is claimed. Test totals, native evidence breadth and visual structure do not isolate a causal REA benefit because independent design and validation scope differ.

## Evidence reference ledger

Hashes cover complete retained files. Observation IDs select exact JSON records; actual logs remain authoritative for executed test boundaries.

**REQ** — [docs/requirements.md](../../docs/requirements.md)

FixedFR01-20 and14mandatory scenarios; Win11 UI reference
SHA-256 `76b659ec0594607e02b1818683ccc826a7af2c8dff6583888852d351e3890c88` (20875bytes).

**ORIG** — [docs/observations/original-runtime.json](../../docs/observations/original-runtime.json)

Original native actions/accessibility;25records, exact OBS IDs; transient snapshots require settled successors
SHA-256 `20417984a2960b463c8f2e867513aa28a50768d91b2af0fb9284c272f9c529a6` (116668bytes).

**ORIG-SUM** — [docs/observations/behavior-summary.md](../../docs/observations/behavior-summary.md)

Original narrative and actual Win11 design reference; laterOBS020-022 remain in runtimeJSON
SHA-256 `2add54061de4571dbc574bbea1ce76ac26253bcf302f3d9e0354f4473c0070ac` (5067bytes).

**A-REPORT** — [experiments/A/docs/evaluation/p1-report.md](../../experiments/A/docs/evaluation/p1-report.md)

Frozen A report/limits/final tests
SHA-256 `fc73c31ee49eb9f365859f71b41782a61b3813754391ceaa740ae70bd308fbf7` (12141bytes).

**A-P0REPORT** — [experiments/A/docs/evaluation/p0-report.md](../../experiments/A/docs/evaluation/p0-report.md)

Historical P0 report and native checks
SHA-256 `986504f5d6f8127a95ae1f4b0ffa9a3c9725f2a08592941b1f647476fbcc8f49` (8359bytes).

**A-SUM** — [experiments/A/docs/evaluation/p1-test-summary.json](../../experiments/A/docs/evaluation/p1-test-summary.json)

A final per-suite functional counts excluding init/cleanup
SHA-256 `d3cf072a382ce1018f0c0d8b728684354f2dcd50b48f90723ff960aabf240692` (3221bytes).

**A-UI** — [experiments/A/docs/evaluation/p0-direct-ui.json](../../experiments/A/docs/evaluation/p0-direct-ui.json)

Historical actual native P0 UI; packageSHA8845ddb1213b62c3b36a0d4dba6a8249516ff41b3adf397f4c334d948b93cd98; not finalP1 replay
SHA-256 `e5e50fad265ad3657953d4bf814c1b02f361fc691a305f8b0061449416e5b8a1` (43100bytes).

**A-RENDER** — [docs/evaluation/measurements/a-phase6-render.metrics.json](../../docs/evaluation/measurements/a-phase6-render.metrics.json)

Root postfreeze final packaged --render; exit/resource evidence, not interaction replay
SHA-256 `72b0b742429889c57dab6831d80eb52cc7d72de9933540275bb98efc4982cf2c` (1001bytes).

**A-IMAGE** — [docs/observations/screenshots/a-frozen-unicode.png](../../docs/observations/screenshots/a-frozen-unicode.png)

Visually inspected final A hidden native-platform QWidget render;6entries
SHA-256 `a05818439d4799e62665f3f562bd636af1a00c1eadbeaf1eb4a8b9355913cc31` (27907bytes).

**WIN11** — [docs/observations/screenshots/windows11-reference.png](../../docs/observations/screenshots/windows11-reference.png)

Visually inspected actual hostWin11; visual reference only
SHA-256 `953d45474b9b46f4eb80243f79a1c71ce673205c11770c56bd10bcf6c78a03df` (85292bytes).

**B-IMAGE** — [docs/observations/screenshots/rea-win11-aligned-final.png](../../docs/observations/screenshots/rea-win11-aligned-final.png)

Visually inspected final B design screenshot
SHA-256 `0f76b87fadd04b49e49e1ca770f4b9b34da8b9e6874cb05b16040997ec49b4f5` (37988bytes).

**B-MATRIX** — [docs/evaluation/requirements-matrix.md](../../docs/evaluation/requirements-matrix.md)

Frozen B FR ledger and limits
SHA-256 `3da21d1fab7708505cd2d524f5956ed31f07acae04f43f35ca3f332402d925f1` (57607bytes).

**B-P1** — [docs/evaluation/p1-validation.md](../../docs/evaluation/p1-validation.md)

Frozen B executed P1 checks, native/ext4/DrvFS distinctions
SHA-256 `a2223f44fd1311845c855c7a6802cc884a0076725a0598bcef537645b0abb090` (31040bytes).

**B-NATIVE0** — [docs/observations/rea-p0-runtime.json](../../docs/observations/rea-p0-runtime.json)

Actual B P0 desktop records
SHA-256 `8beb1a13f1a1de667b8f21b977a7d5f9736723fd663ca88177b1d9b1d75cc0a9` (50730bytes).

**B-NATIVE1** — [docs/observations/rea-p1-runtime.json](../../docs/observations/rea-p1-runtime.json)

Actual B P1 desktop records
SHA-256 `5d4c0910a9d7eda64e3b2f55c289ac3d850e9ab03345dcb7ac3353165512706a` (55467bytes).

**B-CTEST** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/test.stdout.log](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/test.stdout.log)

Actual final CTest5/5; unlike features, suite totals not comparable scores
SHA-256 `2a587582d7ed5811d83dfd1568137a8f938ae99f054c1b0f413a61c1b50a7512` (582bytes).

**B-LINUX** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/linux-ctest.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/linux-ctest.txt)

Actual native-ext4Linux5/5; not DrvFS success
SHA-256 `cc2c35206aa43f3922dab5b105007ce796e23dc50fa15742f7ac94c633c574e9` (3402bytes).

**B-LINUX-UI** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/linux-native-ui.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/linux-native-ui.txt)

Linux23QtUIpasses including production-copy close/fresh-window
SHA-256 `0c4b5a76d9ab0cdc97c94c7d43338e82ac265589e730fd8381e4139bf0b7b534` (5792bytes).

**B-RECYCLE** — [docs/evaluation/measurements/b-owned-recycle.metrics.json](../../docs/evaluation/measurements/b-owned-recycle.metrics.json)

Windows owned recycle opt-in command/exit; functional assertion in P1 ledger
SHA-256 `254a3e05dda8b61b2f51542c95f24da9ca00266d9fc4982692a30606201aa41c` (917bytes).

**B-FREEZE** — [docs/evaluation/initial-freeze-manifest.json](../../docs/evaluation/initial-freeze-manifest.json)

Initial freeze authority; new report is successor
SHA-256 `a93289e08db7e7338110480063716bd2be434ffd1e76585e0bc36a2b996cf729` (71090bytes).

**FREEZE-VERIFY** — [verification/phase6-freeze-verification.json](../../verification/phase6-freeze-verification.json)

Root verified354B/124A files before source access; worker did not rerun whole manifest
SHA-256 `b71d70a0820290afac0bcac0d91096a49623d89a538309e0afd6c945a1a71328` (488bytes).

**A-CORE** — [experiments/A/artifacts/initial-freeze/test-reports-windows/core.txt](../../experiments/A/artifacts/initial-freeze/test-reports-windows/core.txt)

Frozen actual Windows QtTest named PASS/SKIP boundaries
SHA-256 `d3a36b753022d562a27c1d352406911bb979b447316e942a051b26768692ce23` (782bytes).

**A-PLATFORM** — [experiments/A/artifacts/initial-freeze/test-reports-windows/platform.txt](../../experiments/A/artifacts/initial-freeze/test-reports-windows/platform.txt)

Frozen actual Windows QtTest named PASS/SKIP boundaries
SHA-256 `e54690440150a08d2d1d2f48921243dca136606dc0a8f7e57ab8089f489b81dd` (941bytes).

**A-QTUI** — [experiments/A/artifacts/initial-freeze/test-reports-windows/ui.txt](../../experiments/A/artifacts/initial-freeze/test-reports-windows/ui.txt)

Frozen actual Windows QtTest named PASS/SKIP boundaries
SHA-256 `34108d2dbbbfa4998ad11dbc66bab426273f20bcaa1c1c1b7c682e7563fd0300` (3258bytes).

**A-OP** — [experiments/A/artifacts/initial-freeze/test-reports-windows/operations.txt](../../experiments/A/artifacts/initial-freeze/test-reports-windows/operations.txt)

Frozen actual Windows QtTest named PASS/SKIP boundaries
SHA-256 `426e646ec68c2eb427c694e3318eaa2a321d9cd90137824c6b111f27f354151c` (1543bytes).

**A-MGMT** — [experiments/A/artifacts/initial-freeze/test-reports-windows/management_ui.txt](../../experiments/A/artifacts/initial-freeze/test-reports-windows/management_ui.txt)

Frozen actual Windows QtTest named PASS/SKIP boundaries
SHA-256 `5486eeb6f75d73dd9ef9a23eabcf287cd22dab15fddc7937432f5d3594feea98` (3153bytes).

**A-LINUX-PLATFORM** — [experiments/A/artifacts/initial-freeze/test-reports-linux/platform.txt](../../experiments/A/artifacts/initial-freeze/test-reports-linux/platform.txt)

Actual Linux cycle/permission/platform-specific boundaries
SHA-256 `ad249ac7a49e2526b585e8b79faa6832beca196b436d43f13af40d3d4e2df9ae` (938bytes).

**A-LINUX-OP** — [experiments/A/artifacts/initial-freeze/test-reports-linux/operations.txt](../../experiments/A/artifacts/initial-freeze/test-reports-linux/operations.txt)

Actual Linux cycle/permission/platform-specific boundaries
SHA-256 `795e08d2ab025f07ffff8f3234e3ab90018ebaf7664198bc3c1f3f34033a1512` (1778bytes).

**B-DOMAIN** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-domain.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-domain.txt)

Actual initial-final Windows QtTest; totals include init/cleanup; operation text uses legacyCP949
SHA-256 `a78697a5b4b418867c76a7330f1c8bdc97251e30e1f1cfb38ff648b440d13845` (818bytes).

**B-SESSION** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-session.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-session.txt)

Actual initial-final Windows QtTest; totals include init/cleanup; operation text uses legacyCP949
SHA-256 `73abc73d5b73e2bb370b81742ac798d902f3caa386dda12167b23107e2821b04` (1278bytes).

**B-PLATFORM** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-platform.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-platform.txt)

Actual initial-final Windows QtTest; totals include init/cleanup; operation text uses legacyCP949
SHA-256 `dba0736f08f9b957cc774e0eeb3820ff513a3113736eb3138f33a05aaf800ce6` (1311bytes).

**B-QTUI** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-ui.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-ui.txt)

Actual initial-final Windows QtTest; totals include init/cleanup; operation text uses legacyCP949
SHA-256 `9e4019f6162505afd49c21fb6acd33f94354d1bdd438430ca70f38152e73dd5a` (6097bytes).

**B-OP** — [docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-operation.txt](../../docs/evaluation/validation-logs/b-initial-final-20261022T090136/windows-operation.txt)

Actual initial-final Windows QtTest; totals include init/cleanup; operation text uses legacyCP949
SHA-256 `41b20aee0641351fe5ad72ed42f37dd6f3f80e0f1920150e68167e4a5f483583` (2890bytes).

**B-PKG1** — [docs/evaluation/measurements/b-final-package-initial.metrics.json](../../docs/evaluation/measurements/b-final-package-initial.metrics.json)

Actual separate final packaged render/snapshot/quit process
SHA-256 `5bdaa885aaac805c814972589b5c404ad6463b7d518d1ef091346544b7686938` (1175bytes).

**B-PKG1-MODEL** — [docs/evaluation/measurements/b-final-package-initial-model.json](../../docs/evaluation/measurements/b-final-package-initial-model.json)

Actual6-entry final packaged model snapshot
SHA-256 `f0cc9516e0de88a862c658e4783572d006ec01d82cd1fc55b675dd4c5afbd5b5` (2892bytes).

**B-PKG2** — [docs/evaluation/measurements/b-final-package-restart.metrics.json](../../docs/evaluation/measurements/b-final-package-restart.metrics.json)

Actual separate final packaged render/snapshot/quit process
SHA-256 `8eec9a488f6364e98253abdaf390acdcb6a7cd09855f69ce07c46ae85bc0ed3f` (1179bytes).

**B-PKG2-MODEL** — [docs/evaluation/measurements/b-final-package-restart-model.json](../../docs/evaluation/measurements/b-final-package-restart-model.json)

Actual6-entry final packaged model snapshot
SHA-256 `f0cc9516e0de88a862c658e4783572d006ec01d82cd1fc55b675dd4c5afbd5b5` (2892bytes).

**A-SOURCE-UI** — [experiments/A/artifacts/initial-freeze/src/ui/main_window.cpp](../../experiments/A/artifacts/initial-freeze/src/ui/main_window.cpp)

Source-derived frozen A/B policy only, no new execution or original-source read Locator: 629-648 modal mkdir/rename;724-747 Skip/Replace/Cancel
SHA-256 `59c3d73b35d915fdceb115795a6aa9a165d7c4a87a9bec5c036fd6caeaa30b35` (36994bytes).

**A-SOURCE-OPS** — [experiments/A/artifacts/initial-freeze/src/platform/file_operations.cpp](../../experiments/A/artifacts/initial-freeze/src/platform/file_operations.cpp)

Source-derived frozen A/B policy only, no new execution or original-source read Locator: 256-258 preflight;483-505 replacement staging
SHA-256 `51704b344d86e2bee61b08acba482d2666501de31c7f15108d2b0d2d7b433fd7` (27958bytes).

**B-SOURCE-UI** — [src/ui/main_window.cpp](../../src/ui/main_window.cpp)

Source-derived frozen A/B policy only, no new execution or original-source read Locator: 705-718 conflict subset KeepBoth;1169-1200 modal rename/mkdir
SHA-256 `7845db3e0f538c031ef10d5468573fd7f7d33edd9637baad1db79cce5f390e3d` (65805bytes).

**A-SOURCE-SEARCH** — [experiments/A/artifacts/initial-freeze/src/ui/file_model.cpp](../../experiments/A/artifacts/initial-freeze/src/ui/file_model.cpp)

Source-derived frozen A/B policy only, no new execution or original-source read Locator: 104-106 case-insensitive contains
SHA-256 `c0a446a263131ecb7793fa6ec04558d9b4014e041463949ed0c7a994251d8f9f` (4596bytes).

**B-SOURCE-SEARCH** — [src/core/domain.cpp](../../src/core/domain.cpp)

Source-derived frozen A/B policy only, no new execution or original-source read Locator: 90-96 case-folded literal filtering
SHA-256 `b426f3097722b251b5c59a9c456ab8d87699051309e6ef7df00abaed8e26ff7a` (3621bytes).

**B-DRAG-ATTEMPT** — [docs/observations/phase6-native-drag-attempt.json](../../docs/observations/phase6-native-drag-attempt.json)

Later root postfreeze native attempt. No transfer established; target activation/z-order/gesture routing not independently established; not product failure.
SHA-256 `82475e21d29a02b34d1e76aedf2667f28b3e62496ef971c555b5867fa09f42e2` (1506bytes).

**B-USER-COPY** — [docs/observations/phase6-user-assisted-native-copy.json](../../docs/observations/phase6-user-assisted-native-copy.json)

Postfreeze user-assisted B-to-Windows-Explorer Copy result. User reported manual copy after requested gesture; root verified both actual files and baseline hashes. Gesture itself was not automatically captured; intermediate source-path movement unknown.
SHA-256 `6328c1dcebf858f1b42e535b2dc9d84049fad0c78cf3304c43f8a8d2df253092` (1889bytes).

**B-USER-INBOUND** — [docs/observations/phase6-user-assisted-native-inbound.json](../../docs/observations/phase6-user-assisted-native-inbound.json)

Postfreeze user-assisted Windows-Explorer-to-B Copy endpoint. User clarified prior placement; root verified both26-byte files against baseline and prior absence. No automatic gesture/modifier capture; no source-removing native Move inference.
SHA-256 `d3406da989403b582b412aa06d8e317a71550ce559a5e5cef8f199e5b3ae100e` (1754bytes).

Documentary validation passed: fixed20 FR rows and14 scenarios,6/2/6 count derivation, all row references,45 current input hashes, normalized counts and initial/current FR-17 evidence separation. No new product verification was executed.

Updated after both user-assisted Copy directions: all45 input hashes reverified; fixed20 requirements,14 scenarios and6 narrow/2 partial/6 original-unverified counts remain unchanged. Initial A/B19-route inventories are preserved; B postfreeze FR-17 core-route corroboration is separately recorded. No new UI or product test was performed by this audit.
