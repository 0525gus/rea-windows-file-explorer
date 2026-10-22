# 사후 소스 검증 전 B 초기 결과

2026-10-22. 이 보고서는 원본 Explorer++ 소스를 다운로드하거나 읽기 **전**의 구현·주장 집합을 고정하기 위한 기록이다. 전체 프로젝트 최종 성공 판정은 아니다. 분석 대상, 전체 요구사항, 안전 조건 및 A/B 범위를 축소하지 않는다.

Phase 1의 실제 REA/Ghidra PE 함수·비어 있지 않은 의사코드·호출 근거를 확보했다. Phase 2는 P0 자료 또는 구체적 분석 한계를 갖추는 최소 게이트이며 전체 이벤트 매핑 복원 성공과 다르다. Phase 3 인터페이스와 Phase 4 Windows P0 게이트를 통과했다. Phase 5는 FR01–20의 Qt/공통 코어/OS adapter 구현, Windows/Linux 실제 실행과 검증으로 확장했다. [요구 매트릭스](requirements-matrix.md), [P1 검증](p1-validation.md), [UI 검토](ui-design-review.md)가 실제 결과와 아직 미검증인 범위를 소유한다.

REA 원자료에는 9,610개 함수와 4,962개 정의 문자열 inventory, native pseudocode와 참조, PE 구조·리소스, 7개 추가 P1 Evidence가 있다. [초기 주장 원장](../analysis/initial-claims.json)은 25개 조사 질문을 고정한다. 정적 관찰/도출/추론과 12개 unknown을 구분하며 **소스 대조 정확도는 아직 null**이다. 이후 source 지식을 이 초기 결과에 소급 반영하지 않는다. copy/move/drop의 일부 Shell 위임, PIDL 열거/metadata와 비교 함수, 조건부 검색 worker는 근거가 있지만 F2 commit·일반 tab dispatch·recycle 등 모든 경로를 복구하지 못했다.

제품은 Qt model을 쓰면서 자체 session/history/search/sort/selection과 비동기 작업 소유권을 둔다. Windows adapter는 documented Win32/Shell/COM, POSIX adapter는 표준 파일 API와 no-replace primitive를 사용한다. staging/no-overwrite/identity/retained cleanup, Qt clipboard/views/DnD 및 guaranteed-trash는 **새 일반 API 기반 정책**이며 원본 내부 구현 복원으로 세지 않는다. 파일 입력/기존 conflict·부분 결과·cleanup 소유권은 보존한다. crash/restart cleanup journal, 모든 cross-volume/악의적 동시 변경의 원자성은 제공하지 않는다.

원본 Explorer++의 실제 P0, 한글 mkdir·F2 rename·copy 동작을 관찰했고 B의 독립 fixture 결과와 비교했다. 결과 파일 이름과 hash가 일치한 경우에도 원본 inline edit와 B modal 등 UX 차이를 기록한다. 디자인 기준은 실제 Windows 11 탐색기이며 Explorer++ legacy UI가 아니다. 원본 아이콘/코드를 가져오지 않았다.

Windows/MSVC/Qt6.8.3와 Linux/Qt6.4.2/native ext4에서 실제 5/5 suites를 실행했다. 일반 Windows symlink 권한, OS-specific tests 및 기본 opt-in trash의 skips를 성공으로 세지 않는다. Windows junction/cycle·native cleanup failure/retry·identity replacement 거부, Linux 실제 symlink no-follow와 사용자 데이터 hash 보존을 검증했다. DrvFS directory publication의 2개 실패도 별도로 남는다. macOS는 [source/API 호환성 검토](macos-review.md)이며 실제 build/run 미검증이다. [정적 분석](static-analysis.md)은 Windows clang-tidy와 Qt 소유권 false positive, Linux GCC 분석의 C++ 지원 한계를 구분한다.

Windows package는 app-local Qt/CRT DLL, 필요한 plugin, 교체 가능한 동적 Qt, 라이선스/attribution과 앱 소스 및 파일별 hash를 포함한다. 최종 UI의 `dist/windows-20261022T090415366`을 Qt 개발 PATH/QT_PLUGIN_PATH 없이 **서로 다른 PID로 두 번** 실행해 Unicode 6개 snapshot/정상 종료를 확인했다. [처음](measurements/b-final-package-initial.metrics.json)과 [재실행](measurements/b-final-package-restart.metrics.json) 모두 exit0이며 이 독립 OS process restart를 Qt test의 fresh session과 구분한다. 앱 SHA256은 `bed10b64b0dbfb13606b91d8647f7c99f1062fff455a4730547f7d8d51e3c1fb`다. `Qt6Test.dll` 오류는 독립 테스트 실행의 PATH 누락에서 실제 발생했으며 CMake의 runtime DLL 배치로 수정해 clean-PATH 실행을 검증했다.

측정 기준은 고정 1,200개 fixture, 첫 listing+5 refresh, OS cache를 flush하지 않은 조건이다. Details 첫 paint의 약330ms gap은 실제 profile에 남겼다. app fallback 지정만으로는 해결되지 않았고 Windows Type 열에 설치된 Malgun Gothic을 명시한 후 [별도 probe](measurements/b-type-font-probe.json)는 최대19ms, slow event 0개, peak committed 28,020,736 bytes를 기록했다. 그러나 warm listing 956–1181ms로 모든 refresh가 사전 1초 목표를 만족하지 않았다. 최종 반복 결과와 원본 같은 작업 비교는 별도 성능 보고서에서 정리한다. OS cache·background load와 계측 경계 차이를 숨기거나 원본 내부 heartbeat를 추정하지 않는다.

A는 같은 상속 모델 설정·호스트·요구 기능의 fresh context에서 REA/B/source 입력 없이 독립 구현했고 초기 snapshot을 고정했다고 보고했다. B freeze 전에는 A 코드·설계·화면을 읽지 않는다. 정확 model deployment ID/가중치 동일성, A/B별 input/output token·API 청구 비용, 정밀 active time은 이 인터페이스에서 확인할 수 없다. 전역 goal 사용량이나 파일 bytes를 각 arm의 토큰/달러로 바꾸지 않는다. 실제 supervisor 단계 wall/CPU/peak process-family committed memory와 미측정 비용을 구분해 비교한다. REA 선행 분석 비용도 B 전체 비용에서 제외하지 않는다.

남은 필수 평가는 freeze 검증, 같은 버전 원본 소스의 사후 정확도 대조, A/B 산출물/기능/안정성/리소스 비교, 최종 보고서다. Native 앱 간 OS drag gesture는 도구 창 경계 때문에 미검증이며 external URL Qt integration과 실제 internal drag만 확인했다. 이것을 완전 검증으로 확대하지 않는다. Phase 6 접근은 승인된 ACL 백업 복원 절차와 **양쪽 초기 소스/보고서/시험/주장 파일의 hash 및 Git 고정**을 검증한 뒤에만 시작한다.
