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

COM 객체·PIDL·문자열·Windows handle은 호출 thread의 RAII 소유이며 다른 thread로 넘기지 않는다. `CoInitializeEx` 실패는 명시적 오류다. `makePlatform` 반환 객체 자체에는 apartment-bound 상태가 없다. 수정 기능의 capability는 전부 false이며 P1 계약/구현 단계에서 copy/move/recycle/rename/mkdir와 항목별 결과·충돌·확인 정책을 추가해야 한다. 이 기록은 P0 역공학 전체 성공, P1 완료 또는 실제 플랫폼 시험 통과 선언이 아니다.
