# P0 플랫폼 어댑터 구현 기록

대상 계약: `include/explorer/types.hpp`, `platform.hpp`. 구현 파일은 `src/platform/windows/platform.cpp`, `src/platform/posix/platform.cpp`이며 CMake는 실제 OS 파일 하나만 선택해야 한다. 공개 타입에는 Windows/COM/QtWidgets 타입이 없다. 이 작업에서는 **컴파일·실행·테스트하지 않았다**. 통합 작업이 실제 Windows/Linux build와 계약·GUI 시험을 수행해야 한다. 원본 소스와 실험 A 자료에 접근하지 않았다.

통합 작업의 후속 Windows build/platform 시험 통과 보고를 받아 SDK 경계를 수정했다. `NOMINMAX`는 CMake가 소유하도록 중복 정의를 제거했고, non-strict PIDL의 `__unaligned` qualifier 경고는 SDK `STRICT_TYPED_ITEMIDS`와 absolute/child별 정확한 RAII owner로 해결했다. qualifier를 버리는 cast는 추가하지 않았다. 이 수정 후 재빌드 결과는 통합 작업에서 기록해야 한다. 두 OS의 `parent`는 filesystem 검증 없는 lexical 계산이며 실제 존재/권한 검증은 후속 비동기 `list`가 소유한다. `Drives` 진입은 `Location{}` 값 계약이며 `drives` 문자열을 특수 경로 token으로 추가하지 않는다.

Windows는 REA [탐색 근거](../analysis/file-navigation.md)의 PIDL parse→desktop/bind→`EnumObjects`→`Next`→속성·표시 이름→`FindFirstFileW` 메타데이터 순서를 반영한다. 일반 filesystem 폴더·파일과 hidden 항목을 포함하고 Shell 가상 항목은 제외한다. 실제 파일 이름은 `SHGetNameFromIDList(SIGDN_FILESYSPATH)`의 경로에서 얻으므로 표시 설정의 확장자 숨김이 이름 검색을 바꾸지 않는다. Shell 표시 이름 조회도 수행하지만 현재 작은 Entry 계약에는 별도 표시 이름 필드가 없어 정상 파일 이름을 우선한다.

`list`는 `FILE_LIST_DIRECTORY` 접근을 먼저 요청해 권한 거부를 Shell의 빈 결과로 오인하지 않게 한다. `ERROR_ACCESS_DENIED`는 `PermissionDenied`다. 이 코드는 접근 거부 경로를 새로 읽은 증거가 아니다. 실제 통합 시험은 root가 정한 안전한 경로에서 수행해야 하며 `verification/source`의 ACL·내용을 수정하지 않는다. 빈 Shell enumerator는 이 preflight 성공 후에만 빈 성공이다.

identity는 `FILE_READ_ATTRIBUTES`, share read/write/delete, `OPEN_REPARSE_POINT|BACKUP_SEMANTICS` handle의 `FILE_ID_INFO(volume+file ID)`이며 실패하면 빈 unknown이다. 변경 시각/크기 조회 실패도 optional unknown으로 남긴다. reparse point 여부를 기록하고 열거 자체가 재귀적으로 자식을 따라가지 않는다. 긴 경로용 extended prefix는 Win32 호출 경계에 적용한다. Shell 자체의 장경로·특수 namespace 제한은 성공으로 숨기지 않으며 별도 실제 시험이 필요하다. Drive 목록은 `GetLogicalDriveStringsW`, root parent는 명시적 `Drives`이다.

Windows open은 현존 여부·identity를 재검사한 후 [Shell 근거](../analysis/shell-integration.md)의 PIDL 실행을 사용한다. SDK에서 `SEE_MASK_INVOKEIDLIST=0xC`이며 `IDLIST`를 이미 포함한다. 빈 verb를 유지한다. 새 구현은 worker의 COM apartment가 곧 해제되는 수명주기를 위해 `SEE_MASK_NOASYNC`를 추가했다. 이는 원본에서 관찰한 flag로 소급 주장하지 않는다. 이 flag의 의미는 [Microsoft 공식 계약](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-shellexecuteinfow)에 따른다. Shell delegation 성공은 대상 앱의 실제 창·파일 읽기 성공까지 증명하지 않는다. path 기반 OS 위임 전후의 외부 교체 race를 원자적으로 제거하지는 못한다.

