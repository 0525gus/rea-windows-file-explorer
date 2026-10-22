#include "shell_services.hpp"
#ifndef STRICT_TYPED_ITEMIDS
#define STRICT_TYPED_ITEMIDS
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <atomic>
#include <memory>
#include <vector>

namespace explorer::windows_shell {
namespace {
const wchar_t *wide(const QString &s) { return reinterpret_cast<const wchar_t *>(s.utf16()); }
struct Apartment {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ~Apartment() { if (SUCCEEDED(hr)) CoUninitialize(); }
    bool valid() const { return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE; }
};
template<class T> struct Release { void operator()(T *p) const { if (p) p->Release(); } };
template<class T> using Com = std::unique_ptr<T, Release<T>>;
struct Free { void operator()(void *p) const { CoTaskMemFree(p); } };
using Pidl = std::unique_ptr<ITEMIDLIST_ABSOLUTE, Free>;
Error error(const QString &action, const QString &path, HRESULT hr) {
    return {hr == E_ACCESSDENIED ? ErrorCode::PermissionDenied : ErrorCode::Io,
            action, path, QStringLiteral("%1 failed (HRESULT 0x%2)").arg(action).arg(quint32(hr), 8, 16, QChar('0')),
            qint64(hr), {}};
}
QString name(IShellItem *item, SIGDN kind) {
    wchar_t *raw = nullptr;
    if (FAILED(item->GetDisplayName(kind, &raw))) return {};
    std::unique_ptr<wchar_t, Free> memory(raw);
    return raw ? QString::fromWCharArray(raw) : QString();
}
QString parsingPath(const QString &path) {
    return path.startsWith("shell-item:") ? path.mid(11) : path;
}
Result<Pidl> pidl(const QString &path) {
    if (path.isEmpty() || path.contains(QChar::Null))
        return Error{ErrorCode::InvalidPath, "Resolve Shell item", path, "Invalid Shell location"};
    const auto parsing = parsingPath(path);
    PIDLIST_ABSOLUTE raw = nullptr;
    const HRESULT hr = SHParseDisplayName(wide(parsing), nullptr, &raw, 0, nullptr);
    if (FAILED(hr)) return error("Resolve Shell item", path, hr);
    return Pidl(raw);
}
Result<Com<IShellItem>> item(const QString &path) {
    IShellItem *raw = nullptr;
    const auto parsing = parsingPath(path);
    const HRESULT hr = SHCreateItemFromParsingName(wide(parsing), nullptr, IID_PPV_ARGS(&raw));
    if (FAILED(hr)) return error("Resolve Shell item", path, hr);
    return Com<IShellItem>(raw);
}
Result<Location> locationFromId(PCIDLIST_ABSOLUTE id) {
    IShellItem *raw = nullptr;
    const auto hr = SHCreateItemFromIDList(id, IID_PPV_ARGS(&raw));
    Com<IShellItem> shell(raw);
    if (FAILED(hr)) return error("Resolve Shell folder", {}, hr);
    SFGAOF attributes = 0;
    if (FAILED(shell->GetAttributes(SFGAO_FOLDER, &attributes)) || !(attributes & SFGAO_FOLDER))
        return Error{ErrorCode::InvalidPath, "Shell location", {}, "This Shell item is not a folder"};
    const auto parsing = name(shell.get(), SIGDN_DESKTOPABSOLUTEPARSING);
    const auto path = ILIsEmpty(id) ? QStringLiteral("shell:Desktop") : QStringLiteral("shell-item:") + parsing;
    return Location{Location::Kind::Shell, path, path, path,
                    name(shell.get(), SIGDN_NORMALDISPLAY), {}};
}
QString apiPath(const QString &path) {
    const auto native = QDir::toNativeSeparators(path);
    if (native.startsWith(QStringLiteral("\\\\?\\"))) return native;
    if (native.startsWith(QStringLiteral("\\\\"))) return QStringLiteral("\\\\?\\UNC\\") + native.mid(2);
    return QStringLiteral("\\\\?\\") + native;
}
QString fileIdentity(const QString &path) {
    HANDLE h = CreateFileW(wide(apiPath(path)), FILE_READ_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return {};
    FILE_ID_INFO id{};
    const bool ok = GetFileInformationByHandleEx(h, FileIdInfo, &id, sizeof(id));
    CloseHandle(h);
    if (!ok) return {};
    return QString::number(id.VolumeSerialNumber, 16) + ':' +
        QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(id.FileId.Identifier), 16).toHex());
}
// Receives exact per-item completion and the new Recycle Bin/restore item.
class Sink final : public IFileOperationProgressSink {
    std::atomic<ULONG> refs_{1};
  public:
    HRESULT status = E_PENDING;
    QString parsing, path;
    bool recycleOnly = false;
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override { auto n = --refs_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == IID_IFileOperationProgressSink) {
            *out = static_cast<IFileOperationProgressSink *>(this); AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    void completed(HRESULT hr, IShellItem *created) {
        status = hr;
        if (SUCCEEDED(hr) && created) {
            parsing = name(created, SIGDN_DESKTOPABSOLUTEPARSING);
            path = name(created, SIGDN_FILESYSPATH);
        }
    }
    HRESULT STDMETHODCALLTYPE StartOperations() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD, IShellItem *, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD, IShellItem *, LPCWSTR, HRESULT hr, IShellItem *p) override { completed(hr,p); return S_OK; }
    HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT hr, IShellItem *p) override { completed(hr,p); return S_OK; }
    HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD flags, IShellItem *) override {
        // TSF_DELETE_RECYCLE_IF_POSSIBLE: refuse a permanent-delete fallback.
        return recycleOnly && !(flags & TSF_DELETE_RECYCLE_IF_POSSIBLE) ? E_ABORT : S_OK;
    }
    HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD, IShellItem *, HRESULT hr, IShellItem *p) override { completed(hr,p); return S_OK; }
    HRESULT STDMETHODCALLTYPE PreNewItem(DWORD, IShellItem *, LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PostNewItem(DWORD, IShellItem *, LPCWSTR, LPCWSTR, DWORD, HRESULT, IShellItem *) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE UpdateProgress(UINT, UINT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResetTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE PauseTimer() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ResumeTimer() override { return S_OK; }
};
struct MenuHost { IContextMenu2 *two = nullptr; IContextMenu3 *three = nullptr; };
LRESULT CALLBACK menuProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) {
    auto *host = reinterpret_cast<MenuHost *>(data);
    if (msg == WM_INITMENUPOPUP || msg == WM_DRAWITEM || msg == WM_MEASUREITEM || msg == WM_MENUCHAR) {
        LRESULT result = 0;
        if (host->three && host->three->HandleMenuMsg2(msg, wp, lp, &result) == S_OK) return result;
        if (host->two && host->two->HandleMenuMsg(msg, wp, lp) == S_OK) return 0;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}
}
Result<Location> parse(const QString &input) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", input, apartment.hr);
    auto parsed = pidl(input);
    if (auto *e = failure(parsed)) return *e;
    auto result = locationFromId(std::get<Pidl>(parsed).get());
    if (!failure(result)) std::get<Location>(result).callerInput = input;
    return result;
}
Result<std::optional<Location>> parent(const Location &location) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", location.path, apartment.hr);
    auto parsed = pidl(location.path);
    if (auto *e = failure(parsed)) return *e;
    auto &id = std::get<Pidl>(parsed);
    if (ILIsEmpty(id.get())) return std::optional<Location>{};
    if (!ILRemoveLastID(id.get())) return std::optional<Location>{};
    auto result = locationFromId(id.get());
    if (auto *e = failure(result)) return *e;
    return std::optional<Location>{std::get<Location>(result)};
}
Result<Listing> list(const Location &location, std::stop_token stop, ProgressSink progress,
                     const std::function<void(Entry &)> &metadata) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", location.path, apartment.hr);
    auto parsed = pidl(location.path);
    if (auto *e = failure(parsed)) return *e;
    auto &id = std::get<Pidl>(parsed);
    IShellFolder *raw = nullptr;
    auto hr = SHBindToObject(nullptr, id.get(), nullptr, IID_PPV_ARGS(&raw));
    Com<IShellFolder> folder(raw);
    if (FAILED(hr)) return error("Bind Shell folder", location.path, hr);
    IEnumIDList *enumRaw = nullptr;
    hr = folder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &enumRaw);
    Com<IEnumIDList> enumerator(enumRaw);
    if (FAILED(hr)) return error("Enumerate Shell folder", location.path, hr);
    Listing listing{location, {}, true};
    if (!enumerator) return listing;
    quint64 bytes = 0;
    while (true) {
        if (stop.stop_requested()) return Error{ErrorCode::Cancelled, "Shell listing", location.path, "Listing cancelled"};
        PITEMID_CHILD childRaw = nullptr;
        ULONG fetched = 0;
        hr = enumerator->Next(1, &childRaw, &fetched);
        std::unique_ptr<ITEMID_CHILD, Free> child(childRaw);
        if (FAILED(hr)) return error("Enumerate Shell item", location.path, hr);
        if (!fetched) break;
        IShellItem *itemRaw = nullptr;
        hr = SHCreateItemWithParent(id.get(), folder.get(), child.get(), IID_PPV_ARGS(&itemRaw));
        Com<IShellItem> shell(itemRaw);
        if (FAILED(hr)) return error("Create Shell item", location.path, hr);
        Entry entry;
        SFGAOF attributes = 0;
        hr = shell->GetAttributes(SFGAO_FOLDER | SFGAO_LINK, &attributes);
        if (FAILED(hr)) return error("Shell item attributes", location.path, hr);
        entry.name = name(shell.get(), SIGDN_NORMALDISPLAY);
        entry.path = name(shell.get(), SIGDN_FILESYSPATH);
        const auto parsing = name(shell.get(), SIGDN_DESKTOPABSOLUTEPARSING);
        entry.shell = entry.path.isEmpty() || ((attributes & SFGAO_FOLDER) && !QFileInfo(entry.path).isDir());
        if (entry.shell) entry.path = "shell-item:" + parsing;
        entry.key = parsing;
        entry.identity = "shell:" + parsing;
        entry.directory = attributes & SFGAO_FOLDER;
        entry.link = attributes & SFGAO_LINK;
        entry.typeName = entry.directory ? "Shell folder" : "Shell item";
        if (!entry.shell) {
            metadata(entry);
            // Recycle Bin keeps the original display name, not its $R filename.
            entry.key = parsing;
        }
        bytes += sizeof(Entry) + 2ULL * (entry.path.size() + entry.key.size() + entry.name.size() + entry.identity.size());
        if (bytes > 64ULL * 1024 * 1024)
            return Error{ErrorCode::ResourceLimit, "Shell listing", location.path, "Shell listing exceeds 64 MiB"};
        listing.entries.push_back(std::move(entry));
        if (progress) progress({quint64(listing.entries.size()), "Reading Shell folder"});
    }
    return listing;
}
Result<Unit> open(const Entry &entry) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", entry.path, apartment.hr);
    auto parsed = pidl(entry.path);
    if (auto *e = failure(parsed)) return *e;
    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOASYNC;
    info.lpIDList = std::get<Pidl>(parsed).get();
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&info)) return error("Open Shell item", entry.path, HRESULT_FROM_WIN32(GetLastError()));
    return Unit{};
}
Result<Unit> menu(const Location &location, const QVector<Entry> &entries, quintptr owner, QPoint point) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", location.path, apartment.hr);
    const auto hwnd = reinterpret_cast<HWND>(owner);
    IContextMenu *menuRaw = nullptr;
    std::vector<Pidl> ids;
    std::vector<PCIDLIST_ABSOLUTE> pointers;
    Com<IShellItemArray> array;
    if (!entries.isEmpty()) {
        for (const auto &entry : entries) {
            if (!entry.shell && !entry.identity.isEmpty() && fileIdentity(entry.path) != entry.identity)
                return Error{ErrorCode::Changed, "Shell menu", entry.path, "Item changed; refresh before opening its menu"};
            auto parsed = pidl(entry.key.startsWith("::") ? entry.key : entry.path);
            if (auto *e = failure(parsed)) return *e;
            ids.push_back(std::move(std::get<Pidl>(parsed)));
            pointers.push_back(ids.back().get());
        }
        IShellItemArray *raw = nullptr;
        auto hr = SHCreateShellItemArrayFromIDLists(UINT(pointers.size()), pointers.data(), &raw);
        array.reset(raw);
        if (SUCCEEDED(hr)) hr = array->BindToHandler(nullptr, BHID_SFUIObject, IID_PPV_ARGS(&menuRaw));
        if (FAILED(hr)) return error("Create Shell context menu (select items from one folder)", location.path, hr);
    } else {
        auto parsed = pidl(location.kind == Location::Kind::Drives ? QStringLiteral("shell:MyComputerFolder") : location.path);
        if (auto *e = failure(parsed)) return *e;
        IShellFolder *raw = nullptr;
        auto hr = SHBindToObject(nullptr, std::get<Pidl>(parsed).get(), nullptr, IID_PPV_ARGS(&raw));
        Com<IShellFolder> folder(raw);
        if (SUCCEEDED(hr)) hr = folder->CreateViewObject(hwnd, IID_PPV_ARGS(&menuRaw));
        if (FAILED(hr)) return error("Create background Shell menu", location.path, hr);
    }
    Com<IContextMenu> context(menuRaw);
    IContextMenu2 *twoRaw = nullptr;
    IContextMenu3 *threeRaw = nullptr;
    context->QueryInterface(IID_PPV_ARGS(&twoRaw));
    context->QueryInterface(IID_PPV_ARGS(&threeRaw));
    Com<IContextMenu2> two(twoRaw);
    Com<IContextMenu3> three(threeRaw);
    MenuHost host{two.get(), three.get()};
    HMENU popup = CreatePopupMenu();
    if (!popup) return error("Create popup menu", location.path, HRESULT_FROM_WIN32(GetLastError()));
    const UINT flags = CMF_NORMAL | ((GetKeyState(VK_SHIFT) & 0x8000) ? CMF_EXTENDEDVERBS : 0);
    const auto hr = context->QueryContextMenu(popup, 0, 1, 0x7fff, flags);
    if (FAILED(hr)) { DestroyMenu(popup); return error("Populate Shell menu", location.path, hr); }
    const UINT_PTR subclassId = reinterpret_cast<UINT_PTR>(&host);
    const bool subclassed = SetWindowSubclass(hwnd, menuProc, subclassId, reinterpret_cast<DWORD_PTR>(&host));
    POINT native{};
    // GetCursorPos uses physical coordinates, including mixed-DPI monitors.
    if (!GetCursorPos(&native)) { native.x = point.x(); native.y = point.y(); }
    const auto command = TrackPopupMenuEx(popup, TPM_RETURNCMD | TPM_RIGHTBUTTON, native.x, native.y, hwnd, nullptr);
    if (subclassed) RemoveWindowSubclass(hwnd, menuProc, subclassId);
    DestroyMenu(popup);
    if (!command) return Unit{};
    CMINVOKECOMMANDINFOEX invoke{};
    invoke.cbSize = sizeof(invoke);
    invoke.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
    invoke.hwnd = hwnd;
    invoke.lpVerb = MAKEINTRESOURCEA(command - 1);
    invoke.lpVerbW = MAKEINTRESOURCEW(command - 1);
    invoke.nShow = SW_SHOWNORMAL;
    invoke.ptInvoke = native;
    const auto invoked = context->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&invoke));
    if (FAILED(invoked)) return error("Invoke Shell command", location.path, invoked);
    return Unit{};
}
QImage thumbnail(const Entry &entry, int edge) {
    Apartment apartment;
    if (!apartment.valid()) return {};
    IShellItemImageFactory *raw = nullptr;
    const auto hr = SHCreateItemFromParsingName(wide(entry.shell ? parsingPath(entry.path) : QDir::toNativeSeparators(entry.path)),
                                                nullptr, IID_PPV_ARGS(&raw));
    Com<IShellItemImageFactory> factory(raw);
    if (FAILED(hr)) return {};
    HBITMAP bitmap = nullptr;
    if (FAILED(factory->GetImage({edge, edge}, SIIGBF_RESIZETOFIT, &bitmap)) || !bitmap) return {};
    BITMAP info{};
    if (!GetObjectW(bitmap, sizeof(info), &info)) { DeleteObject(bitmap); return {}; }
    if (info.bmWidth <= 0 || info.bmWidth > 2048 || info.bmHeight == 0 || std::abs(qint64(info.bmHeight)) > 2048) {
        DeleteObject(bitmap); return {};
    }
    QImage image(info.bmWidth, std::abs(info.bmHeight), QImage::Format_ARGB32);
    if (image.isNull()) { DeleteObject(bitmap); return {}; }
    image.fill(Qt::transparent);
    BITMAPINFO format{};
    format.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    format.bmiHeader.biWidth = image.width();
    format.bmiHeader.biHeight = -image.height();
    format.bmiHeader.biPlanes = 1;
    format.bmiHeader.biBitCount = 32;
    format.bmiHeader.biCompression = BI_RGB;
    HDC dc = GetDC(nullptr);
    const bool ok = GetDIBits(dc, bitmap, 0, image.height(), image.bits(), &format, DIB_RGB_COLORS) != 0;
    ReleaseDC(nullptr, dc);
    DeleteObject(bitmap);
    if (!ok) return {};
    // Some icon handlers return opaque RGB with a zero alpha channel.
    bool alpha = false;
    for (int y = 0; y < image.height() && !alpha; ++y)
        for (int x = 0; x < image.width(); ++x)
            if (qAlpha(image.pixel(x,y))) { alpha = true; break; }
    if (!alpha) image = image.convertToFormat(QImage::Format_RGB32);
    return image;
}
Result<platform_detail::TrashReceipt> recycle(const QString &path, const QString &expectedIdentity) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", path, apartment.hr);
    if (fileIdentity(path) != expectedIdentity)
        return Error{ErrorCode::Changed, "Recycle", path, "Source identity changed"};
    // Query capability first: do not accept a filesystem that cannot recycle.
    SHQUERYRBINFO bin{}; bin.cbSize = sizeof(bin);
    wchar_t root[32768]{};
    if (!GetVolumePathNameW(wide(apiPath(path)), root, 32768))
        return error("Find Recycle Bin volume", path, HRESULT_FROM_WIN32(GetLastError()));
    auto volume = QString::fromWCharArray(root);
    if (volume.startsWith(QStringLiteral("\\\\?\\"))) volume.remove(0, 4);
    if (GetDriveTypeW(wide(volume)) == DRIVE_REMOTE || FAILED(SHQueryRecycleBinW(wide(volume), &bin)))
        return Error{ErrorCode::Unsupported, "Recycle", path, "This volume does not provide a Recycle Bin"};
    auto source = item(path);
    if (auto *e = failure(source)) return *e;
    IFileOperation *raw = nullptr;
    auto hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&raw));
    Com<IFileOperation> operation(raw);
    if (FAILED(hr)) return error("Create recycle operation", path, hr);
    Com<Sink> sink(new Sink);
    sink->recycleOnly = true;
    hr = operation->SetOperationFlags(FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT |
                                      FOFX_EARLYFAILURE | FOFX_RECYCLEONDELETE);
    if (SUCCEEDED(hr)) hr = operation->DeleteItem(std::get<Com<IShellItem>>(source).get(), sink.get());
    if (SUCCEEDED(hr)) hr = operation->PerformOperations();
    BOOL aborted = FALSE;
    operation->GetAnyOperationsAborted(&aborted);
    if (FAILED(hr) || aborted || FAILED(sink->status))
        return error("Recycle", path, FAILED(hr) ? hr : aborted ? E_ABORT : sink->status);
    if (sink->path.isEmpty() || sink->parsing.isEmpty())
        return Error{ErrorCode::Io, "Recycle", path, "Shell returned no restorable Recycle Bin receipt", 0, {path}};
    return platform_detail::TrashReceipt{sink->parsing, sink->path, {}};
}
Result<QString> restore(const platform_detail::TrashReceipt &receipt, const QString &target) {
    Apartment apartment;
    if (!apartment.valid()) return error("COM initialization", target, apartment.hr);
    if (QFileInfo::exists(target)) return Error{ErrorCode::Conflict, "Restore", target, "Restore destination already exists"};
    auto source = item(receipt.token), destination = item(QFileInfo(target).absolutePath());
    if (auto *e = failure(source)) return *e;
    if (auto *e = failure(destination)) return *e;
    if (name(std::get<Com<IShellItem>>(source).get(), SIGDN_FILESYSPATH).compare(receipt.path, Qt::CaseInsensitive) != 0)
        return Error{ErrorCode::Changed, "Restore", receipt.path, "Recycle Bin receipt changed"};
    IFileOperation *raw = nullptr;
    auto hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&raw));
    Com<IFileOperation> operation(raw);
    if (FAILED(hr)) return error("Create restore operation", target, hr);
    Com<Sink> sink(new Sink);
    hr = operation->SetOperationFlags(FOF_RENAMEONCOLLISION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SILENT | FOFX_EARLYFAILURE);
    const auto leaf = QFileInfo(target).fileName();
    if (SUCCEEDED(hr)) hr = operation->MoveItem(std::get<Com<IShellItem>>(source).get(),
        std::get<Com<IShellItem>>(destination).get(), wide(leaf), sink.get());
    if (SUCCEEDED(hr)) hr = operation->PerformOperations();
    BOOL aborted = FALSE; operation->GetAnyOperationsAborted(&aborted);
    if (FAILED(hr) || aborted || FAILED(sink->status))
        return error("Restore", target, FAILED(hr) ? hr : aborted ? E_ABORT : sink->status);
    return sink->path.isEmpty() ? target : sink->path;
}
}
