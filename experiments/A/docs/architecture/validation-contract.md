# Experiment A validation contract

Status: planned only; no build/test claims at Phase 3. P0 is implemented and tested before root authorizes P1 extension. Root owns all desktop UI input. Agent tests may use offscreen self-built binaries and mutate only disposable A copies. Immutable fixture is never written. The denied verification path is passed only to our adapter test, never enumerated by a shell.

| Mandatory scenario | Required evidence |
|---|---|
| 1 Empty | real adapter returns zero entries; offscreen UI commits empty path and zero count |
| 2 1,000+ | bulk fixture returns exact 1,200 names; GUI event heartbeat continues during pending listing; elapsed/CPU/memory measured separately |
| 3 Unicode | exact names/paths, metadata and default-open request survive model/adapter; root actual default-app confirmation |
| 4 Missing | tagged NotFound, committed location/history/list preserved and visible recoverable error |
| 5 Denied | real ACL-denied directory adapter result AccessDenied; fake errors additionally test UI path; unavailable Linux permission case labeled |
| 6 History | successful visit/back/forward/up, failed history navigation, forward truncation, refresh no duplicate |
| 7 Tabs | independent paths/history/filter; close with pending listing discards late result |
| 8 Sort | numeric size/time ascending/descending and exact Unicode name set; original's observed order compared without claiming unobserved tie semantics |
| 9 Search | Unicode query finds exactly two fixture names; current-folder scope and clear restore; optional recursive case has cycle guard |
| 10 Collisions | disposable batch copy/move with skip/replace/cancel decisions; source/destination digest checks; no implied merge |
| 11 External delete | delete disposable entry externally between enumeration and activation/operation; adapter/UI reports error and refresh recovers |
| 12 Rename failure | invalid leaf, existing destination and denied/stale path; preserve original; selected row updates after success |
| 13 Links/cycles | real link where supported; browse listing bounded; recursive search/copy do not follow cycle; unavailable creation reported |
| 14 Restart | offscreen window create/close twice; pending browse/job shutdown test; actual deployed binary start/close via root |

FR-11–20 additionally require multi-item copy/move/trash/rename/mkdir, Ctrl/Shift selection, internal and external URL drop dispatch, Details/List/Icons changes, shortcuts and selected/progress count. UI tests exercise public signals/actions/views rather than duplicating implementation arithmetic. Adapter integration checks real files/metadata and content hashes. Fake adapter tests are reserved for deterministic late completion/error/cancellation/cleanup states hard to provoke with real OS.

Windows: build + CTest offscreen + root actual UI + deployed executable. Linux: existing WSL Qt 6.4.2 build + CTest + offscreen application start; any desktop/default-app integration not exercised is unverified. macOS: compilation guards/API portability review only until a machine exists. Static analysis/format checks use already installed tools and retain commands/results; no new software is installed.

P0 gate artifacts: source, build logs/resource JSON, exact test report, feature checklist with limitations, root-launchable path. P1/final artifacts extend the same evidence matrix. Final comparison only counts shared, actually exercised scenarios and labels original-side unobserved cases. Source access remains prohibited until the explicit Phase 6 freeze authorization.
