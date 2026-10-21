# Phase 3 공개 계약 확정

C++20/Qt Core 공개 헤더를 계약 소유자로 한다. 아래 이름·타입으로 Phase4P0를 구현하며 문서/구현 불일치가 발견되면 생산 경계에서 함께 수정한다. Phase2자료게이트는 ../analysis/phase2-validation.md에 기록했다. 원본 소스 미사용. ADR/architecture의 원칙은 유지하되 아래 작은 계약이 실제 초기 구현 범위다.

- `ErrorCode`: InvalidPath, NotFound, PermissionDenied, Unsupported, Io, Cancelled, Changed, Conflict, ResourceLimit. `Error`는 operation/input/message/nativeCode 및 partialPaths를 보존한다. `Result<T>`는 std::variant<T,Error>; 실패를 빈 성공 목록으로 바꾸지 않는다.
- `Location`: Kind(Drives,Directory), callerInput, path, key, label. Drives는 빈 path와 구별된 kind이며 Directory는 adapter가 파싱한 절대 lookup path를 갖는다. UI는 path를 다시 정규화하지 않는다. path/원문/key/display는 별도 값이다.
- `Entry`: key, path, name, typeName, directory, link, optional sizeBytes/modifiedMs, identity. unknown 메타데이터를 0/epoch로 바꾸지 않는다. identity는 adapter가 얻은 파일 object identity이며 비어 있으면 불명이다.
- `Listing`: location, entries, complete. `Progress`: examined, phase. list의 resource/cancel/열거 실패는 Error와 partialPaths로 반환하고 이전 complete 목록을 보존한다.
- `IPlatform`: parseLocation(QString)->Result<Location>, parent(Location)->Result<optional<Location>>, list(Location,stop_token,ProgressSink)->Result<Listing>, compareNames(QString,QString)->int(순수비교), open(Entry)->Result<Unit>, capabilities()->Capabilities. `makePlatform()`는 composition에서 실제 OS adapter를 만든다. Header에 Win32/COM/UI types 없음.
- Domain: History current/back/forward/visit/commitCursor; visit는 성공한 새 방문에만 호출한다. SortSpec(Name,Size,Modified;ascending), sortEntries(entries,spec,compareNames), filterEntries(entries,query). 폴더우선/unknown뒤/동률정책은 architecture.md의 새 제품 정책이며 원본일치검증과 구분한다.
- Application `ExplorerSession`: 탭 id별 location/history/listing/search/sort/selection/loading/error. createTab/closeTab/navigate/back/forward/up/refresh/setSearch/setSort/setSelection/openEntry/shutdown. signal tabChanged(id), tabsChanged(), shutdownFinished(). 진행 수는 공유 atomic counter를 UI timer가 읽는다. 모든 상태 변경은 QObject thread에서; platform의 경로 해석/상위 계산/열거/기본 앱 실행은 worker에서 수행한다. worker는 platform shared_ptr/값/stop_token만 capture한다. request generation+tab id를 readback시 확인한다. QFutureWatcher 연결에는 QObject context가 필수다. 종료시 취소를 요청하고 이미 시작한 앱 실행 위임을 포함한 남은 worker가 종료된 후 shutdownFinished를 낸다.
- Tree는 같은 IPlatform.list 계약의 directory항목만 별도 key/generation으로 lazy 요청한다. 파일 목록에서 QFileSystemModel로 이중열거하지 않는다. Icon은 자체 vector paint를 기본으로하고 실제OS아이콘도 platform안으로분리한다.

P1은 Phase4P0 검증 이후 동일 platform 경계에 OperationRequest/ItemOutcome/OperationReport를 추가한다. copy/move/recycle/permanent-delete/rename/mkdir를 typed구분하고 overwrite/영구삭제확인은UI에서 exactrequest대상으로받는다. sourceidentity재검사, 충돌명시, peritempartial/mixed/cancel/cleanup실패와원본보존은 필수다. 탭변경은mutationjob을취소하지않고applicationtaskowner가끝까지소유한다. 구현전관련contract를갱신하며FR11–20을범위에서제외하지않는다.

검증구조: pure domain+mockplatform asyncsemantics QtTest, realWinShell/POSIXadapter통합, 실제Qt위젯의 QTest이벤트, Windows프로세스기반패키지실행, Linux실제빌드/테스트,14필수시나리오. 제품성공은 문서확정으로입증되지않는다.