POSIX는 **일반 API 기반**이다. `std::filesystem::directory_iterator(error_code)`와 `symlink_status`, `lstat`의 dev+ino identity·시각·크기를 사용한다. 링크는 식별하되 명시적 사용자의 directory-link 진입을 위해 대상이 directory인지 확인하며 자동 재귀는 하지 않는다. 표시 가능한 QString으로 filename bytes가 정확히 round-trip하지 않으면 원시 byte hex를 가진 `Unsupported` 오류를 돌려준다. 이름 비교는 worker별 최초 system locale을 고정한 numeric/case-insensitive `QCollator`다. Windows와 POSIX의 locale 정렬이 같다고 주장하지 않는다.

POSIX open은 identity 검사 후 `xdg-open` 또는 macOS `open`을 직접 argument list로 detached 실행한다. Helper 가용성을 capability로 보고한다. detached 시작 성공은 helper exit code·최종 연결 앱 성공을 확인한 결과가 아니다. 현재 POSIX `Drives`는 `/` 하나를 제공하며 mounted volume 전용 UI·native MIME type 이름은 구현하지 않았다. macOS build/run은 미검증이다.

두 어댑터는 항목별 stop check·progress와 **64 MiB 추정 listing 보관 한도**를 적용한다. 실패·취소·한도 초과는 Error+이미 검사한 partialPaths이며 불완전한 Listing을 성공으로 반환하지 않는다. 이 한도는 실제 heap peak 측정이 아니며 vector 여유 용량·allocator overhead·COM/Shell 내부 메모리를 엄밀히 제한하는 값은 아니다. 단일 OS 호출 도중의 강제 취소는 제공하지 않는다. 메모리 부족 예외와 파일시스템 호출의 극단적 지연은 통합 수명주기 시험에서 다뤄야 한다.

COM 객체·PIDL·문자열·Windows handle은 호출 thread의 RAII 소유이며 다른 thread로 넘기지 않는다. `CoInitializeEx` 실패는 명시적 오류다. `makePlatform` 반환 객체 자체에는 apartment-bound 상태가 없다. 이 기록은 P0 역공학 전체 성공, P1 완료 또는 실제 플랫폼 시험 통과 선언이 아니다.

## Phase 5 P1 어댑터 추가 — 2026-10-22

공통 작업 소유자는 `src/platform/common/file_operations.hpp/.cpp`다. Windows/POSIX adapter가 native primitive를 제공하고 한 adapter instance의 mutex가 수정 job과 cleanup ledger 접근을 직렬화한다. 공개 `OperationRequest`/`OperationReport`, `execute`/`retryCleanup` 계약을 구현했다. 이 추가 코드의 실제 build/test 결과는 통합 작업이 별도 기록한다. 기존 P0 Windows 9 pass/1 symlink-privilege skip 결과를 새 P1 검증 결과로 전용하지 않는다.

