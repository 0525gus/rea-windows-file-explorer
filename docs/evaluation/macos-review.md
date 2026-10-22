# macOS 공통 코드 호환성 검토 — 2026-10-22

**호환성 source review는 수행했고 macOS configure/build/link/run/package는 미검증**이다. 이 환경에는 확인된 macOS host, Xcode/Apple SDK, macOS Qt kit 또는 macOS 실행 결과가 없다. Linux 빌드와 Windows analyzer 성공을 macOS 실행 성공으로 전용하지 않는다. 검토는 B의 `CMakeLists.txt`, `include/explorer/`, `src/core/`, `src/ui/`, `src/main.cpp`, `src/platform/common/`, `src/platform/posix/` 및 공식 Apple/Qt 계약만 사용했다. A와 원본 Explorer++ 소스를 읽지 않았다.

## 재사용 경계

| 경계 | source에서 확인한 내용 | macOS에서 남은 검증 |
|---|---|---|
| Domain/contracts | C++20와 QtCore types, tagged Result/Error, per-tab history/filter/sort | AppleClang/libc++ 및 선택 Qt kit에서 실제 compile/link |
| Application | QtConcurrent, 최대 4개 worker, stop token, tab ID/generation, context-bound watcher completion | 실제 Cocoa event loop의 취소·닫기·late completion 및 OS 지연 |
| Shared operations | QtCore/std::filesystem 및 adapter native hooks, bounds/identity/staging/retained cleanup | APFS/HFS+/외장 볼륨, case/normalization, 충돌·부분 실패·권한 |
| Adapter 선택 | `if(WIN32)` 밖에서는 POSIX 파일을 선택; Linux headers/syscall 및 `/proc/self/fd` fallback은 `Q_OS_LINUX` 안 | 실제 Apple SDK 선언/feature macros 및 macOS native hook link |
| UI | Qt Widgets model/views, URLs/MIME DnD, dark Windows 11 스타일 | Cocoa plugin, HiDPI/Retina·입력기·접근성·font fallback·native clipboard/DnD |

