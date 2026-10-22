# Phase 6 navigation source accuracy audit

Post-freeze verification only. Thirteen immutable initial questions contain six narrow positive assertions and seven unknown mappings. All six positives agree with pinned release source at the behavioral-content level. Seven unknowns are **no initial positive claim**, not successful recovered behavior. No exact VA-to-source identity was proved.

Source: official `version-1.4.0` tag, commit `384c2f687fd55c1e71e9fcb272f9113de009a248`; ZIP SHA-256 `3531f0ba75b5bfb64be4674a960b57c62c2adf62a3b04578e485e2442fdbc655`. Gate chronology is retained in `verification/phase6-freeze-verification.json`, `verification/phase6-access.json`, and `verification/source-acquisition.json`. Their hashes, immutable assertions, initial Evidence IDs and source ranges/hashes are in [the JSON ledger](phase6-navigation-source.json).

Tag/source matches release version 1.4.0. Version.h gets fourth component from environment build number (fallback 0), populated from APPVEYOR_BUILD_NUMBER in the project. This does not establish exact `.2293` build provenance. No PDB/link-map/reproducible build was obtained. Source names below were learned after freeze; static VA associations are inferred from matching operations and sequence.

| Question | Initial status / confidence | Comparison |
|---|---|---|
| RE02-PATH | derived / high | confirmed |
| RE02-ENUM | derived / high | confirmed |
| RE02-METADATA | derived / high | confirmed |
| RE02-LIST | derived / medium | confirmed |
| RE02-SORT | derived / high | confirmed |
| RE02-ERROR | unknown / unknown | no_initial_positive_claim |
| RE03-DOUBLECLICK | unknown / unknown | no_initial_positive_claim |
| RE03-UP | unknown / unknown | no_initial_positive_claim |
| RE03-HISTORY | unknown / unknown | no_initial_positive_claim |
| RE03-NEWTAB | unknown / unknown | no_initial_positive_claim |
| RE03-SELECTION | unknown / unknown | no_initial_positive_claim |
| RE03-RENAME | unknown / unknown | no_initial_positive_claim |
| RE03-SEARCH | derived / high | confirmed |

