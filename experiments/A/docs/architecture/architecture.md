# Experiment A: independent architecture baseline

Status: Phase 3 proposed/frozen for implementation gate, 2026-10-21. No application implementation exists at this point. Scope: FR-01–FR-20, all fourteen mandatory scenarios, Qt/C++20 Windows build, Linux build/run tests, macOS source review, modern Windows 11 dark layout. REA is intentionally excluded from arm A. P2 dark theme is planned; other optional features are reported separately.

## Input and evidence boundary

Allowed inputs are the copied requirements, pure runtime observations, actual Windows 11 screenshot, environment facts, official Qt/Microsoft/C++ documentation, the approved executable and SDK, immutable fixture, and shared build helpers. No original source, B design/code, REA skill/output, or source search is used. Input SHA-256 snapshot is ../evaluation/input-snapshot.json. A and B separation is an instruction/fresh-context boundary; the same Windows account does not enforce filesystem isolation. The source verification directory is denied and is never enumerated.

Behavior knowledge is observational rather than internal: successful folder enumeration; empty/bulk/Unicode listing; Name/Size/Modified columns; back/forward/up; tab paths independent; new tab starts at This PC; filename search finds two Unicode names; default app opens café.txt. Settled OBS-015–017 override older unresolved probes in behavior-summary.md. No native implementation/call graph is inferred. Invalid-path behavior in the observation retained the prior list; our implementation additionally provides a recoverable inline error as required by FR-10.

## Modules and direction

Presentation -> Application -> Domain and Platform contract. Platform implementations depend on the contracts and domain values. Domain has Qt Core value types but no Widgets, Win32 or GUI ownership. Application owns navigation transactions and jobs. UI renders committed state and submits commands; it does not enumerate directories or perform mutations. Adapters alone contain platform calls.

- include/explorer/types.h: paths/locations, file facts, tagged errors, Result, operation requests/progress.
- include/explorer/platform.h: synchronous, worker-safe adapter contract with capability reporting.
- src/core/: navigation history, tab state, search/sort rules and operation validation.
- src/application/: async browse controller and operation manager, cancellation and stale-result handling.
- src/platform/: common filesystem implementation plus Windows and Unix-specific error/launch/recycle boundaries.
- src/ui/: main window, tab browser, directory table/list model, tree navigation, drag/drop and dialogs.
- tests/: core fake-adapter semantics, real filesystem adapter integration, offscreen Widgets workflows; fixture mutation uses disposable A-owned copies.

A compact library/application split is preferred to a generic plugin/service framework. Public contracts have one owner and are documented here; tests consume them without UI assumptions.

## Domain and application contracts

`Location` distinguishes This PC from an absolute directory path. Display labels are never lookup values. Paths retain Unicode/source spelling; absolute/cleaned lexical path is separate from resolved identity. Canonical resolution failure is an explicit error, not an absent path. No automatic URL decoding; drag/drop local file URLs are parsed as URLs before extracting local paths.

`FileEntry` owns absolute path, name, directory/link flags, size when meaningful, modified timestamp, and type label. A directory's size is unknown rather than zero content size. `Listing` owns the requested/resolved location, entries, and any warnings. Link targets may be shown but are not recursively followed by default. A single enumeration owns metadata interpretation; UI/model do not stat names again to reconstruct facts.

`Error` is tagged (NotFound, AccessDenied, NotDirectory, AlreadyExists, InvalidName, Conflict, Unsupported, Cancelled, Io) with action, source/destination paths, native code when available, and human detail. `Result<T>` represents success or one error; batch work additionally returns per-item successes/failures and partial completion. Unsupported capabilities never silently succeed.

`Platform` provides capabilities, roots, list(location, cancellation), openFile(path), inspect/validate and mutation operations. Calls return domain Results. Windows directory enumeration must retain actual access-denied and not-found reasons rather than translate an empty QDir result into an empty successful folder. Default app launch is an adapter action; no shell command interpolation. Recycle is queried and uses Qt/native trash support where available; there is no automatic fallback to permanent deletion.

Each tab owns location, committed listing, history cursor, filter/search settings, sort state, view mode and selected paths. Initial/new tabs start at This PC (Windows drives; filesystem roots elsewhere). Closing the last tab leaves one This PC tab. History commits only after successful directory resolution/listing. Back/Forward prepare a candidate cursor and commit it on success; failure keeps prior path/list/cursor. A new successful visit after Back truncates Forward. Refresh does not append history. Up resolves a parent or This PC at a root.