Qt **6.4** 공식 표는 macOS 10.14–13, x86_64/x86_64h/arm64와 Xcode 13/14 구성을 나열한다. 이는 이 앱이 그 구성에서 검증됐다는 뜻이 아니다. `find_package(Qt6 6.4 ...)`는 최소 Qt 버전이며 최신 Qt kit의 macOS 최소 버전이나 libc++의 `std::stop_token` 지원을 확정하지 않는다. 실제 kit에 맞춰 compiler, SDK, deployment target, architecture를 기록해야 한다. [Qt 6.4 지원 구성](https://doc.qt.io/archives/qt-6.4/supported-platforms.html)

## macOS native 분기

- **시각/identity:** `Q_OS_MACOS`에서 `stat.st_atimespec`/`st_mtimespec`를 사용하고 identity는 `st_dev:st_ino`다. [Apple stat header](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/stat.h)의 Darwin field 명칭과 일치한다. source는 native stat을 `{}` 초기화하고 성공 후 읽는다. inode 재사용, 동시 변경 또는 모든 volume에서의 identity 안정성을 증명하지 않는다.
- **no-replace commit:** Linux `SYS_renameat2` 대신 macOS `renameatx_np(..., RENAME_EXCL)`를 선택한다. [Apple stdio header](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/stdio.h)는 해당 선언을 macOS **10.12**부터 제공하고 `RENAME_EXCL`을 선언한다. `<cstdio>` 경유 선언과 Darwin extensions 노출은 실제 Apple toolchain에서 확인해야 한다. [Apple의 volume capability 계약](https://developer.apple.com/documentation/foundation/urlresourcekey/volumesupportsexclusiverenamingkey)은 exclusive rename 지원이 volume 속성임을 보여 준다. 따라서 API 존재만으로 모든 filesystem 지원을 주장하지 않는다. 지원되지 않는 no-replace 호출은 `Unsupported`로 끝나며 overwrite 가능한 plain rename fallback이 없다. macOS는 Linux의 `/proc/self/fd` file fallback도 사용하지 않는다.
- **파일 copy:** `open`/`O_NOFOLLOW`, `fstat`, `read`/`write`, `fchmod`, `futimens`, `fsync`, `close`를 사용한다. [Apple stat header](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/stat.h)는 `futimens`를 macOS **10.13**부터 선언한다. API availability상 Qt 6.4 표의 최소 macOS 10.14보다 높지 않다는 source-derived 판단이다. 실제 SDK compile 또는 호출 성공 증거는 아니다. ACL, xattrs/resource forks, Finder metadata, creation time, directory timestamp 보존은 구현하지 않았다.
- **기본 앱 열기:** identity 확인 후 `QStandardPaths::findExecutable("open")`과 `QProcess::startDetached`의 argument list를 사용한다. shell command 문자열을 만들지 않는다. helper 미발견은 Unsupported, 시작 실패는 Io다. 시작 성공은 helper exit code, LaunchServices 처리 또는 최종 앱의 파일 열기 성공을 확인하지 않는다. macOS에서 실제 helper 실행은 하지 않았다.
- **휴지통:** `QFile::moveToTrash` 실패를 오류로 돌려주고 permanent-delete fallback을 호출하지 않는다. [Qt의 trash 계약](https://doc.qt.io/qt-6.8/qfile.html#moveToTrash)은 플랫폼/파일시스템 지원에 따른 실패를 허용한다. 실제 macOS trash·복원·권한·다중 항목 결과는 미검증이다.

위 Apple `main` header 확인은 2026-10-22의 공식 소스 계약 대조다. 로컬 Apple SDK를 확보하거나 컴파일한 증거가 아니며 product 코드를 가져온 것도 아니다.

## UI·filesystem 의미의 현재 제한

POSIX `Drives`는 `/` 하나를 보여 준다. Finder의 mounted volumes, network shares, sandbox bookmark/security-scoped URL, TCC 요청 또는 package bundle을 하나의 앱으로 다루는 의미는 구현하지 않았다. `.app`은 현재 일반 directory처럼 보이므로 Finder와 같은 activation 동작을 약속하지 않는다.

QFile의 native-name 변환 및 listing의 byte round-trip 검사와 `lstat` identity를 사용한다. APFS의 case-sensitive/insensitive 구성, NFC/NFD와 한국어/결합 문자, 대소문자만 바꾸는 rename은 실제 Mac에서 시험해야 한다. 공통 leaf 정책은 POSIX에서 허용되는 backslash도 separator로 거부하므로 macOS가 허용하는 모든 이름을 지원하는 설계가 아니다. Windows reserved-name 검사는 Windows adapter 정책에서만 적용한다. link/조상 link는 파일 작업에서 Unsupported이며 자동 재귀로 따라가지 않는다. 디렉터리 snapshot isolation, source rename/unlink 마지막 lookup의 외부 race 제거, crash 후 cleanup ledger 복원을 제공하지 않는다.

UI는 요구된 Windows 11 스타일을 공유하므로 macOS native Finder UI가 아니다. Segoe UI 지정은 해당 font가 없는 Mac에서 Qt fallback에 의존한다. 내부 clipboard token은 Copy/Move를 구별하지만 외부 cut/paste 판별에는 Windows Preferred DropEffect MIME를 사용하므로 Finder와의 native cut semantics를 구현했다고 주장하지 않는다. non-Windows external DnD가 Move만 허용하는 경우 sender와 receiver의 이중 source 삭제를 피하려고 거부하고, Copy acknowledgement 가능한 전송만 받는다. Finder/다른 앱과의 실제 DnD, keyboard modifier mapping, 다중 selection 및 취소는 미검증이다.

## 배포와 후속 실제 검증

CMake는 일반 `add_executable`과 runtime install만 사용한다. `MACOSX_BUNDLE`, 앱 bundle Info.plist, Qt frameworks/Cocoa plugin 배포, signing/notarization 또는 universal binary 산출물은 설정·검증하지 않았다. Qt는 배포 bundle과 필요한 plugins를 포함하는 `macdeployqt` 절차를 문서화한다. [Qt macOS 배포](https://doc.qt.io/qt-6.5/macos-deployment.html)

실제 Mac이 확보되면 선택 Qt/Xcode/SDK/architecture와 deployment target을 고정하고 configure/build/link부터 수행해야 한다. 이어 domain/session mock suite, native platform/operation suite, APFS의 파일·폴더 copy/move/rename/mkdir/충돌·identity 교체·권한·취소·cleanup retry, opt-in trash를 실행해야 한다. Unicode/normalization/case-only 이름, symlink loop, default app helper 결과, 3개 view·tabs/history/search·Cocoa DnD/clipboard·닫기, bundle을 새 사용자 환경에서 실행하는 검증이 남아 있다. 이 문서의 source review로 해당 실행 항목을 체크하지 않는다.