Identity references: [version, lines 5-19](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Version.h#L5), [version-resource, lines 51-77](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++VersionInfo.rc#L51), [build-number, lines 150-162](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++.vcxproj#L150).

## RE02-PATH - confirmed

Initial assertion (derived, high): The recovered parse helper turns a path string into a PIDL with SHParseDisplayName; the recovered browse body takes a PIDL. The UI-to-virtual-dispatch edge is unresolved.

Observed source facts:

- ShellNavigationController::BrowseFolder(wstring,bool) parses with SHParseDisplayName then passes absolute PIDL to its PIDL overload; that forwards to navigator or opens a tab in ForceNewTab mode.
- ShellBrowser::BrowseFolder accepts PCIDLIST_ABSOLUTE and bool; browser constructs controller with this as navigator.
- Address Enter invokes OnGo, which decodes text and calls Explorerplusplus::OpenItem. That string overload independently parses PIDL; folder dispatch reaches Navigation::BrowseFolderInCurrentTab(PIDL) then controller. Ordinary address entry need not call the controller string parse overload.

Source: [parse: ShellNavigationController::BrowseFolder overloads, lines 126-148](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L126), [browse: ShellBrowser::BrowseFolder, lines 18-94](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L18), [controller-owner: ShellBrowser creates controller, lines 123-128](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L123), [address-enter: AddressBar::EditSubclass, lines 73-83](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/AddressBar.cpp#L73), [address-go: AddressBar::OnGo, lines 142-156](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/AddressBar.cpp#L142), [open: OpenItem overloads; ordinary folder branch, lines 121-208](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L121), [open-folder: OpenFolderItem; OpenFileItem, lines 292-311](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L292), [navigate: OnNavigateUp; BrowseFolderInCurrentTab, lines 22-67](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Navigation.cpp#L22).

Narrow parsing/PIDL transport agrees. Source closes the UI chain after freeze and distinguishes two parse owners; no recovery credit is assigned to that initially unknown edge.

Inferred candidates: `0x140163000` -> `ShellNavigationController::BrowseFolder(const std::wstring &,bool)`; `0x1401637d0` -> `ShellBrowser::BrowseFolder(PCIDLIST_ABSOLUTE,bool)`.

Residual unknowns: Exact VA/vtable identity remains inferred. No exact .2293 build provenance or runtime special-namespace verification.

## RE02-ENUM - confirmed

Initial assertion (derived, high): Browse directly calls the enumeration helper; the helper binds a Shell folder and invokes COM slots consistent with EnumObjects, IEnumIDList::Next, GetAttributesOf and GetDisplayNameOf.

Observed source facts:

- BrowseFolder directly calls EnumerateFolder, which binds through BindToIdl(pidl,IID_PPV_ARGS(&pShellFolder)) with a real COM output pointer.
- BindToIdl calls SHGetDesktopFolder, BindToObject for nonempty PIDL or QueryInterface for desktop, then releases desktop.
- Enum flags are FOLDERS|NONFOLDERS, plus INCLUDEHIDDEN|INCLUDESUPERHIDDEN if showHidden. EnumObjects failure or null enumerator returns hr. Next must return S_OK and fetch one child.
- Each child has SFGAO_FOLDER queried; display-name flags are INFOLDER for virtual parent or folder child, otherwise INFOLDER|FORPARSING. Successful name lookup feeds AddItemInternal.

Source: [browse: ShellBrowser::BrowseFolder, lines 18-94](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L18), [enum: ShellBrowser::EnumerateFolder, lines 140-207](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L140), [bind: BindToIdl, lines 653-674](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/ShellHelper.cpp#L653).

Initial COM-slot interpretation and enumeration relationship agree with explicit source methods. Missing binder out argument is a decompiler limitation, not source null-dereference evidence.

Inferred candidates: `0x140163e80` -> `ShellBrowser::EnumerateFolder`; `0x140196040` -> `BindToIdl`.

Residual unknowns: Exact compiled identities and six initially unresolved callees are not proved. No Shell-extension/virtual-item runtime completeness validation.

## RE02-METADATA - confirmed

Initial assertion (derived, high): The recovered metadata helper combines/clones PIDLs, resolves a filesystem path, queries FindFirstFileW data and returns an increasing item ID; root/query-failure branches populate fallback values.

Observed source facts:

- SetItemInformation generates ID, ILCombines parent/child, clones complete/child PIDLs into m_itemInfoMap, copies display name, resolves SHGetPathFromIDList and queries FindFirstFile for non-root paths.
- GenerateUniqueItemId returns m_directoryState.itemIDCounter++; DirectoryState initializes counter to zero. IDs are directory-state scoped, not process-global monotonic identities.
- Valid query handle is closed; root or failed-query fallback sets filename, zero low/high size and DIRECTORY attribute. Fallback WIN32_FIND_DATA is not value-initialized; this branch does not initialize every timestamp.
- ItemInfo_t owns PIDLs and WIN32_FIND_DATA. Column dispatch chooses name/type/size and modified/created/accessed helpers.

Source: [metadata: ShellBrowser::SetItemInformation, lines 238-296](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L238), [id: ShellBrowser::GenerateUniqueItemId, lines 751-754](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L751), [item-struct: DirectoryState; ItemInfo_t, lines 181-208](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.h#L181), [columns: GetColumnText, lines 23-44](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ColumnDataRetrieval.cpp#L23).

All narrow metadata/ID/fallback assertions agree. Source adds field ownership and ID lifetime. Initial claim did not establish complete fallback timestamps.

Inferred candidates: `0x140164210` -> `ShellBrowser::SetItemInformation`.

Residual unknowns: Exact compiled field offsets and VA identity. Runtime incomplete fallback metadata, failed SHGetPathFromIDList and all Shell-item error/display cases untested.

## RE02-LIST - confirmed

Initial assertion (derived, medium): The browse path clears the current list, enumerates, then materializes queued IDs into Unicode ListView insert messages; queue producer/consumer linkage is derived from shared fields.

Observed source facts:

- BrowseFolder deletes rows, enumerates, calls InsertAwaitingItems(FALSE), verifies sort mode then calls SortFolder.
- AddItemInternal obtains metadata ID, constructs AwaitingAdd_t with row/internal ID/position/iAfter and pushes to ShellBrowser-owned std::list m_AwaitingAddList.
- InsertAwaitingItems consumes that same list, looks up itemInfo, filters, prepares LVITEM with internal ID lParam, optional group/position and ListView_InsertItem; then PositionDroppedItems and list clear.
- PositionDroppedItems manipulates dropped-file positions/selection and exits early for Details mode. It is not general selection notification owner. Browse/enumeration/insertion are sequential source calls, not a responsiveness benchmark.

Source: [browse: ShellBrowser::BrowseFolder, lines 18-94](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L18), [queue: ShellBrowser::AddItemInternal overloads, lines 209-236](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L209), [queue-type: AwaitingAdd_t, lines 218-225](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.h#L218), [queue-owner: m_AwaitingAddList std::list owner, lines 548-553](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.h#L548), [insert: ShellBrowser::InsertAwaitingItems, lines 298-438](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L298), [position: ShellBrowser::PositionDroppedItems, lines 756-941](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L756).

Shared-offset queue hypothesis agrees with explicit single source-owned container and producer/consumer sequence. Generic selection, final sort and responsiveness remain outside initial claim.

Inferred candidates: `0x140164150` -> `ShellBrowser::AddItemInternal(int,int,BOOL)`; `0x1401646b0` -> `ShellBrowser::InsertAwaitingItems`; `0x14017cc30` -> `ShellBrowser::PositionDroppedItems`.

Residual unknowns: Exact binary offsets/Unicode macro expansion identity. No throughput, cancellation, thread-affinity trace or large-list benchmark.

## RE02-SORT - confirmed

Initial assertion (derived, high): A comparator branches between natural-name, FILETIME and 64-bit size comparisons, uses name fallback on ties and conditionally reverses the result. Natural-name comparison calls StrCmpLogicalW.

Observed source facts:

- SortMode Name=1, DateModified=2, Size=3 agree with initial branches. Sort dispatches to SortByName, SortByDate(Modified), SortBySize and additional modes.
- SortByName uses StrCmpLogicalW on drive paths for two roots or name-column text otherwise. Date compares FILETIMEs; size combines high/low into 64-bit values and treats two folders as zero size.
- Sort separates folders/files except Recycle Bin before comparison; ties use StrCmpLogicalW display names; !sortAscending negates entire result including folder priority.
- LVN_COLUMNCLICK reaches ColumnClicked, toggles direction for repeated column and calls SortFolder, which sends LVM_SORTITEMS with SortStub.

Source: [sort-modes: SortMode values, lines 10-15](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortModes.h#L10), [sort: SortFolder; SortStub; Sort, lines 14-91](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortManager.cpp#L14), [sort-tail: Sort tie fallback and direction, lines 370-384](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortManager.cpp#L370), [sort-name-size: SortByName; SortBySize, lines 10-87](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortHelper.cpp#L10), [sort-date: SortByDate, lines 106-126](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortHelper.cpp#L106), [selection-entry: ListView notification dispatch, lines 97-124](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ListView.cpp#L97), [column-click: ShellBrowser::ColumnClicked, lines 775-804](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ColumnManager.cpp#L775).

Natural-name/FILETIME/64-bit size/tie/direction mechanisms agree. Source-only additions name date/direction/folder priority/application; no complete initial sort-policy recovery is claimed.

Inferred candidates: `0x140185140` -> `ShellBrowser::Sort`; `0x140184520` -> `SortByName`.

Residual unknowns: Exact linked/inlined function boundaries. Locale/extensions/root/recycle ordering and final equality stability untested.

## RE02-ERROR - no_initial_positive_claim

Initial assertion (unknown, unknown): The complete failure-propagation and error-UX path is unknown; HRESULT branches and metadata fallback alone do not establish it.

Observed source facts:

- Controller parse returns SHParseDisplayName failure without browsing. Address-bar OpenItem(string) does nothing on parse failure in its inspected body.
- EnumerateFolder returns bind/enumeration failure, but BrowseFolder ignores that return after changing current-directory/clearing rows, proceeds to completion signal and returns S_OK.
- NavigationController updates history cursor before BrowseFolder(entry,false), without rollback in that method. Completion observer appends requested history entries.
- These source paths do not supply universal prior-state restoration/error-message guarantees. This is a source-derived limitation, not a new runtime reproduction.

Source: [parse: ShellNavigationController::BrowseFolder overloads, lines 126-148](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L126), [open: OpenItem overloads; ordinary folder branch, lines 121-208](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L121), [browse: ShellBrowser::BrowseFolder, lines 18-94](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L18), [enum: ShellBrowser::EnumerateFolder, lines 140-207](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L140), [history: Traversal; branch truncation; entries/cursor, lines 119-175](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/NavigationController.h#L119), [history-commit: Navigation completion observer, lines 28-75](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L28).

Initial complete error/UX mapping was unknown. New source coverage is neither a correct negative nor initial RE success.

Residual unknowns: Actual denied/disappearing-directory UI outcomes, races and all callers. Exact shipped-build behavior and error UX remain unverified.

## RE03-DOUBLECLICK - no_initial_positive_claim

Initial assertion (unknown, unknown): The native notification-to-dispatch-to-browse connection remains unknown. A Shell-attribute activation dispatcher and a separate browse pipeline are candidate components.

Observed source facts:

- NotifyHandler routes NM_DBLCLK to OnListViewDoubleClick. It checks active ListView, hit-tests, uses Alt for properties, Ctrl for new tab, Shift for new window, otherwise opens row.
- OpenListViewItem ILCombines parent/child PIDLs and invokes OpenItem. Shell attributes select ordinary folders for OpenFolderItem, then current-tab navigation unless new-window/new-tab choices apply.
- Navigation::BrowseFolderInCurrentTab forwards to selected tab controller, which forwards PIDL to navigator.

Source: [notify: Explorerplusplus::NotifyHandler, lines 1421-1459](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L1421), [doubleclick: Explorerplusplus::OnListViewDoubleClick, lines 834-876](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp#L834), [open-row: Explorerplusplus::OpenListViewItem, lines 1194-1204](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp#L1194), [open: OpenItem overloads; ordinary folder branch, lines 121-208](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L121), [open-folder: OpenFolderItem; OpenFileItem, lines 292-311](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L292), [navigate: OnNavigateUp; BrowseFolderInCurrentTab, lines 22-67](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Navigation.cpp#L22), [parse: ShellNavigationController::BrowseFolder overloads, lines 126-148](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L126).

Source agrees with candidate dispatcher role and supplies missing notification chain after freeze. Initial ledger made no positive event mapping.

Inferred candidates: `0x1400bce20` -> `Explorerplusplus::OpenItem(PCIDLIST_ABSOLUTE,BOOL,BOOL)`.

Residual unknowns: Exact VA/vtable target mapping. Runtime modifier and namespace dispatch untested.

## RE03-UP - no_initial_positive_claim

Initial assertion (unknown, unknown): The internal Up command/parent-navigation path is unknown. ILRemoveLastID in the execution helper constructs an execution working directory and is not evidence of the Up handler.

Observed source facts:

- Up command invokes Navigation::OnNavigateUp. Unlocked tab calls controller GoUp; address-locked tab computes parent and creates selected new tab.
- GoUp obtains GetVirtualParentPath(current history PIDL) then navigator BrowseFolder. Parent helper rejects desktop namespace root; otherwise clones PIDL and removes last ID.
- OpenFileItem separately removes last PIDL ID for execution working directory; that operation alone does not identify Up.

Source: [nav-command: Back/Forward/Up command dispatch, lines 1199-1212](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L1199), [navigate: OnNavigateUp; BrowseFolderInCurrentTab, lines 22-67](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Navigation.cpp#L22), [up: CanGoUp; GoUp; Refresh, lines 77-119](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L77), [parent: GetVirtualParentPath; IsNamespaceRoot, lines 166-193](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/ShellHelper.cpp#L166), [open-folder: OpenFolderItem; OpenFileItem, lines 292-311](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L292).

Initial Up mapping was unknown and correctly did not attribute execution-directory construction to it. Newly observed source chain is additional coverage only.

Inferred candidates: `0x1400bd5c0` -> `Explorerplusplus::OpenFileItem (execution directory, not Up)`.

Residual unknowns: Actual Up-handler binary address association. Runtime desktop/root/virtual-parent and address-lock outcomes untested.

## RE03-HISTORY - no_initial_positive_claim

Initial assertion (unknown, unknown): The internal history policy and event mapping remain unknown; NavigationController/ShellNavigationController RTTI and index-based virtual calls are leads only.

Observed source facts:

- Back/Forward commands invoke selected tab controller inherited GoBack/GoForward. NavigationController owns vector<unique_ptr<HistoryEntryType>> m_entries and m_currentEntry.
- GoToOffset(-1/+1) validates entry, updates cursor before BrowseFolder(entry,false); invalid offset returns failure. AddEntry resizes current+1 to truncate forward entries, appends and increments cursor.
- Controller subscribes to navigator completion at_front; adds PIDL HistoryEntry when addHistoryEntry. ShellBrowser emits completion at end of BrowseFolder. Refresh browses with false.
- Each Tab creates separate ShellBrowser, which creates separate controller; this establishes per-tab source ownership.

Source: [nav-command: Back/Forward/Up command dispatch, lines 1199-1212](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L1199), [history-command: OnGoBack; OnGoForward; OnGoToOffset, lines 285-301](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainMenuHandler.cpp#L285), [history: Traversal; branch truncation; entries/cursor, lines 119-175](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/NavigationController.h#L119), [history-commit: Navigation completion observer, lines 28-75](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L28), [browse: ShellBrowser::BrowseFolder, lines 18-94](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L18), [up: CanGoUp; GoUp; Refresh, lines 77-119](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp#L77), [tab-owner: Tab separately creates ShellBrowser, lines 25-50](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Tab.cpp#L25), [controller-owner: ShellBrowser creates controller, lines 123-128](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L123).

Initial names/index branches were leads. Source-owned cursor/commit/branch policy was learned after freeze and is not credited as recovered.

Residual unknowns: Exact binary controller/history bodies. Runtime failure rollback and per-tab history stress not tested; cursor moves before browse in inspected method.

## RE03-NEWTAB - no_initial_positive_claim

Initial assertion (unknown, unknown): TabContainer is a named candidate owner, but native creation/switch/close event handlers and per-tab history ownership remain unknown.

Observed source facts:

- OnNewTab uses selected+focused directory row if available, otherwise default directory. TabContainer retries static default if configured default creation fails.
- TabContainer validates PIDL directory, stores unique_ptr<Tab> in m_tabs, inserts control item, connects observers and browses separate browser/controller. DuplicateTab creates from current directory string rather than copying history there.
- TCN_SELCHANGE and SelectTabAtIndex emit tab-selected signal. CloseTab may close main window for last tab per config; otherwise rejects locked/address-locked, removes control item, stops directory monitor, releases browser and erases tab.
- Tab separately creates browser; browser separately creates controller, establishing per-tab location/history source owners.

Source: [new-tab: Explorerplusplus::OnNewTab, lines 83-105](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabHandler.cpp#L83), [tab-default: Default/string tab creation, lines 862-899](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L862), [tab-create: CreateNewTab; SetUpNewTab, lines 920-1044](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L920), [duplicate: TabContainer::DuplicateTab, lines 1350-1354](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L1350), [tab-switch: TCN_SELCHANGE dispatch, lines 592-606](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L592), [tab-select: TabContainer::SelectTabAtIndex, lines 1229-1241](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L1229), [tab-close: TabContainer::CloseTab, lines 1077-1112](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L1077), [close-command: Explorerplusplus::OnCloseTab, lines 231-235](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabHandler.cpp#L231), [tab-owner: Tab separately creates ShellBrowser, lines 25-50](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Tab.cpp#L25), [controller-owner: ShellBrowser creates controller, lines 123-128](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L123).

TabContainer candidate name agrees, but creation/switch/close/history chain was initially unknown and cannot be counted retrospectively.

Residual unknowns: Compiled vtable-to-method correspondence. All restore/cleanup failure paths, final-tab config combinations and runtime outcomes outside this pass.

## RE03-SELECTION - no_initial_positive_claim

Initial assertion (unknown, unknown): The complete user selection notification/state path is unknown. ListView selection/group assistance in materialization does not establish that handler.

Observed source facts:

- Browser ListViewParentProc consumes LVN_ITEMCHANGED; OnListViewItemChanged compares selection state, handles checkbox mode, skips performing-drag changes, updates counts/size by internal item ID and emits selection signal.
- TabContainer rebroadcasts per-tab selection. Explorerplusplus invalidates matching display folder-size work and schedules timer for selected tab. Timer updates display, status and toolbar states.
- InsertAwaitingItems/PositionDroppedItems serve narrower row insertion/drop positioning roles, not general selection-notification entry.

Source: [selection-entry: ListView notification dispatch, lines 97-124](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ListView.cpp#L97), [selection: OnListViewItemChanged; UpdateFileSelectionInfo, lines 444-524](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ListView.cpp#L444), [selection-rebroadcast: Selection signal rebroadcast, lines 1007-1009](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp#L1007), [selection-observer: Selection signal subscription, lines 28-37](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabHandler.cpp#L28), [selection-ui: OnTabListViewSelectionChanged, lines 254-273](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabHandler.cpp#L254), [selection-timer: Selection status/display/toolbar timer, lines 119-127](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L119), [insert: ShellBrowser::InsertAwaitingItems, lines 298-438](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L298), [position: ShellBrowser::PositionDroppedItems, lines 756-941](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L756).

Initial refusal to treat materialization/position helpers as complete selection handler agrees. Source supplies new event-owner coverage only.

Inferred candidates: `0x14017cc30` -> `ShellBrowser::PositionDroppedItems`.

Residual unknowns: Exact selection-handler binary addresses and native-control Ctrl/Shift/keyboard traces. Every drag/checkbox/status race not assessed.

## RE03-RENAME - no_initial_positive_claim

Initial assertion (unknown, unknown): The generic F2-to-commit internal chain remains unknown. The separately recovered new-menu callback starts label editing but does not establish commit.

Observed source facts:

- F2 accelerator maps to IDM_FILE_RENAME command then OnFileRename; active-list focus dispatches OnListViewFileRename. One selection uses SFGAO_CANRENAME check and ListView_EditLabel; multiple selections use mass rename.
- End-label-edit rejects null/empty/trailing-dot text, checks invalid characters, preserves hidden extension, trims trailing spaces, builds paths and calls FileActionHandler::RenameFiles; false shows warning.
- RenameFiles creates IShellItem, calls NFileOperations::RenameFile, records successful items for undo. API wrapper creates IFileOperation, sets FOF_ALLOWUNDO|FOF_SILENT, calls RenameItem/PerformOperations and returns HRESULT without per-item/aborted reconciliation in inspected wrapper.
- New-menu QueueRename locates PIDL row or queues edit-start state. Generic F2 edits directly without requiring QueueRename; both use label-edit notification path at source level.

Source: [f2: F2 accelerator mapping, lines 103-108](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++_NoTranslation.rc#L103), [rename-command: IDM_FILE_RENAME dispatch, lines 399-402](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L399), [rename-focus: Explorerplusplus::OnFileRename, lines 65-87](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/EventSwitcher.cpp#L65), [rename-start: Single/multiple rename start, lines 878-913](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp#L878), [notify: Explorerplusplus::NotifyHandler, lines 1421-1459](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L1421), [rename-commit: OnListViewEndLabelEdit, lines 407-532](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp#L407), [rename-action: FileActionHandler::RenameFiles, lines 15-57](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileActionHandler.cpp#L15), [rename-api: NFileOperations::RenameFile, lines 29-62](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp#L29), [new-rename: ShellBrowser::QueueRename, lines 1259-1301](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L1259).

Initial F2-to-commit chain was unknown. Edit-start evidence did not establish mutation API. Source fills the chain after freeze without new initial recovery credit.

Inferred candidates: `0x14017d7a0` -> `ShellBrowser::QueueRename`.

Residual unknowns: Exact F2/commit/API binary identities. Runtime conflict, permission, cancellation and IFileOperation result fidelity untested.

## RE03-SEARCH - confirmed

Initial assertion (derived, high): A SearchDialog command branch for control 0x4cb starts a thread at 0x1400d23f0; recovered worker helpers enumerate names and conditionally recurse, then send result/completion messages.

Observed source facts:

- IDSEARCH=1227 (0x4cb); resource defines Search default pushbutton. OnCommand calls OnSearch; idle OnSearch calls StartSearching.
- StartSearching reads directory/pattern/recursion/regex/case/attribute controls, constructs Search and CreateThreads NSearchDialog::SearchThread; stub calls Search::StartSearching.
- Worker validates selected regex, calls SearchDirectory and sends SEARCHFINISHED (WM_APP+2). SearchDirectory enumerates, checks synchronized stop state and recurses only if m_bSearchSubFolders.
- SearchDirectoryInternal builds directory/*, uses FindFirstFile/FindNextFile, excludes . and .., matches name/attributes, parses matched path to PIDL and posts SEARCHITEMFOUND (WM_APP+1). It queues subdirectories and checks cancellation.
- Found receiver queues PIDLs and starts timer; OnTimer inserts rows and frees consumed PIDLs. Finished receiver displays completed/cancelled status and releases dialog-held Search reference; worker releases its reference on normal completion.
- Loop calls FindNextFile before processing its body, so record returned by FindFirstFile is not processed there. Reviewed recursion has no explicit visited-directory/reparse-point cycle guard. These are source observations, not reproduced runtime failures.

Source: [search-id: IDSEARCH = 1227 = 0x4cb, lines 280-283](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/resource.h#L280), [search-button: Search default pushbutton, lines 415-420](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++.rc#L415), [search-command: SearchDialog::OnCommand, lines 255-263](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L255), [search-start: OnSearch; StartSearching, lines 325-445](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L325), [search-thread: NSearchDialog::SearchThread, lines 990-998](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L990), [search-worker: Search::StartSearching, lines 1021-1053](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L1021), [search-recurse: Search::SearchDirectory, lines 1055-1085](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L1055), [search-enum: Search::SearchDirectoryInternal, lines 1088-1192](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L1088), [search-cancel: Search::StopSearching, lines 1195-1200](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L1195), [search-messages: Search message constants, lines 26-35](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L26), [search-receive: SearchDialog found/finished handling, lines 817-868](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L817), [search-results: SearchDialog::OnTimer, lines 915-974](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp#L915).

Control/command/thread/enumeration/conditional-recursion/messages agree closely. Source additionally names controls/receivers; full correctness/cancellation safety were not initially recovered.

Inferred candidates: `0x1400d0280` -> `SearchDialog::OnCommand`; `0x1400d04c0` -> `SearchDialog::StartSearching`; `0x1400d23f0` -> `NSearchDialog::SearchThread`; `0x1400d2490` -> `Search::StartSearching`; `0x1400d2590` -> `Search::SearchDirectory`; `0x1400d26a0` -> `Search::SearchDirectoryInternal`.

Residual unknowns: Exact .2293 compiled VA/vtable linkage. Runtime recursion/link-cycle/first-entry/error/close cancellation behavior and message-ownership stress unverified.

## Source file integrity

SHA-256 covers exact local file bytes. JSON records one-based source line spans per reference.

| Source file | SHA-256 |
|---|---|
| [Explorer++/Explorer++/AddressBar.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/AddressBar.cpp) | `6b2ca46375616664f0b5dd6c6223a5b3375b9ff1d0e7400c6cfd839ef3a341a5` |
| [Explorer++/Explorer++/EventSwitcher.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/EventSwitcher.cpp) | `9c99c66800cfea4705d8a3d21448e3d8fa087b1fcfe4da08eb7fbe8dc92f1dd5` |
| [Explorer++/Explorer++/Explorer++.rc](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++.rc) | `e3e6504eda385dac6b977b7d2861a4d863f86db6737c776a92381deb208d6fe4` |
| [Explorer++/Explorer++/Explorer++.vcxproj](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++.vcxproj) | `0319a8bc1d34dc34ad353a78c45612c4bd96da156dd0a659328dc085ae4e23e9` |
| [Explorer++/Explorer++/Explorer++VersionInfo.rc](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++VersionInfo.rc) | `26261ebd0b9827db3958b9b2bc46aad150a68dbb3fa376c7689a61ba3f098b92` |
| [Explorer++/Explorer++/Explorer++_NoTranslation.rc](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++_NoTranslation.rc) | `d7d6bd7bdbf1833076a4755f2763a58cf53b32e44db29741591f95a71447ed41` |
| [Explorer++/Explorer++/ListViewHandler.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp) | `4561b73e1099c32e135d7bcf580607b3d5b9bb8d635140bff36f0e1184187058` |
| [Explorer++/Explorer++/MainMenuHandler.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainMenuHandler.cpp) | `0a8225b9e6c09573752060cbc1fddd8ef7ec63416c25bc40385d6f95ea4dc09a` |
| [Explorer++/Explorer++/MainWndSwitch.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp) | `f6c2ef9bce117ee6d8eb1fc7bb617508ae8e52b99b6b5cf63db48e0360600e6f` |
| [Explorer++/Explorer++/MsgHandler.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp) | `38542cb20cc3061ae664b94388ec37c17c7d3e5215af05dcea4b6c6e52da9b22` |
| [Explorer++/Explorer++/Navigation.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Navigation.cpp) | `58f02b47faaabdacf1de049413930e016edbd625a7bdf506373283d5d6c25870` |
| [Explorer++/Explorer++/NavigationController.h](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/NavigationController.h) | `3f479a11d5ca20410a42eedf9c2b3ec10d376f67ac5db218b9c4d3eac54adf17` |
| [Explorer++/Explorer++/SearchDialog.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/SearchDialog.cpp) | `8a1b681207fc2a270a4a3a7a25a6bf954fecdc92620a45d076d6afdb5521893f` |
| [Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp) | `256f19d1a78c138c92ccf08de0906d5726bfb9dba214d7bc528a334383c40093` |
| [Explorer++/Explorer++/ShellBrowser/ColumnDataRetrieval.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ColumnDataRetrieval.cpp) | `976bd3eb2ab6f8996faae652714598a86efc51a1b980c12ebe6b305bea5a7d95` |
| [Explorer++/Explorer++/ShellBrowser/ColumnManager.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ColumnManager.cpp) | `84e58a91213a15e69990e4fa65f972a0e355fef3755015b161408a895ae8192c` |
| [Explorer++/Explorer++/ShellBrowser/ListView.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ListView.cpp) | `0617930db01d4c39c37b746b80fd45b8fdce168c74acf44bdb198b9a9d358bd2` |
| [Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp) | `6f500469513936f2670daecf142d2bee4d4d046d416a3eb98903abbecfb8f77e` |
| [Explorer++/Explorer++/ShellBrowser/ShellBrowser.h](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.h) | `d156801705efa9d89fb5874fdd5dd7f352d24919bfd4918af1fc68026baa9f92` |
| [Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellNavigationController.cpp) | `c44dade9eeb1c6a1734172f9859d5e825b9a815acc1c90bcaa83f3fca5a9b9ed` |
| [Explorer++/Explorer++/ShellBrowser/SortHelper.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortHelper.cpp) | `924a48daaa2de17e959353c5b38d47baa80d422d59d839ee96d28572cdd5c18b` |
| [Explorer++/Explorer++/ShellBrowser/SortManager.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortManager.cpp) | `558d52a6bdeaa879cc74fef165c17a3c8a169a413f815660e3b845e426577044` |
| [Explorer++/Explorer++/ShellBrowser/SortModes.h](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/SortModes.h) | `de9699136a5543e304c160f077f829acaf06299d2da37f8cc87cde4af943a8fd` |
| [Explorer++/Explorer++/Tab.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Tab.cpp) | `fe03b5bcaa658ed9885ec21345e95e0c2786efb46494e53348d9d1fc1a407ba2` |
| [Explorer++/Explorer++/TabContainer.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabContainer.cpp) | `6b9bf95a1f834b6418727f918cb6ee08cc814c2d868d967da9cf5f62f1bef250` |
| [Explorer++/Explorer++/TabHandler.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TabHandler.cpp) | `0698b288e0eb49c678a237071b661b29333e3e46554e2f559260e1d8a23a64f2` |
| [Explorer++/Explorer++/Version.h](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Version.h) | `6e3b2ee67ad856403a219bbbdbe550dadf4cc938cbf12c8947b1dfd4bed7fc8c` |
| [Explorer++/Explorer++/resource.h](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/resource.h) | `9f5adbd30e4bd121963e81789dafc4784f93bc56ff481b5387217b3ef8170bc6` |
| [Explorer++/Helper/FileActionHandler.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileActionHandler.cpp) | `5cd7b5e38ce5b7c3b19f8938327e1ea52458851fb47ab650fc31149ffd01e46b` |
| [Explorer++/Helper/FileOperations.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp) | `7b7d130e06ee6c457820fdf64a0d5bb8ca1f74faa0ca25522e16ee5e9bb7c9a5` |
| [Explorer++/Helper/ShellHelper.cpp](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/ShellHelper.cpp) | `90486967f22c8d8af15c8a2b5e0307324256995cb65df71d5bb4848b2f88c24d` |

## Verification limits

Read-only source review used ordinary repository tools and frozen compact findings. No target/build/test/GUI action or new REA query ran. Source review does not verify runtime behavior. Positive questions overlap; six agreeing assertions are not independent accuracy samples or 13/13 recovery. Whole-investigation accuracy remains unmeasured. Initial claims/snapshots were not edited.

Executed integrity validation passed: 13 copied initial records match the frozen ledger exactly; all 72 source references have matching byte hashes and valid BOM-aware decoded line spans; all six audit input hashes remained unchanged. No runtime/provider tests ran.