Browse/search requests carry tab identity and increasing generation. Worker results are accepted only while both still match. Cancel/close invalidates generation; late completion cannot repopulate a closed tab. Workers use a bounded QThreadPool and value results; no Widgets calls. A worker can finish during shutdown; its cancellation state and adapter outlive it, and shutdown waits for owned workers before destruction. UI indicates loading immediately and preserves the last committed list until completion or error.

Search defaults to current-folder Unicode filename substring matching. A search action may offer explicit subfolder/case controls separately. Traversal never follows directory symlinks/reparse links and has cancellation checks, preventing cycles. Sorting is typed for size/time; name uses locale-aware comparison with stable path tie-break. Folder grouping is explicit and consistent. Complete listing stays retained when filtering; displayed and total count are distinguished. Metadata and selected-byte sums do not double count rows.

## File safety and job contract (P1)

A mutation request has operation kind, original source paths, destination directory or new leaf name, and explicit collision policy. UI collects the user's choice for each collision or apply-to-all; worker never opens a dialog. Default conflict policy stops/skips and never overwrites implicitly. Replace and permanent deletion require confirmation. Paths containing an invalid leaf or traversal as a new name are rejected at the producing boundary.

Copy/move preflight rejects source equals destination and placing a directory inside itself, including resolved identity where available. Unresolved identity is an error for destructive applicability. Existing directory merges and unfamiliar reparse-point mutation are rejected explicitly until supported. Links are copied as links or reported Unsupported; directory traversal never follows them. Native same-volume rename is preferred for move; cross-volume copy removes a source only after the entire item copy succeeds. Copy failure preserves source and reports any owned partial destination; cleanup failure retains the path in result for retry. Replacement must preserve existing destination through a temporary sibling/backup transaction until new data is committed, and failures restore it or explicitly report retained owned backup. No silent destructive QFile::remove-before-copy pattern.

The operation manager serializes mutation jobs, runs work off the GUI thread, reports item and byte progress, and refreshes affected tabs after completion/partial failure. Progress includes phase so preflight/cleanup are not shown as full success. Cancellation (P2) is only advertised if implemented across execution and final cleanup. Permanent deletion is offered only through an explicit confirmed action; normal Delete requests recycle. Clipboard and drag/drop carry local file URLs plus copy/move intention. Copy/Cut/Paste and internal/external drops use the same operation path and conflict handling.

## Presentation baseline

Qt Widgets, native frame/system font, own QPainter/simple text icons plus OS-provided file icons. Dark palette uses a near-black tab band, charcoal toolbar, subtle separator, 180–220 px tree sidebar, broad content view, quiet status bar; no screenshot crops/assets. Three top rows mirror the reference: tabs/+; navigation/address/search; command bar with New, Cut/Copy/Paste/Rename/Delete, Sort, View, More. Content supports Details, List and Icons with the same selection model. Details columns are Name/Modified/Type/Size. A resize-friendly splitter and expanding address/list area avoid fixed desktop geometry. A details/preview side pane is optional and not claimed.

Named accessible controls and stable object names support root's UI validation. Table/list support Ctrl/Shift extended selection and double-click. Common shortcuts: Alt+Left/Right/Up, Ctrl+L, Ctrl+T/W, Ctrl+Tab, F5, Ctrl+F, Ctrl+A/C/X/V, Delete, Shift+Delete, F2, Ctrl+Shift+N, Enter, Escape. Errors use inline recoverable text with an optional details dialog; dangerous confirmations are modal. A nonexistent path remains editable while committed path/list remain intact.

## Delivery and measurement

CMake target explorer_a links Qt Core/Gui/Widgets/Concurrent; tests additionally Qt Test. Build needs Qt 6.4+ so the existing Linux 6.4.2 and Windows 6.8.3 are supported. Windows-only code guarded and linked in the platform target. Deployment uses existing windeployqt into A-owned artifacts. No installer/system/PATH/config changes. Linux build/test runs through existing WSL, macOS is unverified without an environment.

Shared build helper retains elapsed/CPU/peak process-family memory and logs. Directory benchmark uses immutable bulk fixture, cold app start and warm refresh separately, with fixture identity and resource limits. Whole-task token/API cost and exact deployment ID are unknown unless root has telemetry; no invented estimate is presented as measurement. Development timing records include stage boundary timestamps; idle time waiting for root gates is labeled separately.

Feature evidence labels: runtime observed, general API based, AI inference, implemented/unverified, passed actual test, unsupported/unimplemented. All A implementation attribution is general API based or AI inference; none is REA-derived. Source-based accuracy is deferred until root explicitly authorizes Phase 6 after independent freeze.
