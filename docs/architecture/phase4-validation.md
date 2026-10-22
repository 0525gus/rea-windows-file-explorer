# Phase 4 — P0 구현·시험 게이트

판정일: **2026-10-22, Asia/Seoul**. **FR-01~10의 Windows P0 구현과 아래 범위의 실제 시험을 통과하여 Phase 5 P1 구현을 시작할 수 있다.** 이는 전체 프로젝트 완료, 원본과의 완전한 일치, 모든 비기능·플랫폼 시나리오 통과 판정이 아니다. 자세한 범위·초기 실패·수정·미검증 사항은 [P0 검증표](../evaluation/p0-validation.md)가 소유한다.

C++20/Qt6 Widgets 제품을 실제 Windows11 x64에서 MSVC19.51, Qt6.8.3, CMake4.3.1/Ninja1.13.2로 빌드했다. 현재 검토한 실행파일은 `build-windows/rea_explorer.exe`, 276,480 bytes, SHA-256 `f0b78c077f60565f71a91d120a2a35ad48419e26ab996a8dbbe87398fff386f9`이며 마지막 링크 시각은 00:22:01 KST다. 이는 사후 소스 검증용 최종 freeze가 아니라 현재 게이트 산출물 식별이다.

## 직접 확인한 P0 범위

| 요구사항 | 구현과 실제 검증 |
|---|---|
| FR-01 기본 UI | 주소·탐색 트리·4열 목록·탭·상태줄. Qt 위젯/모델/resize 시험 및 실제 데스크톱 B-UI-001의 창·목록·메타데이터 확인 |
| FR-02 폴더 탐색 | 실제 Windows Shell adapter에서 빈 폴더, 1,200개 전체 이름/메타데이터, Unicode 목록 검증. B-UI-004~006에서 C: 트리 확장과 dev 즉시 자식 탐색 관찰 |
| FR-03 경로 이동 | Alt+D/Enter 주소 이동과 실제 Qt 목록 더블클릭 하위 폴더 이동 시험. B-UI-001에서 실제 데스크톱 주소 이동 |
| FR-04 기록/상위 | 뒤/앞/상위 실제 Qt 버튼, 성공 시 history commit, 실패 보존, forward 분기·refresh·경계 정책 시험 |
| FR-05 파일 정보 | 이름·크기·형식·수정일 모델/표시 열, manifest의 1,200개·Unicode 항목별 크기/시각 및 실제 화면 값 확인 |
| FR-06 정렬 | 이름/크기/날짜 × 양방향의 1,200개 **전체 순서** 독립 manifest oracle 비교. 폴더 우선, metadata 동률 이름 오름차순, domain unknown/동률 정책 시험 |
| FR-07 검색 | 현재 complete 폴더의 literal 이름 부분검색. Unicode/공백/메타문자 및 실제 Qt 2개·0개·전체 결과, 데스크톱 B-UI-003의 정확한 2개 결과 확인 |
| FR-08 다중 탭 | 생성·전환·닫기, 독립 경로/history/search/sort/selection, 3탭에서 비활성 탭 닫기 선택 보존, 닫힌 탭의 늦은 worker 결과 처리 시험 |
| FR-09 파일 실행 | production OS 위임 경계와 모의 async open/lifecycle 시험. 별도 데스크톱 B-UI-002에서 한글_파일.txt 더블클릭 후 실제 Notepad 제목 확인 |
| FR-10 오류 처리 | 실제 InvalidPath/NotFound, 잠긴 source gate의 PermissionDenied, 실제 임시 파일 외부 삭제·다른 identity로 교체 후 NotFound/Changed 거부. 기존 위치/목록/history 보존·복구 안내와 늦은 결과/종료 소유권 시험 |

