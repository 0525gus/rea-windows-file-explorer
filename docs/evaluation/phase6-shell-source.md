# Phase 6 shell/P1 source accuracy audit

Post-freeze verification only. Official `version-1.4.0` commit `384c2f687fd55c1e71e9fcb272f9113de009a248`; acquired archive SHA-256 `3531f0ba75b5bfb64be4674a960b57c62c2adf62a3b04578e485e2442fdbc655`. Frozen B commit `64032d766cf634fe00b45aa0d8c951878e0ef5c3`; manifest SHA-256 `a93289e08db7e7338110480063716bd2be434ffd1e76585e0bc36a2b996cf729`. The JSON companion retains input/source hashes, exact immutable initial assertions, original Evidence IDs and initial unknowns.

No target execution, runtime event trace, source rebuild, PDB or linker map was performed. Official release-tag pairing is established by acquisition metadata; exact shipped 1.4.0.2293 build identity is not established by Version.h.

Six narrow positive contents are supported by source, one Boost attribution remains partial, and five initial unknowns remain uncredited coverage gaps. No end-to-end REA success or exact VA-to-source symbol mapping is established. Related questions share evidence; these are not 12 independent accuracy trials. No A/B implementation or frozen document was edited.

Explorer++ Project; cited source headers SPDX-License-Identifier: GPL-3.0-only; LICENSE retained in acquired tree. Verification observations only; no original implementation copied into either experiment.

| Question | Initial status / confidence | Comparison |
|---|---|---|
| RE04-ICONS | derived / high | confirmed |
| RE04-THUMBNAILS | unknown / unknown | no_initial_positive |
| RE04-OPEN | observed / high | confirmed |
| RE04-CHANGES | observed / high | confirmed |
| RE04-COPY-MOVE | derived / high | confirmed |
| RE04-DROP | inferred / medium | confirmed |
| RE04-PROPERTIES | unknown / unknown | no_initial_positive |
| RE04-RECYCLE | unknown / unknown | no_initial_positive |
| P1-RENAME-START | derived / high | confirmed |
| P1-RENAME-COMMIT | unknown / unknown | no_initial_positive |
| P1-MKDIR-WRAPPER | derived / medium | partial |
| P1-MKDIR-UI | unknown / unknown | no_initial_positive |

## RE04-ICONS — confirmed

Initial assertion: Inspected functions call SHGetFileInfoW with system-index and icon/overlay PIDL/path flag combinations; this supports Windows icon-information delegation.

Source finding: Windows icon-information delegation is explicit in source: system-index drive updates and icon/overlay PIDL/path refresh match the initial narrow API modes. Source additionally exposes async IconFetcher, path cache lookup and ListView display consumers.

Qualification: Initial claim is correct at API/mode level; it never established every displayed icon or full lifecycle.

Remaining limits: Per-address choice among similar SHGetFileInfo callers remains inferred; no exact function symbol assignment. All-row/cache/cancellation correctness was not executed.