- Copy/Move는 단일·복수 파일 및 directory tree를 대상으로 한다. Rename은 source 한 개와 identity가 알려진 destination parent, 안전한 leaf name을 받는다. CreateDirectory는 source 없이 parent/name을 받는다. path separator, NUL, `.`/`..`, Windows reserved/trailing-invalid 이름을 거부한다. root/하위 경로로 directory를 복사·이동하지 않는다.
- `parseLocation`과 완전한 `list` 결과는 현재 directory identity를 채운다. 수정 작업은 source snapshot identity와 destination directory identity를 요구하고 항목 실행·commit 때 다시 검사한다. `inspectEntry`는 OS DnD의 로컬 경로를 worker에서 absolute lookup path·actual filename·identity·metadata·link 값으로 해석한다.
- Source tree preflight는 자동으로 링크/reparse point를 따라가지 않으며 조상에 있는 링크도 거부한다. 범위는 100,000 nodes, depth 256, manifest 추정 64 MiB다. 디렉터리 자식의 추가/교체와 파일 identity·size·고해상도 mtime 변화를 재검사한다. 한계를 넘기거나 부분 조사에 실패하면 성공으로 보지 않는다.
- Copy는 destination sibling에 exclusive UUID staging root와 자식을 만들고 native handle/fd로 1 MiB chunk를 읽고 쓴다. 열린 source와 staging handle의 identity를 확인한다. Windows는 source 쓰기/삭제 sharing을 허용하지 않고 POSIX는 `O_NOFOLLOW`/`fstat` 및 read 전후 identity/size/mtime 검사를 사용한다. 성공 후 파일 mtime을 보존하고 flush한다. POSIX regular-file permission bits도 보존한다. ACL/alternate streams/xattrs/creation time/디렉터리 timestamp 전체 보존은 구현하지 않았다.
- Commit은 **no-replace**다. Windows는 정확한 source handle로 `FILE_RENAME_INFO.ReplaceIfExists=FALSE`, **`RootDirectory=NULL`과 완전한 target path**를 전달한다([Microsoft 계약](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_rename_info)). destination directory handle은 identity 확인 뒤 호출이 끝날 때까지 열어 두어 해당 directory의 rename/delete를 막는다. source를 이름만으로 다시 지정하는 fallback은 없다. POSIX는 checked destination dirfd 기준 `renameat2(RENAME_NOREPLACE)` 또는 macOS `renameatx_np(RENAME_EXCL)`다. 지원되지 않는 filesystem에서는 `Unsupported`이며 check-then-overwriting-rename으로 fallback하지 않는다. Fail은 명시적 충돌이고 KeepBoth는 빈 이름을 찾아도 최종 no-replace commit을 거치므로 기존 대상을 조용히 덮어쓰지 않는다.
- Same-volume Move/Rename은 native no-replace rename이다. Cross-volume Move는 staged copy/commit 뒤 원래 manifest의 identity/내용 stamp를 재확인하고 원본을 정리한다. 정리 전·중 변화/취소/오류가 발생하면 destination과 남은 source 경로를 가진 `Partial`로 보고한다. 전체 move를 원자적이라고 부르지 않는다. Rename의 cross-volume fallback은 제공하지 않고 Move 사용을 안내한다.
- 취소된 항목과 아직 시작하지 않은 항목도 각각 결과를 남긴다. Copy 실패는 source를 삭제하지 않는다. Byte progress와 item counts를 전달하고 단일 source Copy는 tree의 알려진 전체 bytes를 제공한다. 여러 source의 global byte total은 사전 전체 계획 없이 확정하지 않으므로 unknown이다.
- 실패/취소의 staging cleanup은 생성한 node의 정확한 identity manifest만 제거한다. 재귀 `remove_all`이나 이름만의 소유 판정은 사용하지 않는다. 정리 실패면 원래 오류에 정리 실패 원인/native code를 덧붙이고 `retainedPaths`/`cleanupPending` 및 adapter ledger를 유지한다. `retryCleanup`은 ledger root만 받으며 다른 경로는 거부한다. 실제 `NotFound`가 확인된 owned node만 absent로 정산하고 permission/unknown/교체된 identity는 보존한다. Windows cleanup은 checked handle의 `FileDispositionInfo`를 사용한다. ledger는 현재 adapter instance 수명 안에서 유지하며 process crash/restart 복구 journal은 아직 없다.
- Recycle은 Qt `QFile::moveToTrash`만 사용한다. false이면 오류이며 permanent-delete fallback이 없다([Qt 공식 계약](https://doc.qt.io/qt-6.8/qfile.html#moveToTrash)). Trash API/volume가 지원되지 않을 수 있으므로 capability의 동작 가용성은 호출 결과와 구별한다. 실제 휴지통 동작을 원본의 복구된 delete 경로라고 주장하지 않는다.

POSIX C/POSIX locale에서 Qt6.4.2 `QCollator` numeric mode가 무시되어 `file2`/`file10` 통합 시험이 실패했다는 실제 통합 결과를 받아 비교 생산 경계를 수정했다. 숫자 run은 길이/숫자열로 overflow 없이 비교하고 나머지 text run만 thread-local locale collator가 소유한다. leading-zero 자연 동률은 domain의 source-name/key 규칙으로 해소한다. 시험 기대값을 약화하지 않았다. 수정 후 실행 결과는 다음 통합 기록으로 확인해야 한다.

동시 외부 변경에 관한 범위: Windows handle rename/cleanup은 열린 객체 identity에 묶이지만 directory 전체를 snapshot isolation하는 트랜잭션은 아니다. POSIX destination은 dirfd에 묶여도 source rename/unlink의 마지막 이름과 검사 사이 race가 남으며, Qt trash 또한 path 기반이다. 조상 교체·동시 악의적 쓰기를 전부 원자적으로 차단한다고 주장하지 않는다. POSIX copy의 size/mtime 재검사도 내용을 바꾸고 시각까지 복원하는 행위를 검출하는 hash 증명이 아니다. 정상 fixture·실제 충돌/취소/권한·변경 시험 범위와 이러한 잔여 한계를 함께 보고한다. 이 P1 구현은 기존 REA copy/move/drop 근거와 분리하여 **문서화된 일반 API 및 새 안전 정책 기반**으로 분류하며, [P1 조사 후보](../analysis/p1-investigation-leads.md)는 추가 원본 분석 질문을 소유한다.

### 실제 Windows commit 회귀와 경계 수정

통합 build `20261022T071529371`의 operation 결과는 5 pass / 6 fail / 3 skip이었다. 처음의 relative leaf + non-null `FILE_RENAME_INFO.RootDirectory`가 commit 경계에서 실패했다. 별도의 새 소유 `test-data/rename-boundary-*` 임시 fixture에서 같은 Win32 호출을 직접 확인하니 해당 형식은 **native 87(ERROR_INVALID_PARAMETER)**, null root + 완전한 extended target path는 **성공/native 0**이었다. 정확한 source handle과 checked destination handle 소유는 유지하고 target 표현의 생산 경계만 수정했다. 첫 diagnostic은 DELETE handle이 열린 동안 Python으로 결과를 다시 읽으려다 sharing 거부를 만났고, handle close 뒤 검증하도록 고친 재실행에서는 source 부재와 destination 내용 보존을 확인했다. 임시 fixture는 닫힌 handle과 확인된 소유 경로 안에서 정리했다.

후속 통합 `20261022T071839283`에서 Windows **5/5 suites 통과**, operation **11 pass / 0 fail / 3 skip**가 확인되었다. skip은 Windows symlink 권한 2개와 opt-in trash 1개이며 이 기능들의 실제 성공으로 세지 않는다. 이 결과는 실패를 숨긴 재분류가 아니라 native target 표현 수정 뒤의 별도 실행 결과다. 실제 전체 시나리오·cross-volume/정리 실패 재시도·desktop 파일 작업·패키지 검증은 각 ledger의 해당 범위를 추가로 확인해야 한다.


### 실제 Linux mount별 commit 한계와 보완

Linux 첫 P1 실행은 `build-linux/operation-results.txt`에 **8 pass / 5 fail / 1 skip**로 남아 있다. 이를 native Linux 전체 미지원 또는 성공으로 일반화하지 않는다. 실제 `/proc/self/mountinfo`에서 workspace `/mnt/c`는 **9p(DrvFS)**, `/tmp`는 root **ext4**임을 확인했다. 새 소유 임시 fixture의 `renameat2(RENAME_NOREPLACE)`는 DrvFS에서 파일·directory, absolute·destination-dirfd 네 조합 모두 **EINVAL(22)**였고 source는 보존됐다. 같은 directory primitive는 native ext4 `/tmp`에서 **성공(0)**이었다. 두 프로필의 검사 파일은 같은 SHA-256 `a9828532dd23d1765b21d9d3d60abee5812a75797eeb0f84519a7fcacb1b4275`를 유지했고 진단용 임시 경로는 제거했다.

일반 `rename`은 기존 대상을 덮어쓸 수 있으므로 fallback으로 추가하지 않았다. Linux regular file만 검증된 fd를 가리키는 `/proc/self/fd/<fd>`와 `linkat(AT_SYMLINK_FOLLOW)`로 **완전한 파일의 새 이름을 no-replace로 게시한 후** source를 identity/size/mtime 재검사하고 unlink한다. 여기서 follow하는 것은 caller의 symlink가 아니라 이미 확인한 kernel fd 경로다. [Linux linkat 계약](https://man7.org/linux/man-pages/man2/link.2.html)에 따른 게시 단계는 기존 이름을 덮어쓰지 않지만 link+unlink 전체가 atomic move는 아니다. unlink/검증 실패면 두 경로를 `Partial/retainedPaths`로 보존한다. native 진단에서 DrvFS 새 target 게시 성공0, 기존 target 충돌 EEXIST17과 원래 내용 보존, inode 일치 및 source unlink 후 내용 보존을 확인했다.

Directory는 hard-link fallback을 쓸 수 없다. 해당 mount에서 no-replace directory rename을 지원하지 않으면 **구체적 native code를 가진 Unsupported**로 반환하고 실패 staging을 소유권에 따라 정리한다. 따라서 이 DrvFS에서 directory Copy/Move commit과 staged mkdir는 현재 제한이다. Linux 사용자에게 Windows interop subprocess를 몰래 사용하거나 `rename`의 overwrite 위험으로 대체하지 않는다. native ext4 작업 검증과 DrvFS 제한을 함께 보고해야 한다.

이 보완 후 실제 `cmake --build /mnt/c/dev/Windows_fileexplore/build-linux --target operation_tests -j2`가 성공했다. 동일 workspace/DrvFS의 추가 실행 `build-linux/operation-drvfs-after-link.txt`는 **11 pass / 2 fail / 1 skip**이다. 단일·다중 파일 Copy/Move, KeepBoth, 충돌과 file Rename은 통과했고 남은 두 실패는 directory copy와 mkdir의 unsupported commit이다. trash opt-in skip은 성공으로 세지 않는다. 이 결과를 감추기 위해 기대값을 약화하지 않았고 원래 실패 로그도 보존했다. native ext4의 같은 계약 suite 결과는 별도 프로필 실행 후 기록해야 한다.

관련 공개 입력 경계도 보완했다. mkdir 외 작업의 빈 source는 빈 성공 report 대신 **실패 InvalidPath item 1개**를 반환한다. source 20,000개 또는 추정 source metadata 64 MiB를 넘는 요청은 **실패 ResourceLimit item 1개**로 거부한다. 이는 PoC 자원 정책이며 filesystem 형식 한계가 아니다. request 자체를 받은 뒤 추가 per-item 복사/실행을 하기 전에 계산하며 domain/session이 이미 보유한 입력 메모리 전체를 소급 제한하는 것은 아니다. 정상 범위의 요청은 각 source의 개별 결과를 유지한다.


Native ext4 계약 suite도 실제 실행했다. 최신 `operation_tests` target을 `-j2`로 빌드한 뒤 다음 두 명령을 별도로 실행했다.

```sh
env QT_QPA_PLATFORM=offscreen REA_OPERATION_TEST_ROOT=/tmp build-linux/operation_tests -o build-linux/operation-native-ext4.txt,txt
env QT_QPA_PLATFORM=offscreen build-linux/operation_tests -o build-linux/operation-drvfs-explicit.txt,txt
```

실제 명령에서는 build/output 경로를 `/mnt/c/dev/Windows_fileexplore/` 아래 absolute path로 지정했다. `REA_OPERATION_TEST_ROOT`는 테스트가 새 QTemporaryDir를 만들 parent만 선택하며 기존 parent 자체를 소유하거나 제거하지 않는다. 기본값은 기존 workspace의 test-data 그대로다. 공유 읽기 fixture와 컴파일된 코드를 바꾸거나 성공 기대값을 약화하지 않았다.

- [Native ext4 결과](../../build-linux/operation-native-ext4.txt): Qt6.4.2/Ubuntu24.04, sandbox `/tmp`, **14 pass / 0 fail / 1 skip**, QtTest elapsed **449 ms**. 새 empty-request 오류와 파일/폴더 copy/move, KeepBoth·실행중 충돌, rename/mkdir, identity 교체 거부, 취소/소유정리, 링크 no-follow가 통과했다.
- [DrvFS 결과](../../build-linux/operation-drvfs-explicit.txt): 동일 suite/code, sandbox `/mnt/c/dev/Windows_fileexplore/test-data`, **12 pass / 2 fail / 1 skip**, **2518 ms**. directory copy와 staged mkdir의 Unsupported 결과가 성공 기대와 달라 실패한 것이다. Native ext4 통과로 이 제한을 지우지 않는다.
- 양쪽 skip은 명시적 opt-in recycle 시험이다. 실제 trash 성공으로 세지 않는다. 전체 작업의 CPU/process-family memory는 이 실행에서 측정하지 않았다. QtTest elapsed를 앱 전체 개발/사용 시간으로 바꾸지 않는다.
- 실행 뒤 `/tmp`와 workspace test-data의 최상위 `runtime-operations-*` 경로를 한정 조회하여 남은 시험 디렉터리가 없음을 확인했다. original source gate와 A 자료는 읽지 않았다.