최신 [00:24 공식 실행](../../build-windows/logs/20261022T002446646/test.stdout.log)은 CTest **4/4 suite 통과**다. named QtTest [domain](../../build-windows/domain-results.txt) **8 pass**, [session](../../build-windows/session-results.txt) **10 pass**, [platform](../../build-windows/platform-results.txt) **11 pass/1 skip**, [UI](../../build-windows/ui-results.txt) **12 pass**, 모두 0 fail이다. 합계 **41 pass/1 skip**에는 8개 init/cleanup이 포함되므로 기능 요구사항 41개 통과로 세지 않는다. Windows symlink 생성 권한 부족으로 링크/순환 시험은 **건너뜀·미검증**이며 성공이 아니다.

## 관찰과 새 제품 정책

[데스크톱 기록](../observations/rea-p0-runtime.json) B-UI-001~006은 실제 Computer Use 관찰이며 REA Evidence가 아니다. 001은 주소/Unicode 목록, 002는 한글 파일→Notepad, 003은 검색 2개, 004는 트리 확장이다. 005는 이전 빌드에서 폴더 이동 후 검색이 남아 0개로 보인 UX 문제를 **실패 관찰 그대로 보존**한다. 006은 검색을 지운 뒤 즉시 자식 4개가 표시된 복구 관찰이다.

이를 반영해 다른 Location으로 성공 이동하면 검색을 지우고, 같은 위치 refresh와 탭 전환은 해당 검색을 유지하도록 수정했다. 이 정책은 최신 session 시험에서 검증했으며 수정 후 동일 데스크톱 시나리오 재관찰까지 완료했다고 주장하지 않는다. 현재 폴더 literal 검색은 원본 SearchDialog의 옵션·재귀 기본값과 다른 요구사항 기반 설계다. 원본 검색의 그대로 복원이나 전체 동작 일치로 표현하지 않는다.

## 근거·소스·실험 경계와 다음 단계

Windows Shell 열거/PIDL 바인딩/메타데이터, natural 비교와 기본 OS 실행 위임의 일부는 실제 [REA 분석](../analysis/file-navigation.md) 및 [기능 근거 지도](../analysis/feature-map.md)를 사용했다. Win32/Qt API 구현, 공통 history/탭/오류/동률/검색 정책과 새 UI 설계는 각각 일반 API·관찰·새 설계로 분류한다. Qt 모델을 사용했다는 사실이나 시험 통과만으로 내부 역공학 성공을 확대하지 않는다.

Explorer++ 원본 소스는 이 초기 분석·구현에 사용하지 않았다. [source gate 상태](../analysis/source-gate-status.json)는 읽기/열거 거부와 원본 미확보를 기록하며 이번 실제 adapter 시험도 PermissionDenied를 확인했다. source gate를 열지 않았고 Phase6의 구현·보고서/freeze manifest 확인 전에는 복원·소스 열람하지 않는다. 같은 사용자/소유자가 ACL 복구 능력을 가지므로 적대적 사용자까지 차단하는 격리 증명은 아니다.

A/B는 같은 요구사항·toolchain·fixture와 상속된 모델 설정을 사용하며 별도 fresh context/디렉터리로 비교한다. 정확한 배포 ID·가중치·백엔드 설정의 동일성은 검증하지 못했다. B는 이미 REA 자료를 보았으므로 이 세션이나 B 설계/시험 해석을 A의 독립 입력으로 재사용하지 않는다. 현재 A/B 경계는 지침/입력 trace에 의존하며 동일 사용자 사이의 강제 ACL 격리나 A/B 효과·비용 평가 완료를 주장하지 않는다.

다음 단계는 전체 FR-11~20 P1 파일 작업/안전성, 표시 완성도, Windows 전용 코드 경계와 실제 Linux 빌드·시험이다. Windows 링크/순환 재검증, macOS 경계/가능한 실행, 모든 14필수 시나리오의 전체 범위, long path·permission·cancel/최종 정리·mixed outcome, 패키지/재실행, 원본과 동등 조건의 CPU/메모리/응답·cold/warm 비교, 모델/토큰/비용·독립 A/B·최종 소스 정확도 검증이 남아 있다. 최종 목표는 활성 상태이며 이 게이트가 전체 완료를 뜻하지 않는다.