Source references: [ICON-SYS](#source-icon-sys), [ICON-MOD](#source-icon-mod), [ICON-REN](#source-icon-ren), [ICON-FETCH](#source-icon-fetch), [ICON-DISPLAY](#source-icon-display), [UNICODE](#source-unicode).

## RE04-THUMBNAILS — no_initial_positive

Initial assertion: The per-file thumbnail acquisition/provider/cache pipeline is unknown. ThumbnailResult names and DwmSetIconicThumbnail import do not establish it.

Source finding: Source-only answer: per-file thumbnails use IExtractImage obtained from parent Shell folder GetUIObjectOf; GetLocation precedes Extract. Thread-pool futures/result IDs retain a unique bitmap and internal item index; ProcessThumbnailResult updates image list/ListView. Taskbar DwmSetIconicThumbnail is a distinct tabProxy path.

Qualification: Unknown is preserved as a coverage gap; the later source does not make it an initially recovered implementation.

Remaining limits: This source pipeline was absent from initial REA knowledge and earns no initial success credit. Result retention/removal, in-flight task completion and full cache/cancellation correctness are not runtime verified.

Source references: [THUMB-WORK](#source-thumb-work), [THUMB-DISPLAY](#source-thumb-display), [THUMB-CLEAN](#source-thumb-clean), [THUMB-IMAGE](#source-thumb-image), [DWM](#source-dwm).

## RE04-OPEN — confirmed

Initial assertion: The inspected helper calls ShellExecuteExW with the input PIDL, fMask 0xC and the exact empty-string verb L""; it is a PIDL execution delegation path.

Source finding: OpenFileItem computes parent working directory and passes exact EMPTY_STRING verb; ExecuteFileAction uses SEE_MASK_INVOKEIDLIST and input PIDL with NULL lpFile, then ShellExecuteEx. OpenItem source connects ordinary files and ZIP/shortcut fallbacks; Control Panel folders have separate explorer.exe dispatch.

Remaining limits: Likely compiler inlining explains combined decompiled body, but exact 0x1400bd5c0 source mapping remains inferred. OpenFileItem ignores helper BOOL; source branching does not prove a launched associated application or every failure result.

Source references: [OPEN-FILE](#source-open-file), [OPEN-EXEC](#source-open-exec), [OPEN-DISPATCH](#source-open-dispatch), [EMPTY](#source-empty), [UNICODE](#source-unicode).

## RE04-CHANGES — confirmed

Initial assertion: A helper calls overlapped ReadDirectoryChangesW with a 16000-byte buffer and completion address 0x1401a9bc0. A separate dispatcher handles add/remove/modify/rename update cases.

Source finding: WatchDirectoryInternal passes allocated PRIMARY_BUFFER_SIZE, OVERLAPPED and CompletionRoutine to ReadDirectoryChangesW. Source defines size as 1000*sizeof(FILE_NOTIFY_INFORMATION), corresponding to 16000 for the Windows ABI. DirectoryAltered separately handles all five action cases. Later source fills bridge: registered callback -> FilesModified queue and 200ms timer -> WM_USER_FILESADDED -> DirectoryAltered with unique folder guard.

Qualification: Source completes initial missing bridge only after freeze. Do not retroactively credit initial REA with callback-to-list connectivity or overflow handling.

Remaining limits: Callback/native address equality remains inferred. CompletionRoutine only handles success/nonzero and ERROR_OPERATION_ABORTED: no explicit zero-byte/other-error overflow recovery here. Stop is worker APC CancelIo/CloseHandle, abort completion frees request. Final shutdown and all races are source-only, not verified safe.

Source references: [WATCH-SIZE](#source-watch-size), [WATCH-REQUEST](#source-watch-request), [WATCH-COMPLETE](#source-watch-complete), [WATCH-REGISTER](#source-watch-register), [WATCH-BRIDGE](#source-watch-bridge), [WATCH-QUEUE](#source-watch-queue), [WATCH-MESSAGE](#source-watch-message), [WATCH-UPDATE](#source-watch-update), [WATCH-OPEN](#source-watch-open), [WATCH-STOP](#source-watch-stop), [WATCH-SHUTDOWN](#source-watch-shutdown).

## RE04-COPY-MOVE — confirmed

Initial assertion: A wrapper constructs a double-NUL UTF-16 source list and calls SHFileOperationW with MOVE/COPY selected by a boolean; success also requires no aborted operations, and returned name mappings are reconciled/freed.

Source finding: CopyDroppedFilesInternalAsync matches observed SHFileOperation request: bCopy selects COPY(true)/MOVE(false), BuildFilenameList creates double NUL, flags request mapping and optional collision rename, success requires (!API return && !aborted); successful mappings update filenames and are freed before callback.

Qualification: This SHFileOperation wrapper is a DropHandler transfer path, not the only copy/move implementation. NFileOperations::CopyFiles separately uses IFileOperation::CopyItems/MoveItems plus ALLOWUNDO. It is still not delete/recycle proof.

Remaining limits: Name mappings are handled inside success branch; source does not establish complete failed/mixed outcome cleanup or rollback. Exact UI command/native wrapper linkage and real collision/progress behavior were not runtime traced.

Source references: [COPY-LEGACY](#source-copy-legacy), [COPY-LIST](#source-copy-list), [COPY-CALLER](#source-copy-caller), [COPY-MODERN](#source-copy-modern).

## RE04-DROP — confirmed

Initial assertion: A seven-slot ShellBrowser table and inspected bodies are consistent with IDropTarget roles. Effect selection uses allowed effects/key/root state; Drop delegates to DropHandler, whose mode-1 path requests a Shell IDropTarget and calls its drag/drop slots.

Source finding: ShellBrowser explicitly implements IDropTarget. Source effect selection matches allowed effects/key/same-drive policy and DropHandler delegation. DragType LeftClick=0 invokes custom format handlers; RightClick=1 obtains Shell IDropTarget via CreateViewObject and calls DragEnter, Drop, DragLeave, Release. Local left-drag MOVE may instead only reposition existing rows.

Qualification: The mode-1 delegated interface is IDropTarget; observed slots +0x18/+0x30/+0x28/+0x10 correspond to DragEnter/Drop/DragLeave/Release. No DragOver call is implied by this delegated branch.

Remaining limits: Exact seven-slot VA/table-to-class binding is still inference; source names do not prove binary vtable layout identity. RegisterDragDrop source is present. Bounded search of ShellBrowser source finds no explicit RevokeDragDrop; destructor destroys window/releases helpers. No safe revoke lifecycle is claimed. No event trace, transfer/cancellation test or complete outgoing drag audit.

Source references: [DROP-INTERFACE](#source-drop-interface), [DROP-ENTER](#source-drop-enter), [DROP-POLICY](#source-drop-policy), [DROP-ROUTE](#source-drop-route), [DROP-ENUM](#source-drop-enum), [DROP-MODE](#source-drop-mode), [DROP-LEFT](#source-drop-left), [DROP-RIGHT](#source-drop-right), [DROP-REGISTER](#source-drop-register), [DROP-DESTROY](#source-drop-destroy).

## RE04-PROPERTIES — no_initial_positive

Initial assertion: The property-sheet execution path is unknown; Properties labels alone do not establish a Shell verb/COM property-sheet call.

Source finding: Source-only answer: focus routing selects ShellBrowser::ShowPropertiesForSelectedFiles, collects child PIDLs, then ShowMultipleFileProperties -> ExecuteActionFromContextMenu with properties. It obtains IContextMenu and invokes an ANSI properties verb, returns HRESULT and releases interface.

Remaining limits: Actual property sheet, Shell extension behavior and errors not executed. No initial native address for property call; no retroactive native mapping.

Source references: [PROPERTY-ENTRY](#source-property-entry), [PROPERTY-SELECTION](#source-property-selection), [PROPERTY-API](#source-property-api).

## RE04-RECYCLE — no_initial_positive

Initial assertion: The original delete/recycle implementation is unknown. RecycleBin names and the inspected MOVE/COPY wrapper are not recycle-on-delete evidence.

Source finding: Source-only answer: Delete/Permanent Delete pass permanent=false/true; selected PIDLs reach NFileOperations::DeleteFiles. It uses IFileOperation::DeleteItems/PerformOperations; nonpermanent adds FOF_ALLOWUNDO. Nonsilent requests WANTNUKEWARNING; silent requests SILENT/NOCONFIRMATION/NOERRORUI.

Qualification: Initial unknown was appropriate: the transfer SHFileOperation wrapper supplies no recycle evidence. Actual delete API/flags are learned only from source.

Remaining limits: This establishes requested Shell deletion/undo policy, not guaranteed recycling for every target/filesystem or observed per-item outcomes. Wrapper returns HRESULT without GetAnyOperationsAborted; no per-item/cancellation outcome verification.

Source references: [DELETE-ENTRY](#source-delete-entry), [DELETE-SELECTION](#source-delete-selection), [DELETE-WRAPPER](#source-delete-wrapper), [DELETE-API](#source-delete-api).

## P1-RENAME-START — confirmed

Initial assertion: The containing function at 0x1400838a0 logs rename of an item created via new menu, then directly calls 0x14017d7a0; the helper matches PIDLs to rows and sends 0x1076 (LVM_EDITLABELW), or stores a pending PIDL when no row exists.

Source finding: NewMenuClient::SelectAndEditItem flags EDIT=1 and EDIT_WINDOWS_10=3 log the exact new-menu rename message and QueueRename. QueueRename matches PIDLs and starts ListView_EditLabel, otherwise clones pending PIDL. Source adds pending consumer: insertion clears flag on PIDL match, update sends NEWITEMINSERTED and owner starts editing.

Qualification: This begins label editing; rename filesystem commit belongs to the separate end-label path below.

Remaining limits: Interior 0x1400838c0 vs containing 0x1400838a0 distinction remains in initial addresses; source does not prove exact entry VA. The source-defined Windows 10 flag is not new runtime evidence. Pending PIDL cleanup and generic F2 runtime relation were not audited end-to-end.

Source references: [NEW-ENUM](#source-new-enum), [NEW-CALLBACK](#source-new-callback), [RENAME-QUEUE](#source-rename-queue), [RENAME-PENDING](#source-rename-pending), [WATCH-UPDATE](#source-watch-update), [WATCH-MESSAGE](#source-watch-message), [RENAME-PENDING-MSG](#source-rename-pending-msg).

## P1-RENAME-COMMIT — no_initial_positive

Initial assertion: The original rename commit API and policy are unknown; starting ListView label editing does not prove SetNameOf or a filesystem rename call.

Source finding: Source-only answer: OnListViewEndLabelEdit validates/cancels text, keeps hidden extension, trims trailing spaces, and invokes FileActionHandler::RenameFiles. That creates IShellItem and calls NFileOperations::RenameFile -> IFileOperation::RenameItem -> PerformOperations with ALLOWUNDO|SILENT; it is not SetNameOf. UI warns on false and successful items are recorded for undo.

Qualification: The initial SetNameOf alternative was never a positive claim; a bounded absence of name spellings remains a bounded search.

Remaining limits: No actual name collision, permission or cancellation/retained selection test. PerformOperations HRESULT is used without explicit aborted-query; SUCCEEDED bookkeeping is not independently verified per-item completion.

Source references: [RENAME-EDIT-END](#source-rename-edit-end), [RENAME-COMMIT](#source-rename-commit), [RENAME-API](#source-rename-api).

## P1-MKDIR-WRAPPER — partial

Initial assertion: The inspected 0x140206a50 body is consistent with a Boost filesystem recursive directory-creation wrapper, with CreateDirectoryW, GetLastError and error/exception paths.

Source finding: Archive dependencies corroborate Boost/filesystem context (NuGet 1.72.0.0); they do not include the Boost filesystem implementation for source/body comparison. Bounded search of the acquired tree finds no create_directory/create_directories implementation/call spelling. The observed recursive native body, its exact Boost function and callers cannot be source-confirmed here.

Qualification: The narrow initial consistency inference remains plausible, not verified original signature. Source New Folder uses IFileOperation::NewItem and supplies no evidence it reaches this Boost wrapper.

Remaining limits: No vendored boost/filesystem implementation or shipped library link map/PDB/reproducible build. Package dependency version is not proof of exact object linked into supplied binary. Which higher-level binary callers 0x140213270/0x1402158e0 reach wrapper remains unresolved.

Source references: [BOOST-PACKAGE](#source-boost-package), [MKDIR-UI](#source-mkdir-ui), [MKDIR-API](#source-mkdir-api), [VERSION](#source-version).

## P1-MKDIR-UI — no_initial_positive

Initial assertion: The native New Folder command-to-creation-wrapper connection remains unknown. External folder creation/inline rename does not identify that internal call chain.

Source finding: Source-only answer: NewFolder toolbar/menu -> OnCreateNewFolder -> destination IShellItem and localized default-name resource -> NFileOperations::CreateNewFolder -> IFileOperation::NewItem(FILE_ATTRIBUTE_DIRECTORY)/PerformOperations. Progress sink obtains created PIDL only on successful PostNewItem and queues inline edit; Shell new-menu callback is another QueueRename entry.

Qualification: The initial native command-to-wrapper link stays unknown rather than being rewritten as a successful REA reconstruction. Later source identifies a distinct IFileOperation user path.

Remaining limits: Collision naming is delegated in source/comment; exact names, permissions and Shell UI outcomes were not executed. No direct source call from this command path to Boost create_directories; this does not prove internal Shell/OS implementation absence.

Source references: [MKDIR-ENTRY](#source-mkdir-entry), [MKDIR-UI](#source-mkdir-ui), [MKDIR-API](#source-mkdir-api), [MKDIR-SINK](#source-mkdir-sink), [NEW-CALLBACK](#source-new-callback), [RENAME-QUEUE](#source-rename-queue).

## Source reference ledger

Line ranges are 1-based physical source lines; hashes cover complete unmodified files. Paths below refer to the acquired commit, not the A/B implementations. Each statement is a source observation or direct derivation, not an execution observation.

<a id="source-version"></a>
**VERSION** — [Explorer++/Explorer++/Version.h:5–13](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Version.h#L5)

Source declares 1.4.0; build number is supplied by ENVIRONMENT_BUILD_NUMBER or defaults to 0. It does not independently bind this tree to shipped build 2293.
SHA-256 `6e3b2ee67ad856403a219bbbdbe550dadf4cc938cbf12c8947b1dfd4bed7fc8c` (621 bytes).

<a id="source-unicode"></a>
**UNICODE** — [Explorer++/Explorer++/Explorer++.vcxproj:41–63](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Explorer++.vcxproj#L41)

Release/Debug project configurations declare Unicode CharacterSet; API generic spellings in this source can compile to W APIs.
SHA-256 `0319a8bc1d34dc34ad353a78c45612c4bd96da156dd0a659328dc085ae4e23e9` (63755 bytes).

<a id="source-empty"></a>
**EMPTY** — [Explorer++/Helper/Macros.h:5–7](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/Macros.h#L5)

EMPTY_STRING is _T("").
SHA-256 `59c683737dab31cf95ba0a61d1d27545c86c88cd4507d711f49b681484f409a6` (1053 bytes).

<a id="source-icon-sys"></a>
**ICON-SYS** — [Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp:1430–1479](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L1430)

Drive update calls SHGetFileInfo with SHGFI_SYSICONINDEX and writes shfi.iIcon/text into a ListView item.
SHA-256 `6f500469513936f2670daecf142d2bee4d4d046d416a3eb98903abbecfb8f77e` (37685 bytes).

<a id="source-icon-mod"></a>
**ICON-MOD** — [Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp:367–388](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp#L367)

Modification overlay refresh calls SHGetFileInfo(path, SHGFI_ICON|SHGFI_OVERLAYINDEX), sets ListView overlay state and destroys hIcon.
SHA-256 `25860f63e28e58b1476d78da9f2346998e8d9a1942d1b7393bd90324d436879f` (18011 bytes).

<a id="source-icon-ren"></a>
**ICON-REN** — [Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp:547–615](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp#L547)

Rename reconciliation calls SHGetFileInfo(PIDL, SHGFI_PIDL|SHGFI_ICON|SHGFI_OVERLAYINDEX), sets text/image/overlay, then destroys hIcon.
SHA-256 `25860f63e28e58b1476d78da9f2346998e8d9a1942d1b7393bd90324d436879f` (18011 bytes).

<a id="source-icon-fetch"></a>
**ICON-FETCH** — [Explorer++/Helper/IconFetcher.cpp:133–148](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/IconFetcher.cpp#L133)

FindIconAsync requests PIDL/icon/overlay flags, rejects zero result, destroys hIcon, returns iIcon.
SHA-256 `69ac5c2cdcd99c392e67216782184a1416f42e22e8daed47f430dae3a5343b55` (4702 bytes).

<a id="source-icon-display"></a>
**ICON-DISPLAY** — [Explorer++/Explorer++/ShellBrowser/ListView.cpp:230–313](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ListView.cpp#L230)

List view reads cached path-index/fallback images, queues IconFetcher work, and ProcessIconResult resolves internal index before setting image and overlay.
SHA-256 `0617930db01d4c39c37b746b80fd45b8fdce168c74acf44bdb198b9a9d358bd2` (21749 bytes).

<a id="source-thumb-work"></a>
**THUMB-WORK** — [Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp:95–177](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp#L95)

Thumbnail task stores future by result counter, binds PIDL parent, gets IExtractImage for child, calls GetLocation then Extract, posts ready message, returns RAII bitmap/internal index.
SHA-256 `42ec043b20b63d4ae717b47be9e5c8ec9bb250953dcd295c39b236ef961f7237` (8807 bytes).

<a id="source-thumb-display"></a>
**THUMB-DISPLAY** — [Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp:180–216](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp#L180)

Ready result looks up future/result ID, requires thumbnail view, obtains bitmap image index, resolves internal index and sets ListView image.
SHA-256 `42ec043b20b63d4ae717b47be9e5c8ec9bb250953dcd295c39b236ef961f7237` (8807 bytes).

<a id="source-thumb-clean"></a>
**THUMB-CLEAN** — [Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp:60–92](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp#L60)

Removing thumbnail view clears queued work and results, restores callbacks and destroys thumbnail image list.
SHA-256 `42ec043b20b63d4ae717b47be9e5c8ec9bb250953dcd295c39b236ef961f7237` (8807 bytes).

<a id="source-thumb-image"></a>
**THUMB-IMAGE** — [Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp:231–298](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/HandleThumbnails.cpp#L231)

Extracted bitmap/icon is composed into image list; backing bitmap and icon handles are deleted.
SHA-256 `42ec043b20b63d4ae717b47be9e5c8ec9bb250953dcd295c39b236ef961f7237` (8807 bytes).

<a id="source-dwm"></a>
**DWM** — [Explorer++/Explorer++/TaskbarThumbnails.cpp:430–465](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/TaskbarThumbnails.cpp#L430)

Taskbar thumbnail handler prepares tabProxy thumbnail and calls DwmSetIconicThumbnail, separate from per-file IExtractImage.
SHA-256 `b3d098612acfe99b5affe40d072438846c8374ab6ad219e1f3c0bfe267a68959` (19514 bytes).

<a id="source-open-dispatch"></a>
**OPEN-DISPATCH** — [Explorer++/Explorer++/MsgHandler.cpp:132–289](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L132)

OpenItem handles ZIP configuration, folder navigation, shortcuts and Control Panel special dispatch; ordinary files call OpenFileItem with EMPTY_STRING parameters.
SHA-256 `38542cb20cc3061ae664b94388ec37c17c7d3e5215af05dcea4b6c6e52da9b22` (40846 bytes).

<a id="source-open-file"></a>
**OPEN-FILE** — [Explorer++/Explorer++/MsgHandler.cpp:302–310](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L302)

OpenFileItem clones parent PIDL for working directory and passes EMPTY_STRING verb to ExecuteFileAction. Its void body ignores helper return.
SHA-256 `38542cb20cc3061ae664b94388ec37c17c7d3e5215af05dcea4b6c6e52da9b22` (40846 bytes).

<a id="source-open-exec"></a>
**OPEN-EXEC** — [Explorer++/Helper/ShellHelper.cpp:123–139](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/ShellHelper.cpp#L123)

ExecuteFileAction assigns SEE_MASK_INVOKEIDLIST, caller verb, PIDL, parameters/directory, NULL lpFile and returns ShellExecuteEx result.
SHA-256 `90486967f22c8d8af15c8a2b5e0307324256995cb65df71d5bb4848b2f88c24d` (34145 bytes).

<a id="source-watch-size"></a>
**WATCH-SIZE** — [Explorer++/Helper/iDirectoryMonitor.cpp:29–29](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/iDirectoryMonitor.cpp#L29)

PRIMARY_BUFFER_SIZE is 1000*sizeof(FILE_NOTIFY_INFORMATION); 16000 follows the Windows struct ABI, not a source literal.
SHA-256 `fe39df6bbb93daca167ade3c24d2094e5be00d44165ab120bfdef413e50d9f26` (11732 bytes).

<a id="source-watch-open"></a>
**WATCH-OPEN** — [Explorer++/Helper/iDirectoryMonitor.cpp:183–233](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/iDirectoryMonitor.cpp#L183)

WatchDirectory stores caller callback/data, opens FILE_LIST_DIRECTORY with share read/delete/write and OVERLAPPED/BACKUP_SEMANTICS, queues worker APC.
SHA-256 `fe39df6bbb93daca167ade3c24d2094e5be00d44165ab120bfdef413e50d9f26` (11732 bytes).

<a id="source-watch-request"></a>
**WATCH-REQUEST** — [Explorer++/Helper/iDirectoryMonitor.cpp:292–317](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/iDirectoryMonitor.cpp#L292)

WatchDirectoryInternal allocates buffer and passes OVERLAPPED plus CompletionRoutine to ReadDirectoryChangesW; failure frees buffer, cancels and closes handle.
SHA-256 `fe39df6bbb93daca167ade3c24d2094e5be00d44165ab120bfdef413e50d9f26` (11732 bytes).

<a id="source-watch-complete"></a>
**WATCH-COMPLETE** — [Explorer++/Helper/iDirectoryMonitor.cpp:319–368](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/iDirectoryMonitor.cpp#L319)

CompletionRoutine parses success/nonzero entries, calls stored callback, frees buffer and rearms; ERROR_OPERATION_ABORTED calls DeleteRequest. Other errors/zero bytes have no explicit recovery branch here.
SHA-256 `fe39df6bbb93daca167ade3c24d2094e5be00d44165ab120bfdef413e50d9f26` (11732 bytes).

<a id="source-watch-stop"></a>
**WATCH-STOP** — [Explorer++/Helper/iDirectoryMonitor.cpp:370–438](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/iDirectoryMonitor.cpp#L370)

DeleteRequest frees buffer/data and erases matching request; StopDirectoryMonitor queues APC which CancelIo/CloseHandle on worker thread.
SHA-256 `fe39df6bbb93daca167ade3c24d2094e5be00d44165ab120bfdef413e50d9f26` (11732 bytes).

<a id="source-watch-shutdown"></a>
**WATCH-SHUTDOWN** — [Explorer++/Helper/iDirectoryMonitor.cpp:93–107](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/iDirectoryMonitor.cpp#L93)

Destructor queues ExitWorkerThread, closes thread handle, waits stop event, clears list and deletes critical section. This is source structure, not verified cancellation safety.
SHA-256 `fe39df6bbb93daca167ade3c24d2094e5be00d44165ab120bfdef413e50d9f26` (11732 bytes).

<a id="source-watch-register"></a>
**WATCH-REGISTER** — [Explorer++/Explorer++/MsgHandler.cpp:658–694](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L658)

HandleDirectoryMonitoring stops previous watcher, skips virtual folders and registers DirectoryAlteredCallback with tab/folder identifiers and broad change flags, no subtree.
SHA-256 `38542cb20cc3061ae664b94388ec37c17c7d3e5215af05dcea4b6c6e52da9b22` (40846 bytes).

<a id="source-watch-bridge"></a>
**WATCH-BRIDGE** — [Explorer++/Explorer++/Misc.cpp:328–347](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/Misc.cpp#L328)

DirectoryAlteredCallback looks up tab and calls ShellBrowser::FilesModified with tab/folder identity.
SHA-256 `a809d24f1c94d5f9accbb1e23cbd00fd42ff6b9a2c50b9ff943befbe7628fdca` (11053 bytes).

<a id="source-watch-queue"></a>
**WATCH-QUEUE** — [Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp:147–171](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp#L147)

FilesModified adds action/name/folder ID under critical section and resets 200ms timer; TimerProc sends WM_USER_FILESADDED.
SHA-256 `25860f63e28e58b1476d78da9f2346998e8d9a1942d1b7393bd90324d436879f` (18011 bytes).

<a id="source-watch-message"></a>
**WATCH-MESSAGE** — [Explorer++/Explorer++/MainWndSwitch.cpp:131–155](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L131)

WM_USER_FILESADDED resolves tab and calls DirectoryAltered; WM_USER_NEWITEMINSERTED calls OnShellNewItemCreated.
SHA-256 `f6c2ef9bce117ee6d8eb1fc7bb617508ae8e52b99b6b5cf63db48e0360600e6f` (38021 bytes).

<a id="source-watch-update"></a>
**WATCH-UPDATE** — [Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp:19–112](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/DirectoryModificationHandler.cpp#L19)

DirectoryAltered checks current unique folder ID, dispatches added/modified/removed/old-name/new-name, sends update/new-item messages and clears altered list.
SHA-256 `25860f63e28e58b1476d78da9f2346998e8d9a1942d1b7393bd90324d436879f` (18011 bytes).

<a id="source-copy-list"></a>
**COPY-LIST** — [Explorer++/Helper/FileOperations.cpp:270–290](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp#L270)

BuildFilenameList copies every string including terminator and adds final NUL; caller frees allocation.
SHA-256 `7b7d130e06ee6c457820fdf64a0d5bb8ca1f74faa0ca25522e16ee5e9bb7c9a5` (19056 bytes).

<a id="source-copy-legacy"></a>
**COPY-LEGACY** — [Explorer++/Helper/DropHandler.cpp:1117–1197](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/DropHandler.cpp#L1117)

CopyDroppedFilesInternalAsync selects FO_COPY on bCopy, FO_MOVE otherwise; WANTMAPPINGHANDLE plus optional RENAMEONCOLLISION; checks return+abort, reconciles/frees mappings and calls OnDropFile only on success.
SHA-256 `18d9efb8b941b6be428d48400ed9390f0be46338d8f902d5c0f94f2b3d1aac6e` (33027 bytes).

<a id="source-copy-caller"></a>
**COPY-CALLER** — [Explorer++/Helper/DropHandler.cpp:897–960](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/DropHandler.cpp#L897)

CopyDroppedFiles separates source names by preferred/calculated effects, forces rename-on-collision for same directory, calls internal copy(true) and move(false) paths.
SHA-256 `18d9efb8b941b6be428d48400ed9390f0be46338d8f902d5c0f94f2b3d1aac6e` (33027 bytes).

<a id="source-copy-modern"></a>
**COPY-MODERN** — [Explorer++/Helper/FileOperations.cpp:191–267](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp#L191)

Separate NFileOperations::CopyFiles creates IFileOperation, sets owner/ALLOWUNDO, selects MoveItems/CopyItems and returns PerformOperations HRESULT.
SHA-256 `7b7d130e06ee6c457820fdf64a0d5bb8ca1f74faa0ca25522e16ee5e9bb7c9a5` (19056 bytes).

<a id="source-drop-interface"></a>
**DROP-INTERFACE** — [Explorer++/Explorer++/ShellBrowser/ShellBrowser.h:57–93](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.h#L57)

ShellBrowser explicitly inherits IDropTarget and declares DragEnter/DragOver/DragLeave/Drop overrides.
SHA-256 `d156801705efa9d89fb5874fdd5dd7f352d24919bfd4918af1fc68026baa9f92` (21099 bytes).

<a id="source-drop-enter"></a>
**DROP-ENTER** — [Explorer++/Explorer++/ShellBrowser/DropTarget.cpp:24–113](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/DropTarget.cpp#L24)

DragEnter checks formats and same drive, computes effect, records left/right drag type; DragOver recomputes effect and supplies visual/scroll work.
SHA-256 `5a5626082b09de6942d1368b123a40ebd7d50c2e90e9e7184482ee7068b39159` (20293 bytes).

<a id="source-drop-policy"></a>
**DROP-POLICY** — [Explorer++/Helper/ShellHelper.cpp:436–503](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/ShellHelper.cpp#L436)

DetermineDragEffect respects acceptance/allowed effects; Ctrl+Shift/Alt link, Shift move, Ctrl copy; default same-drive move else copy with available-effect fallback.
SHA-256 `90486967f22c8d8af15c8a2b5e0307324256995cb65df71d5bb4848b2f88c24d` (34145 bytes).

<a id="source-drop-route"></a>
**DROP-ROUTE** — [Explorer++/Explorer++/ShellBrowser/DropTarget.cpp:400–519](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/DropTarget.cpp#L400)

Drop computes child-folder destination, handles local left MOVE by RepositionLocalFiles, otherwise creates DropHandler and passes m_DragType/destination/callback.
SHA-256 `5a5626082b09de6942d1368b123a40ebd7d50c2e90e9e7184482ee7068b39159` (20293 bytes).

<a id="source-drop-enum"></a>
**DROP-ENUM** — [Explorer++/Helper/DropHandler.h:10–14](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/DropHandler.h#L10)

DragType declares LeftClick then RightClick, giving implicit 0 and 1.
SHA-256 `6bc129a99e33c19c204137883924885afd5ac9c1bac800720b917a8cddeaa1e2` (3185 bytes).

<a id="source-drop-mode"></a>
**DROP-MODE** — [Explorer++/Helper/DropHandler.cpp:83–107](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/DropHandler.cpp#L83)

Drop switches LeftClick to HandleLeftClickDrop, RightClick to HandleRightClickDrop.
SHA-256 `18d9efb8b941b6be428d48400ed9390f0be46338d8f902d5c0f94f2b3d1aac6e` (33027 bytes).

<a id="source-drop-left"></a>
**DROP-LEFT** — [Explorer++/Helper/DropHandler.cpp:126–211](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/DropHandler.cpp#L126)

Left-click drop reads preferred effect then selects HDROP, Shell ID list, file descriptors, text or DIBV5 format handlers; successful synchronous names trigger callback.
SHA-256 `18d9efb8b941b6be428d48400ed9390f0be46338d8f902d5c0f94f2b3d1aac6e` (33027 bytes).

<a id="source-drop-right"></a>
**DROP-RIGHT** — [Explorer++/Helper/DropHandler.cpp:852–883](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/DropHandler.cpp#L852)

Right-click handler binds destination, CreateViewObject(IDropTarget), calls DragEnter(MK_RBUTTON), Drop, DragLeave and Release in that order.
SHA-256 `18d9efb8b941b6be428d48400ed9390f0be46338d8f902d5c0f94f2b3d1aac6e` (33027 bytes).

<a id="source-drop-register"></a>
**DROP-REGISTER** — [Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp:459–477](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L459)

InitializeDragDropHelpers creates helpers, RegisterDragDrop(list view,this), compensates AddRef with Release.
SHA-256 `6f500469513936f2670daecf142d2bee4d4d046d416a3eb98903abbecfb8f77e` (37685 bytes).

<a id="source-drop-destroy"></a>
**DROP-DESTROY** — [Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp:158–173](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L158)

Destructor destroys list window, clears thread-pool queues and releases drag helpers; no explicit RevokeDragDrop call in this destructor.
SHA-256 `6f500469513936f2670daecf142d2bee4d4d046d416a3eb98903abbecfb8f77e` (37685 bytes).

<a id="source-property-entry"></a>
**PROPERTY-ENTRY** — [Explorer++/Explorer++/EventSwitcher.cpp:121–135](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/EventSwitcher.cpp#L121)

Properties command routes focused list to selected-file properties, tree to selected-tree properties.
SHA-256 `9c99c66800cfea4705d8a3d21448e3d8fa087b1fcfe4da08eb7fbe8dc92f1dd5` (5672 bytes).

<a id="source-property-selection"></a>
**PROPERTY-SELECTION** — [Explorer++/Explorer++/ShellBrowser/ListView.cpp:636–658](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ListView.cpp#L636)

Selected child PIDLs and directory PIDL passed to ShowMultipleFileProperties; empty selection returns.
SHA-256 `0617930db01d4c39c37b746b80fd45b8fdce168c74acf44bdb198b9a9d358bd2` (21749 bytes).

<a id="source-property-api"></a>
**PROPERTY-API** — [Explorer++/Helper/ShellHelper.cpp:1300–1362](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/ShellHelper.cpp#L1300)

ShowMultipleFileProperties requests properties verb; ExecuteActionFromContextMenu obtains IContextMenu for directory/selection, converts action to ANSI, invokes command, releases interface and returns HRESULT.
SHA-256 `90486967f22c8d8af15c8a2b5e0307324256995cb65df71d5bb4848b2f88c24d` (34145 bytes).

<a id="source-delete-entry"></a>
**DELETE-ENTRY** — [Explorer++/Explorer++/MainWndSwitch.cpp:386–399](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L386)

Delete and permanent-delete command cases call OnFileDelete(false/true).
SHA-256 `f6c2ef9bce117ee6d8eb1fc7bb617508ae8e52b99b6b5cf63db48e0360600e6f` (38021 bytes).

<a id="source-delete-selection"></a>
**DELETE-SELECTION** — [Explorer++/Explorer++/ListViewHandler.cpp:796–831](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp#L796)

OnListViewFileDelete builds selected absolute PIDLs and calls FileActionHandler.DeleteFiles(owner,pidls,permanent,false).
SHA-256 `4561b73e1099c32e135d7bcf580607b3d5b9bb8d635140bff36f0e1184187058` (31528 bytes).

<a id="source-delete-wrapper"></a>
**DELETE-WRAPPER** — [Explorer++/Helper/FileActionHandler.cpp:59–72](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileActionHandler.cpp#L59)

DeleteFiles delegates NFileOperations and records delete undo entry on SUCCEEDED(hr).
SHA-256 `5cd7b5e38ce5b7c3b19f8938327e1ea52458851fb47ab650fc31149ffd01e46b` (3276 bytes).

<a id="source-delete-api"></a>
**DELETE-API** — [Explorer++/Helper/FileOperations.cpp:64–152](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp#L64)

IFileOperation deletion adds ALLOWUNDO only when nonpermanent; silent adds SILENT/NOCONFIRMATION/NOERRORUI, nonsilent WANTNUKEWARNING; selected item array then DeleteItems+PerformOperations. No GetAnyOperationsAborted call in body.
SHA-256 `7b7d130e06ee6c457820fdf64a0d5bb8ca1f74faa0ca25522e16ee5e9bb7c9a5` (19056 bytes).

<a id="source-new-enum"></a>
**NEW-ENUM** — [Explorer++/Explorer++/INewMenuClient.h:22–29](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/INewMenuClient.h#L22)

NMCSAEI_SELECT=0; EDIT=1; EDIT_WINDOWS_10=3. Windows 10 value is a project testing comment, not a runtime observation made by this audit.
SHA-256 `6f71db6a6ee5b30e8d8409f0f8e04bed27b1201994aeeadf7e39ca94a4c9f6c7` (1261 bytes).

<a id="source-new-callback"></a>
**NEW-CALLBACK** — [Explorer++/Explorer++/INewMenuClient.cpp:81–103](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/INewMenuClient.cpp#L81)

SelectAndEditItem ignores SELECT, logs and QueueRename for EDIT/EDIT_WINDOWS_10.
SHA-256 `cab3d71ab156eb86f0261886d7ce291441470a89ca49071a736fe3b041e34bf5` (2395 bytes).

<a id="source-rename-queue"></a>
**RENAME-QUEUE** — [Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp:1259–1307](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/ShellBrowser.cpp#L1259)

QueueRename compares PIDLs to rows and ListView_EditLabel on match; otherwise clones PIDL and sets new-item pending flag.
SHA-256 `6f500469513936f2670daecf142d2bee4d4d046d416a3eb98903abbecfb8f77e` (37685 bytes).

<a id="source-rename-pending"></a>
**RENAME-PENDING** — [Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp:400–407](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ShellBrowser/BrowsingHandler.cpp#L400)

Item insertion compares pending PIDL, clears new-item flag on match and records inserted row index.
SHA-256 `256f19d1a78c138c92ccf08de0906d5726bfb9dba214d7bc528a334383c40093` (14295 bytes).

<a id="source-rename-pending-msg"></a>
**RENAME-PENDING-MSG** — [Explorer++/Explorer++/MsgHandler.cpp:877–885](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MsgHandler.cpp#L877)

OnShellNewItemCreated calls ListView_EditLabel on new row index; producer notification is in WATCH-UPDATE.
SHA-256 `38542cb20cc3061ae664b94388ec37c17c7d3e5215af05dcea4b6c6e52da9b22` (40846 bytes).

<a id="source-rename-edit-end"></a>
**RENAME-EDIT-END** — [Explorer++/Explorer++/ListViewHandler.cpp:407–531](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/ListViewHandler.cpp#L407)

End-label handler rejects cancellation/empty/trailing dot/forbidden characters, builds paths, preserves hidden extension, ignores unchanged name, trims trailing spaces, calls RenameFiles and shows warning if false.
SHA-256 `4561b73e1099c32e135d7bcf580607b3d5b9bb8d635140bff36f0e1184187058` (31528 bytes).

<a id="source-rename-commit"></a>
**RENAME-COMMIT** — [Explorer++/Helper/FileActionHandler.cpp:15–56](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileActionHandler.cpp#L15)

RenameFiles creates IShellItem from old path, strips destination basename, invokes NFileOperations::RenameFile per item, records SUCCEEDED items for undo, returns whether any succeeded.
SHA-256 `5cd7b5e38ce5b7c3b19f8938327e1ea52458851fb47ab650fc31149ffd01e46b` (3276 bytes).

<a id="source-rename-api"></a>
**RENAME-API** — [Explorer++/Helper/FileOperations.cpp:29–61](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp#L29)

RenameFile creates IFileOperation, sets ALLOWUNDO|SILENT, calls RenameItem and PerformOperations, returns HRESULT; no SetNameOf and no explicit aborted-query in this body.
SHA-256 `7b7d130e06ee6c457820fdf64a0d5bb8ca1f74faa0ca25522e16ee5e9bb7c9a5` (19056 bytes).

<a id="source-mkdir-entry"></a>
**MKDIR-ENTRY** — [Explorer++/Explorer++/MainWndSwitch.cpp:1178–1184](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainWndSwitch.cpp#L1178)

Toolbar/menu NewFolder command calls OnCreateNewFolder.
SHA-256 `f6c2ef9bce117ee6d8eb1fc7bb617508ae8e52b99b6b5cf63db48e0360600e6f` (38021 bytes).

<a id="source-mkdir-ui"></a>
**MKDIR-UI** — [Explorer++/Explorer++/MainMenuHandler.cpp:211–245](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/MainMenuHandler.cpp#L211)

OnCreateNewFolder creates destination IShellItem, observer selects/focuses and QueueRename, loads default name resource, calls NFileOperations::CreateNewFolder, releases sink and shows error on FAILED(hr).
SHA-256 `0a8225b9e6c09573752060cbc1fddd8ef7ec63416c25bc40385d6f95ea4dc09a` (9671 bytes).

<a id="source-mkdir-api"></a>
**MKDIR-API** — [Explorer++/Helper/FileOperations.cpp:293–329](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/FileOperations.cpp#L293)

CreateNewFolder uses IFileOperation ALLOWUNDO|SILENT, NewItem(FILE_ATTRIBUTE_DIRECTORY, default name, sink), PerformOperations. Comment assigns collision naming to IFileOperation.
SHA-256 `7b7d130e06ee6c457820fdf64a0d5bb8ca1f74faa0ca25522e16ee5e9bb7c9a5` (19056 bytes).

<a id="source-mkdir-sink"></a>
**MKDIR-SINK** — [Explorer++/Explorer++/FileProgressSink.cpp:165–204](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Explorer++/FileProgressSink.cpp#L165)

PostNewItem only calls observer after successful hrNew, interface acquisition and PIDL extraction.
SHA-256 `04b71c6fb790ad5765980822d3eff6701ec6773e41b2631de0820a54c9dc7559` (5918 bytes).

<a id="source-boost-package"></a>
**BOOST-PACKAGE** — [Explorer++/Helper/packages.config:3–11](../../verification/source/explorerplusplus-384c2f687fd55c1e71e9fcb272f9113de009a248/Explorer++/Helper/packages.config#L3)

NuGet dependency declarations include boost and boost_filesystem vc141/vc142, version 1.72.0.0. Dependency declaration does not prove exact shipped library object identity.
SHA-256 `d34c3d97a3949cb2723d1554238788cffde8512bfdd2b0b7daf76de29c3f7720` (1752 bytes).

## Search and verification boundary

Inspected frozen claims and prior shell/P1 findings, then the acquired source with `rg` and exact line reads. A bounded full-tree source search for `create_directory`/`create_directories` found no implementation/call spelling; package declarations provide Boost 1.72.0.0 context only. A bounded ShellBrowser source search found no explicit `RevokeDragDrop`. These are not universal absence claims.

JSON was reparsed and validated for exactly the last 12 initial IDs, byte-for-byte initial assertion/confidence preservation, valid source line ranges, current source hashes, and status counts. No runtime/provider/build tests were required or performed for this documentary audit.

External SDK derivation for the buffer expression: installed `winnt.h:15389–15394` declares three DWORDs plus `WCHAR FileName[1]`; normal Windows ABI size is 16 bytes, yielding 16000 for `1000*sizeof(FILE_NOTIFY_INFORMATION)`. SDK file SHA-256 `bc603675b330e624fd6f3de9ed3497ed6101e4c56e12cf819a9ef7c41f1c78d8`. The SDK is separate from target source.
