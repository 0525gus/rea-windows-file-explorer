# 요구사항·검증 근거 매트릭스

기준 문서: [`../requirements.md`](../requirements.md)의 전체 요구사항. 이 문서는 범위를 줄이거나 명세를 대체하지 않는다. `계획됨`은 완료가 아니다. 구현, 테스트 통과, 역공학 성공은 서로 다른 판정이며 각각 실제 근거가 있어야 한다.

## 현재 기준 상태

- 갱신: **2026-10-22, Asia/Seoul**. Phase 1 실제 PE 분석, Phase 2 자료/한계 정리의 최소 게이트, Phase 3 모듈/계약 게이트는 각각 [Phase 1](../analysis/phase1-validation.md), [Phase 2](../analysis/phase2-validation.md), [Phase 3](../architecture/phase3-validation.md)에서 통과했다. **Phase 4 P0 검증은 진행 중**이며 전체 프로젝트 완료가 아니다.
- 동일 공식 Explorer++ 1.4.0.2293 x64, SHA-256 `22ac20341c5525658ebe5e9c4b9ac9f7c152b18080ff347861fac3f04d56a2d6`를 사용한다([manifest](../analysis/target-manifest.json)). REA/Ghidra 실제 함수·의사코드·참조, 전체 9,610개 함수/4,962개 정의 문자열 inventory, 리소스 트리와 분석 한계를 [분석 문서](../analysis/binary-overview.md) 및 정본 bundle에 보존했다. 바이트 파서 보충 자료를 REA Evidence로 바꾸지 않는다.
- 제품 Windows C++20/MSVC/Qt6.8.3 빌드가 성공했고 최신 [CTest 로그](../../build-windows/Testing/Temporary/LastTest.log)는 **4/4 suite 통과**다. 개별 QtTest 결과는 [domain](../../build-windows/domain-results.txt) 8 pass, [session](../../build-windows/session-results.txt) 10 pass, [platform](../../build-windows/platform-results.txt) **9 pass/1 skip**, [UI](../../build-windows/ui-results.txt) 10 pass이다(init/cleanup 포함). 플랫폼 skip은 Windows symlink 생성 권한 부재이며 T-13 통과로 세지 않는다. UI suite는 OFFSCREEN이며 native desktop/폰트/OS 앱 실행을 입증하지 않는다.
- 실제 Windows desktop의 [B 실행 기록](../observations/rea-p0-runtime.json)에는 주소 Enter로 Unicode 폴더 열기(B-UI-001), `한글_파일.txt` 더블클릭 후 `한글_파일.txt - 메모장` 창(B-UI-002), 한글 검색의 정확한 2개 파일 결과(B-UI-003)가 있다. [실제 항목 snapshot](../observations/rea-p0-unicode.json)은 6개 항목의 이름/size/time/identity를 보존한다. 이는 native REA 호출 주소 추적이 아니며 내부 이벤트 경로 복원을 새로 증명하지 않는다.
- [원본 공통 관찰](../observations/behavior-summary.md)은 안정된 주소/빈·대량·Unicode/정렬/검색/탭/뒤·앞·상위/기본 앱 실행을 포함한다. 동일 fixture [원본](../observations/screenshots/explorerpp-unicode-details.png), [Win11 참고](../observations/screenshots/windows11-reference.png), [재구현](../observations/screenshots/rea-p0-unicode.png)을 보존했다. UI 디자인 기준은 Win11이며 원본 Explorer++의 legacy UI가 아니다. 원본 관찰의 1,200항목 상태 표시와 재구현의 전체 1,200이름 oracle 검사를 구분한다.
- [소스 게이트](../analysis/source-gate-status.json)는 현재 SID의 실제 directory-list/probe-read 거부를 확인했고 잠금 상태다. 원본 소스는 다운로드·열람하지 않았다. Phase 6의 verified freeze manifest 전 unlock하지 않는다. 소유자/관리자의 ACL 복원 능력까지 차단하는 보안 격리는 아니다.
- A는 상위 작업의 fresh-context/별도 디렉터리 시작 보고가 있으며 이 갱신에서는 A 소스·설계·출력을 읽지 않았다. A/B의 동일 사용자 파일시스템은 강제 ACL 격리로 입증되지 않았다. 상속 모델만으로 실제 모델 배포 ID·토큰·과금/설정의 동등성까지 확정하지 않는다. 비교 실험 결과는 아직 없다.
- Linux는 Qt6.4.2 환경에서 [CMake configure cache](../../build-linux/CMakeCache.txt)를 확보했으나 이 갱신 시점의 **B 제품 build/test/run은 미검증**이다. macOS도 미검증이다. P1, 패키지, 성능/비용·A/B·최종 소스 대조·전체 완료 감사는 남아 있다. [P0 ledger](p0-validation.md)의 초기 실패/수정/대기 기록은 당시 상태로 보존하고 최신 판정에는 위 실제 결과 파일을 사용한다.

## 판정 및 근거 규칙

| 항목 | 기록 규칙 |
|---|---|
| 관찰 | 도구 응답, 원본 실행 결과, 빌드·테스트 출력처럼 실제 확보한 자료와 그 시점·대상·조건을 링크한다. |
| 도출 | 관찰 자료에서 계산한 값은 원자료, 계산식, 분모와 제외 항목을 남긴다. |
| 추론 | 직접 확인하지 않은 내부 동작과 설계 가설에는 추론임을 명시한다. |
| 미확인 | 자료 부족, 도구 한계, 실행 환경 부재를 분리하고 다음 확인 절차를 남긴다. |
| 기능 구현 판정 | 구현 파일만으로 완료하지 않는다. 해당 기능의 실제 실행·테스트 결과와 제한을 연결한다. |
| REA 성공 판정 | 분석 질문, 대상 해시, 실제 REA 호출, 관련 함수/참조/의사코드, 구현에 사용한 근거가 연결될 때만 `REA 근거 기반`으로 분류한다. |
| 실패 | 호출·환경·분석 실패도 실제 출력, 원인 또는 원인 미확인, 복구 시도와 결과를 보존한다. |
| 미실행 | 제안된 테스트·명령과 실제 실행된 테스트·명령을 구분한다. |

각 분석 근거의 최소 필드는 `evidence_id`, 대상 버전·경로·SHA-256, 조사 질문, 도구 및 엔진, 호출 인자·결과/보존 위치, 시간, 함수 주소 또는 식별자, 디컴파일·참조 자료, 관찰/도출/추론의 구분, 신뢰도, 미확인 사항이다. REA 세션과 Evidence의 원래 식별자·출처·실패 사유를 유지한다. 함수 주소가 없는 자료는 없다고 명시한다.

## 목적·입력·실험 통제 (§1–2)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| OBJ-01 | 실행파일을 REA로 역공학하고 그 결과로 주요 기능·동작을 충실히 재구현한다. | 기능별 원본 관찰 → REA 분석 → 구현 → 비교 테스트 연결표. | 진행 중: REA 근거를 사용한 Windows P0 구현·실제 시험 있음; 전체 기능/원본 충실도·최종 평가 미완료 |
| OBJ-02 | Windows 11 탐색기와 유사한 UI/사용 경험, Windows·Linux·macOS 공통 코드베이스를 설계한다. | UI 비교·UX 확인, 계층/어댑터 설계, 플랫폼별 실제 검증과 미검증 범위. | Win11 다크 UI/분리 설계·Windows 화면 확보; Linux configure만, macOS·전체 UX 미검증 |
| Q-01 | 소스 없이 실행파일 내부 구조를 어디까지 분석할 수 있는지 평가한다. | 조사 질문 목록, 근거 확보율, 실패·신뢰도·범위 보고. | 실제 분석/한계 보고 확보; 질문별 성공률·최종 평가 미완료 |
| Q-02 | REA 결과를 이용한 AI의 기능 복원 정확도를 평가한다. | 동일 시나리오 원본/재구현 비교와 최종 소스 대조. | 부분 원본/B 실행 비교 가능; 전체 비교·사후 소스 정확도 미검증 |
| Q-03 | REA 미사용 AI 코딩 대비 이점을 평가한다. | 동일 조건 A/B 실험, 기능·일치율·시간·비용 비교, 교란 요인. | A 독립 시작 보고, B 진행; 같은 범위 결과/정량 비교 미완료 |
| Q-04 | Windows 전용 기능의 크로스플랫폼 재설계 가능성을 평가한다. | Shell/API 의존성, 공통 인터페이스, OS 지원표·검증 결과. | 공통 계약·Windows/POSIX 구현 확보; 비Windows 제품 검증 미완료 |
| Q-05 | 분석·구현 모델, 컴퓨팅 리소스, 비용을 평가한다. | 실제 모델·세션·리소스·토큰/비용 자료 및 측정 불가 항목. | 환경/도구 버전 기록; 모델 배포 ID·총 CPU/메모리·토큰/비용 미측정 |
| INPUT-01 | 대상은 Explorer++, 공식 프로젝트는 `https://github.com/derceg/explorerplusplus`, 공식 배포 실행파일과 직접 관찰을 입력으로 사용한다. 우선 Windows x64이다. | 공식 배포 출처·릴리스·파일 식별, x64 확인, 관찰 기록. | 공식 x64 EXE 확보·실제 원본 관찰 있음; manifest/behavior-summary 참조 |
| INPUT-02 | 실행파일의 정확한 버전, 다운로드 출처, SHA-256을 기록한다. | 대상 manifest와 실제 해시 명령 출력, 파일 크기·보관 위치. | target-manifest와 REA digest·바이트 parser 교차검증 확보 |
| CLEAN-01 | 초기 역공학 중 Explorer++ 원본 C++ 코드 열람·저장소 클론·코드 검색을 금지한다. | 접근 허용 목록, 작업/세션 기록, 소스 미접근 선언과 한계. | 준수 중: 소스 미다운로드/미열람, 읽기 gate 잠금; 최종 freeze 미완료 |
| CLEAN-02 | 기존 구현 복사·통합, 근거 없는 공개 역공학 결과 사용, 원본 소스의 AI 컨텍스트 제공을 금지한다. | 의존성·입력·자료 출처 목록, 기능별 근거 분류, 세션 통제. | 준수 중: 입력/REA/API 분류 유지; 최종 의존성·세션 감사 미완료 |
| CLEAN-03 | 허용 입력은 컴파일 배포물, 직접 실행 관찰, REA/Ghidra 출력, OS·프레임워크 공식 API 문서이다. | 입력 자료 목록·접근 로그·공식 문서 URL/사용 목적. | manifest·공통 관찰·REA bundle·공식 API 출처 기록; 최종 입력 감사 남음 |
| CLEAN-04 | 초기 구현 및 분석 보고서 확정 후에만 원본 소스로 정확도를 검증한다. | 확정 시각·커밋·해시, 소스 열람 허가 게이트 기록, 이후 대조 보고서. | 잠금 유지; 초기 산출물 freeze/verified manifest·사후 대조 미실행 |
| CLEAN-05 | 역공학과 사후 검증 작업 디렉터리 및 접근 권한을 분리한다. | 분리 경로·ACL/계정/세션 설정과 실제 접근 통제 확인. 디렉터리 이름만으로 충족하지 않는다. | 실제 source ACL read/list 거부 확인; owner/관리자 복원 가능 한계 명시 |

## 환경 및 REA 실동작 (§3)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| ENV-01 | Windows 10/11 x64, C++20, MSVC, Qt6 Widgets 또는 QML, CMake, Codex, REA Skill+MCP, Ghidra, Git 환경. | OS·아키텍처·각 도구 경로/정확 버전, C++20 Qt 최소 빌드 결과. | Windows 실제 C++20 Qt6.8.3 빌드/4 suite 통과; WSL Qt6.4.2 configure |
| ENV-02 | REA MCP가 에이전트에 정상 등록되어 있는지 확인한다. | 실제 도구 접근·응답 기록. | 실제 도구 호출·session 응답 보존; Phase1 통과 |
| ENV-03 | 에이전트가 REA 도구 목록을 확인할 수 있어야 한다. | 실제 도구 발견 호출/목록 기록, 필요한 기능 선택 근거. | phase1-session에서 207 도구 계약·발견과 실제 사용 기록 |
| ENV-04 | Ghidra 실행 경로와 JDK 구성이 유효해야 한다. | 실제 경로·버전·실행 및 분석 엔진 출력. | Ghidra12.1.4/JDK21.0.12.1 실제 분석 성공·session 보존 |
| ENV-05 | 작은 Windows PE를 실제 분석한다. | 테스트 PE 출처/해시, REA 분석 세션, 성공/실패 출력. | whoami.exe 실제 PE 분석 성공; Phase1 정본 참조 |
| ENV-06 | 함수 정보, 호출 관계, 디컴파일 결과를 실제 획득한다. | 함수·호출·의사코드 출력과 Evidence 연결. | 작은 PE 및 Explorer++ 실제 함수·호출·의사코드 확보 |
| ENV-07 | 분석 결과를 파일에 저장하고 다시 검토한다. | 보존 파일·해시, 재열람 기록과 내용 검토. | 원본 bundle 저장·JSON 재검토 및 대상 digest 검증 |
| ENV-08 | MCP 연결만으로 환경 정상 판정을 하지 않고 실제 호출 결과를 확인한다. | Phase 1 게이트의 PE 분석·재검토 증명. | 실제 PE/제품 빌드까지 수행; MCP 연결만으로 판정하지 않음 |
| ENV-09 | 환경·지원 기능 문제는 원인과 수행하지 못한 범위를 보고한다. | 실패 로그, 원인/미확인 구분, 복구 대안과 결과. | Qt 환경/명령·분석응답·초기 test 실패·UIA 한계와 복구 기록; 남은 범위 공개 |

## 역공학 범위 (§4)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| RE-01.a | 실행파일 형식·아키텍처, PE 헤더·섹션, Import/Export를 확인한다. | 실제 REA 구조 출력, 바이트/헤더 근거, 없는 export도 관찰 범위 명시. | 확보: PE32+ x64·7섹션·entry, imports22DLL/519·exports6; 보충 parser는 REA와 구분 |
| RE-01.b | 문자열·리소스, 함수 목록·진입점, 호출 관계·외부 API 의존성을 확인한다. | 검색·열거 범위와 실제 자료, 함수/주소/참조, 커버리지·제한. | REA 전체 함수9610/정의문자열4962·리소스418·icon groups2·정적 호출 일부 보존; 전역 callgraph 미확인 |
| RE-01.c | `docs/analysis/binary-overview.md`를 작성한다. | 위 구조·출처·해시·근거·미확인 사항을 포함한 보고서. | [binary-overview](../analysis/binary-overview.md) 작성·근거/제약 연결 |
| RE-02.a | 폴더 선택부터 파일 표시까지 경로 전달, 디렉터리 열거를 추적한다. | 관련 함수·호출·의사코드와 원본 시나리오 관찰. | PIDL→bind→EnumObjects/Next→metadata→목록 경로의 정적 근거; UI dispatch 전체 미해결 |
| RE-02.b | 속성·메타데이터 조회, 목록 갱신, 정렬, 접근 실패 처리를 추적한다. | 각 경로의 REA 자료, 실제 동작 및 분석 불가 사유. | 메타데이터·큐/삽입·비교 함수 확보; 정확 UI handler/원본 모든 오류 정책 미확인 |
| RE-02.c | 관련 함수·호출 관계를 다이어그램으로 정리한다. | 출처를 연결한 Mermaid/도식, 확인된 관계와 추론 구분. | [file-navigation](../analysis/file-navigation.md)에 실선/도출/미해결 관계 도식 |
| RE-03.a | 폴더 더블클릭, 상위 폴더, 뒤로/앞으로, 새 탭 이벤트 흐름을 추적한다. | UI 이벤트 → 핸들러 → 내부 작업 관계, 실제 원본 관찰과 REA 참조. | 원본 주요 이벤트 실제 관찰+RTTI/함수 단서; history/tab 내부 handler 복원 미완료 |
| RE-03.b | 파일 선택, 이름 변경, 검색 실행의 이벤트 흐름을 추적한다. | 각 이벤트 경로와 UI/내부 로직 연결, 불명확한 부분 표시. | 검색 thread/열거 경로 확보; 선택·rename 전체 handler와 실행 오류 미확인 |
| RE-04.a | 아이콘/썸네일, 연결 프로그램 실행, 디렉터리 변경 감지의 Shell 사용을 조사한다. | 실제 import/호출/참조 및 관찰. 예시 기능의 존재·적용 여부도 조사 결과로 남긴다. | Shell 아이콘·PIDL 실행·변경감지 호출 근거; file thumbnail/이벤트 연결 일부 미확인 |
| RE-04.b | 복사/이동, 드래그 앤 드롭, 속성 표시, 휴지통 처리의 Shell 사용을 조사한다. | Windows 제공 API와 자체 로직을 구분한 함수/의사코드·동작 근거. | copy/move wrapper·DnD vtable 해석 확보; properties/recycle/rename 전체 미확인 |
| RE-04.c | 모든 기능이 자체 구현이라고 가정하지 않고 OS 재사용과 고유 로직을 구분한다. | 기능별 책임·API 분류표, 확인되지 않은 책임은 미확인. | [shell-integration](../analysis/shell-integration.md)에 OS 위임·고유 정책·미확인 분리 |
| RE-05.a | 주요 결과에 대상/해시, 도구/엔진, 목적, 함수 주소/식별자를 보존한다. | 완결된 근거 메타데이터와 원문 링크. | canonical Evidence에 대상/hash/provider/profile/operation/주소 보존; 기능별 문서 연결 |
| RE-05.b | 디컴파일/참조, 추론 과정/결과, 신뢰도, 미확인 사항을 보존한다. | 각 분석 노트의 구조화된 근거 기록. | pseudocode·xrefs·한계·관찰/도출/추론 분리 보존; 최종 소스 대조 남음 |
| RE-05.c | 근거 부족한 내용을 사실로 확정하지 않는다. | 분석 노트·기능 기여 분류의 관찰/추론/미확인 감사. | 준수 중; UIA·디컴파일·unknown/부분 결과 한계 명시, 전체 역공학 성공 미선언 |

## 기능별 구현·기여·검증 (§5)

모든 기능은 구현 상태, 실제 테스트, 원본 비교, REA 근거를 각각 기록한다. 현재 B의 P0 구현과 Windows 시험·일부 desktop 근거가 있으며 P1/P2 잔여는 아래에 보존한다. REA 기여는 근거가 있는 하위 동작에만 붙이고 전체 기능 역공학 성공률은 아직 산출하지 않는다.

| 기능 | 우선순위·요구 동작 | 필요한 실행 검증/자료 | 구현·테스트 | 기여 분류 |
|---|---|---|---|---|
| FR-01 | P0: 주소 표시줄, 탐색 트리, 파일 목록, 탭, 상태 표시줄. | 실제 UI 캡처·키보드/마우스 조작·레이아웃 확인. | 구현·Windows Qt tests/desktop구성확인; sidebar/시각UX추가검증 | 일반 Qt API/관찰·AI설계; UI구조만으로REA복원아님 |
| FR-02 | P0: 로컬 드라이브와 폴더 목록 탐색. | 원본/재구현의 드라이브·빈/대량 폴더 시나리오. | Windows실제1200/empty/Unicode완전목록시험; drive/tree desktop추가검증 | REA근거기반: Shell/PIDL열거·메타데이터; POSIX일반API |
| FR-03 | P0: 직접 경로 입력, 폴더 더블클릭 이동. | 유효·잘못된 경로와 더블클릭 이동 결과. | addressEnter/Qt폴더doubleclick 통과; desktop주소이동확인 | REA경로parse/browse 부분활용+일반API/UI; 이벤트전체복원미확인 |
| FR-04 | P0: 뒤로, 앞으로, 상위 폴더 이동. | 경계·분기 탐색 기록과 부모 경로 이동. | domain/session/Qtback-forward-up통과; desktopB경계추가검증 | 일반API/관찰·AI설계; RTTI만으로history복원주장안함 |
| FR-05 | P0: 이름·크기·형식·수정일 표시. | 알려진 fixture 메타데이터와 실제 표시 대조. | 전체fixture메타데이터시험+실제Unicode6개표시 | REA근거기반: FindData/PIDL경로; 표시형식은일반QtAPI |
| FR-06 | P0: 이름·크기·날짜 정렬. | 각 열·방향·동률·폴더 처리의 원본 비교. | domain양방향/ties/unknown·adapternumeric·Qt열클릭통과; 전체원본일치미완료 | REA근거기반: natural/time/size비교; 폴더우선/최종tie는새정책 |
| FR-07 | P0: 현재 폴더 기준 이름 검색. | 정답 집합·검색 범위·문자/패턴 의미와 원본 비교. | Qt literal검색tests·desktop한글정확2개관찰 | 일반API/요구기반: inline현재폴더; 원본SearchDialog재귀기본과명시차이 |
| FR-08 | P0: 탭 생성·전환·닫기. | 탭별 경로/기록 독립성과 닫기·전환. | 독립상태·Qt생성/닫기/비활성탭회귀통과; desktopB전체남음 | 일반API/관찰·AI설계; TabContainer이름만으로REA성공아님 |
| FR-09 | P0: OS 기본 연결 프로그램으로 파일 열기. | 안전 fixture의 실제 연결 프로그램 실행·실패 안내. | B-UI-002 실제한글파일doubleclick→Notepad창확인; 실패/교체추가검증 | REA근거기반: PIDL/Shell위임; 실제UIdispatch원본주소연결미확인 |
| FR-10 | P0: 접근 거부·잘못된 경로·삭제된 파일 대응. | 실제 오류·경합 상황에서 생존과 복구 가능한 안내. | 실제missing/invalid/ACL+상태보존tests통과; 실제외부삭제/desktop복구남음 | 일반API/AI설계; 원본오류전체정책복원아님 |
| FR-11 | P1: 단일·복수 파일 복사. | 격리 fixture의 내용/해시·진행률·실패 시 원본 보존. | 미구현/미검증 | 미구현 |
| FR-12 | P1: 파일 이동·충돌 처리. | 안전 fixture의 정상/충돌/실패 결과·확인 절차. | 미구현/미검증 | 미구현 |
| FR-13 | P1: 휴지통 이동 또는 안전한 삭제. | OS 지원·선택 정책·확인 UI, fixture 처리 결과. | 미구현/미검증 | 미구현 |
| FR-14 | P1: 단일 파일·폴더 이름 변경. | 성공·중복·권한/잠금 실패와 원본 보존. | 미구현/미검증 | 미구현 |
| FR-15 | P1: 새 디렉터리 생성. | 생성 성공·이름 충돌·권한 실패. | 미구현/미검증 | 미구현 |
| FR-16 | P1: Ctrl/Shift 다중 선택·일괄 작업. | 범위/추가 선택과 복수 작업 결과. | 선택 상태/Qt selection 모델 일부 구현·B-UI-002 단일선택33B관찰; Ctrl/Shift/일괄작업 P1검증남음 | 일반Qt API/AI설계; 내부선택handler REA복원미확인 |
| FR-17 | P1: 내부 및 OS 앱 간 드래그 앤 드롭 이동. | 내부/앱 경계 실제 드롭·의도·충돌·실패 결과. | 미구현/미검증 | 미구현 |
| FR-18 | P1: 아이콘·목록·자세히 보기. | 각 모드 전환·선택/탐색 일관성·화면 캡처. | 자세히 보기 구현·실제4열/desktop확인; 아이콘·목록 모드 P1미완료 | 일반Qt API/새UI설계 |
| FR-19 | P1: 일반 탐색·파일 관리 단축키. | 문서화한 키 매핑과 실제 키 조작·포커스 검사. | P0 주소/탭/탐색 키 Qt시험통과; 파일관리단축키 P1미완료 | 일반Qt API/관찰·AI설계 |
| FR-20 | P1: 파일 개수·선택 항목·작업 진행률. | 목록/선택 수와 작업 진행·종료 상태 일치. | 항목/선택수·선택크기 B-UI-002 관찰·read progress시험; 파일작업진행 P1미완료 | 일반API/새상태모델; REA전체복원주장없음 |
| P2-01 | 선택: 썸네일·파일 미리보기. | 지원 형식·실제 출력·실패/제한. | 미구현/미검증 | 미구현 |
| P2-02 | 선택: OS 컨텍스트 메뉴 연동. | 플랫폼별 실제 메뉴·호출 결과. | 미구현/미검증 | 미구현 |
| P2-03 | 선택: 즐겨찾기·빠른 접근. | 추가·이동·삭제·상태 검증. | 미구현/미검증 | 미구현 |
| P2-04 | 선택: 상세 검색 필터. | 필터 의미와 정답 집합 비교. | 미구현/미검증 | 미구현 |
| P2-05 | 선택: 파일 변경 실시간 반영. | 외부 생성·수정·삭제 알림 결과. | 미구현/미검증 | 미구현 |
| P2-06 | 선택: 복사·이동 작업 취소. | 취소 시점별 안전성·진행 상태·잔여 파일. | 미구현/미검증 | 미구현 |
| P2-07 | 선택: Undo/Redo. | 작업별 되돌리기·재실행·실패 안전성. | 미구현/미검증 | 미구현 |
| P2-08 | 선택: 다크 모드. | 실제 테마·가독성·전환. | 다크기본테마 구현·실제Win11참고/B화면보존; theme전환/가독성전체검증남음 | 일반Qt API/자체디자인; REA기능복원아님 |
| P2-09 | 선택: 사용자 설정 저장. | 재실행 복원·손상/부재 처리. | 미구현/미검증 | 미구현 |
| P2-10 | 선택: Windows Shell 특수 위치. | 지원 위치·가상 경로 의미·미지원 오류. | 미구현/미검증 | 미구현 |

P2는 시간·분석 가능 범위에 따라 결정하며 미구현을 그대로 공개한다. P1 전체 범위는 Phase 5에서 다루며 P0 통과만으로 전체 프로젝트를 종료하지 않는다.

## UI·UX 및 기술 결정 (§6)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| UX-01 | Windows 11의 현대적 디자인을 참고하고 Explorer++ 기능 동작 분석과 디자인을 구분한다. | 디자인 기준/출처와 별도 기능 근거표, UI 비교 캡처. | 실제 Win11 참고+ADR·B 다크 화면 확보; 시각/UX 비교 전체 감사 남음 |
| UX-02 | 상단 탭 바, 탐색 버튼·주소 표시줄, 좌측 트리, 중앙 목록, 검색 입력창, 하단 상태 표시줄. | 실제 창 캡처·각 구성요소 동작. | 구현 및 실제 B-UI-001/002 화면·접근성 값 확보; sidebar 실제 탐색 추가 검증 남음 |
| UX-03 | 마우스와 키보드 모두 지원한다. | 포커스/탭 순서·단축키·마우스 시나리오. | OFFSCREEN 실제 Qt keyboard/mouse tests 통과; native desktop 전체 경로 남음 |
| UX-04 | 창 크기 변경에 레이아웃이 정상 반응한다. | 여러 창 크기/최소 크기에서 UI 검사. | OFFSCREEN resize 시험 통과; native 다중 크기/최소 크기 시각 검사 남음 |
| UX-05 | 탐색 중 UI 멈춤을 방지한다. | 대량 폴더 중 조작 가능성·UI 응답 시간 측정. | generation/worker 비동기 시험 통과; 대량 native UI latency 미측정 |
| UX-06 | 긴 파일 이름·Unicode를 지원한다. | 실제 긴 경로/이름과 Unicode fixture 결과. | 전체 Unicode/긴 이름 목록·metadata 시험 및 desktop 표시/한글 파일 실행; OS 장경로 미검증 |
| UX-07 | 대량 목록 처리 중 진행 상태를 제공한다. | 작업 중/완료/실패 시 상태와 캡처. | 실제 adapter progress 단조 증가 시험; 대량 처리 중 desktop 표시/응답 측정 남음 |
| UX-08 | 오류 시 복구 가능한 안내를 제공한다. | 원인·재시도/유효 경로 복구 동작. | 실패 상태 보존/오류 banner Qt시험 통과; native 권한/외부삭제 복구 안내 남음 |
| UX-09 | Windows 스타일 기본 아이콘과 포커스 동작을 제공한다. | 합법적 아이콘 출처·포커스 시각/키 조작 검증. | 자체 아이콘·선택/focus/UI 구현; native focus/accessibility 전체 감사 남음 |
| UX-10 | 저작권 보호 UI 자산을 무단 추출하지 않고 허용된 OS 리소스나 별도 제작 자산을 사용한다. | 자산 출처·허용 범위·라이선스 목록. | 스크린샷/원본 resource 자산 추출 없이 자체 UI; 최종 자산/라이선스 목록 남음 |
| ADR-01 | Qt6 Widgets와 QML을 비교한 뒤 하나를 선택하고 ADR로 이유를 남긴다. | 재현성·난이도·성능/메모리·커스터마이즈·크로스플랫폼·유지보수 6기준 비교. 측정 없는 성능은 추론 표시. | [ADR-001](../architecture/adr-001-gui.md) 6기준 비교 후 Widgets 선택; 예상성능과 실측 구분 |

## 아키텍처 (§7)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| ARCH-01 | UI·공통 비즈니스 로직·OS 종속 기능을 분리한다. | 의존성 도식, 실제 코드·빌드 대상 경계. | 실제 UI/core/platform target·public 계약 분리; 지속 경계 감사 필요 |
| ARCH-02 | 권장 Presentation(Qt UI), Application(오케스트레이션), Domain(모델), Platform Interface, Platform Adapter 책임을 정의한다. | 모듈 책임·인터페이스 ADR, 추천과 필수 분리 원칙을 구별. | architecture/interfaces/ADR와 실제 모듈 책임 확정; Phase3 통과 |
| CORE-01 | 공통 C++20/Qt Core로 경로·탐색 기록·탭 상태·검색 결과 관리를 구현한다. | 공통 모듈 구현과 모의 어댑터 테스트. | 공통 domain/session 구현, OS없는 mock session/domain Windows tests 통과 |
| CORE-02 | 공통 정렬·필터·파일 작업 요청·진행 상태·오류 모델·사용자 메시지를 구현한다. | 인터페이스·명시적 오류와 단위/통합 테스트. | 정렬/필터/진행/오류 구현·시험; P1 파일작업 요청/진행 구현 남음 |
| ADAPT-01 | Windows Win32/Shell/COM 호출을 OS 어댑터에 격리한다. | 호출 위치·링크 의존성 감사와 실제 Windows 실행. | Windows Shell/Win32/COM을 Windows adapter에 구현·실제 platform tests 통과 |
| ADAPT-02 | Linux 표준 파일시스템, 필요 시 GIO 통합을 분리한다. | Linux 어댑터 설계·지원표·가능 시 실제 빌드/테스트. | POSIX std::filesystem/lstat/locale/open 구현; Linux configure만, build/run 미검증 |
| ADAPT-03 | macOS 표준 파일시스템, 필요 시 Foundation/AppKit 통합을 분리한다. | macOS 호환성 검토·지원표·환경 확보 시 실행. | POSIX 공유 경계·macOS 조건부 helper 설계; 실제 macOS 검토/build/run 미검증 |
| ADAPT-04 | 공통 비즈니스 로직이 Win32 함수에 직접 의존하지 않는다. | 헤더/호출/링크 의존성 검사와 비Windows 컴파일. | public core 계약에 Win32 타입 없음·CMake OS선택; 비Windows compile 검증 남음 |
| ARCH-V01 | OS 호출이 지정 Adapter 밖에 무분별하게 존재하지 않는다. | 경계 감사 결과와 불가피한 예외 사유. | 계층/target 경계 구현; 전체 source/static-analysis 감사 남음 |
| ARCH-V02 | 플랫폼 기능 지원 여부를 질의할 수 있다. | capability 계약과 각 어댑터 테스트. | capabilities 구현; open 가능/P1 false 명시, 실제 OS별 지원 시험 추가 필요 |
| ARCH-V03 | 미지원 기능은 명시적 오류를 반환한다. | Unsupported 등 결과 계약과 소비 UI 검증. | tagged Unsupported/오류 계약 구현; 실제 미지원 UI 전 경로 검증 남음 |
| ARCH-V04 | 공통 코어를 실제 OS 없이 모의 어댑터로 테스트한다. | OS 없는 공통 테스트 실행 결과. | mock session semantics/lifecycle 시험10pass·domain8pass |
| ARCH-V05 | Windows 전용 기능이 다른 OS 빌드를 방해하지 않는다. | 조건부 빌드 경계·Linux 실제 빌드·macOS 검토/가능 시 빌드. | 조건부 CMake로 Linux configure 성공; Linux/macOS 실제 build 검증 남음 |

## 비기능 요구사항 (§8)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| NFR-01.a | 디렉터리 로딩은 비동기 우선이며 대량 조회가 메인 UI를 장시간 차단하지 않는다. | 작업/스레드 구조와 실제 대량 조회의 응답성 측정. | 비동기 cancel/generation/late-result 시험 통과; native 대량 UI 지연 미측정 |
| NFR-01.b | 복사·이동 진행 상태를 표시한다. | 실제 파일 작업의 진행·완료·실패 UI. | P1 파일작업 미구현/미검증 |
| NFR-01.c | 메모리·CPU를 측정하고 동일 조건의 Explorer++와 비교한다. | 하드웨어/OS/데이터셋/캐시·cold/warm 조건, elapsed·CPU·peak process-family memory 원자료. | 원본/B 동일조건 elapsed·CPU·peak process-family memory 미측정 |
| NFR-01.d | 사전 테스트 환경을 정의한 뒤 성능 목표값을 확정한다. | 측정 전 고정한 환경·기준값/근거·변경 이력. 사후 결과에 맞춘 목표 변경 금지. | 환경/fixture/protocol 정의; 사전 성능 목표값 미확정 |
| NFR-02.a | 접근 불가 디렉터리에서도 종료하지 않고 파일 작업 실패 원인을 표시한다. | 권한 fixture·실패 작업·프로세스 생존/복구 검사. | Windows 실제 ACL PermissionDenied 시험·mock/Qt 실패복구 통과; P1 오류/desktop 추가 검증 |
| NFR-02.b | 외부 변경·삭제를 처리한다. | 실행 중 외부 변경/삭제 경합 테스트. | identity 재검사/refresh·mock NotFound/Changed 시험; 실제 외부삭제/교체 시나리오 남음 |
| NFR-02.c | 경로 길이·Unicode·심볼릭 링크·Windows reparse point를 고려한다. | 의미/정책 문서와 실제 플랫폼 fixture 테스트·권한 제한. | Unicode/긴 이름 실측값·reparse/link 정책 구현; 장경로·실제 Windows 링크 권한 부재 skip |
| NFR-02.d | 링크 순환으로 무한 탐색하지 않는다. | 순환 fixture·중단/방문 정책·완료/자원 제한 검사. | 자동 재귀 안 하는 설계; Windows 실제 링크시험 privilege skip, 순환작업 미검증 |
| NFR-03.a | 초기 테스트는 별도 테스트 디렉터리에서 수행한다. | fixture 경로·소유권/정리 범위·실행 기록. | 고정 fixture/임시 소유 경로에서 실제 읽기/링크 시도; fixture integrity check 통과 |
| NFR-03.b | 시스템 파일·중요 사용자 데이터를 자동 삭제/변경하지 않는다. | fixture 한정 작업 설계·쓰기/삭제 경로 기록. | 소유 test-data 범위 준수; source gate 읽기 거부 유지, P1 파괴작업 미구현 |
| NFR-03.c | 덮어쓰기·영구 삭제 등 파괴 작업은 확인 절차를 둔다. | 실제 취소/승인 UI와 파일 보존/작업 결과. | 설계에 exact 요청 확인 규정; P1 overwrite/permanent-delete UI 미구현 |
| NFR-03.d | 복사 실패 시 원본을 보존한다. | 실패 주입/실제 실패 fixture의 원본 해시·존재 검사. | P1 복사 실패/원본 보존 실제 시험 미실행 |
| NFR-03.e | 경로·권한 검사 실패를 무시하지 않는다. | 오류 전달·사용자 안내·작업 미수행 근거. | 실제 missing/invalid/ACL errors와 상태보존 시험 통과; P1 쓰기 권한 검증 남음 |
| NFR-04.a | CMake 재현 빌드, 공개 인터페이스 문서화, 모듈별 책임 분리. | 깨끗한 빌드 명령/버전/결과, API 문서와 경계. | CMake MSVC 실제 제품 빌드·공개 계약/모듈 문서 확보; 배포 재현성 남음 |
| NFR-04.b | 자동 테스트, 코드 포맷팅·정적 분석 도구를 사용한다. | 설정·실제 실행 결과·미해결 진단. | 4개 자동suite 실행; 초기 실패/수정 기록 보존; formatter/static-analysis 실제 실행 남음 |
| NFR-04.c | 외부 라이브러리·라이선스 목록을 관리한다. | 이름·버전·출처·라이선스·배포 의무 목록. | Qt SDK 일부 license inventory 있음; 최종 외부 라이브러리/자산/재배포 notices 미완료 |

## A/B 실험 및 측정 (§9)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| EXP-01 | 최소 A(REA 미사용)와 B(REA 사용)를 비교한다. | 사전 고정 프로토콜·독립 산출물·동일 테스트 결과. | A fresh-context 시작 보고·B 실제 구현 진행; 독립 결과/비교 미완료 |
| EXP-02 | 두 실험의 모델·실행파일/관찰 입력·개발 환경·기능 요구사항을 동일하게 한다. | 모델 실제 식별자, 목표/fixture/관찰 자료 해시, 도구 버전·예산·세션 자료. | 동일 요구/fixture/toolchain 규칙; 실제 모델 배포ID·설정/비용 동등성 미확인 |
| EXP-03 | 양쪽 원본 소스 열람 금지, A에는 REA 출력 미제공, B는 실제 분석 결과 활용. | 컨텍스트/입력 허용 목록·도구 접근 통제·실제 활용 근거. | B 실제 REA 활용, A 미제공 지시/분리 trace; 최종 입력/접근 감사 남음 |
| EXP-04 | 가급적 별도 디렉터리·에이전트 세션에서 수행한다. | A/B 분리 경로·세션 ID·접근 범위. 오염이 있으면 공개하고 공정한 독립 비교로 주장하지 않는다. | 별도 context/디렉터리 진행; 동일 사용자 A/B 강제 ACL 격리 미입증 |
| EXP-05 | 기능 구현률을 정상 동작 기능/요구 기능으로 측정한다. | 고정 분모(P0/P1/P2별), 기능별 실제 테스트·계산식. | 미측정 |
| EXP-06 | 동작 일치율을 원본과 동일 결과인 테스트 비율로 측정한다. | 동일 시나리오·원본/재구현 출력·비교 규칙·분모. | 미측정 |
| EXP-07 | 역공학 정확도를 사후 소스와 분석 결과 비교로 측정한다. | 소스 열람 전 고정한 주장 집합·후속 대조·정확/오류/미확인. | 소스 게이트 잠금 |
| EXP-08 | 각 단계의 실제 개발 시간을 측정한다. | 시작/종료·중단·활성 작업과 대기 구분, 원자료. | 미측정 |
| EXP-09 | AI 입력/출력 토큰과 API 비용을 보고한다. | 실제 사용 내역·모델/가격 기준 시점·계산식·통화. 접근 불가 값은 미측정. | 미측정 |
| EXP-10 | 조사 질문 중 근거 확보 비율로 분석 성공률을 측정한다. | 사전 질문 목록·질문별 Evidence·성공 기준·분모. | 미측정 |
| EXP-11 | 예외 테스트 통과율로 안정성을 측정한다. | 예외 시나리오 목록·실제 실행·분모·제외 사유. | 미측정 |
| EXP-12 | OS별 빌드·기능 테스트로 이식성을 평가한다. | Windows/Linux/macOS 각각의 실제 결과·조건·미검증. | Windows 실제 결과·Linux configure만·macOS 미검증; 이식성 최종 평가 남음 |
| EXP-13 | 기능 기여를 REA 근거 기반/일반 API 기반/AI 추론 기반/미구현으로 분류한다. | 기능별 실제 근거·구현 연결. 혼합 기능은 근거가 있는 하위 동작을 명시한다. | FR별 근거 기반/일반 API/AI추론의 부분 범위 아래 분류; P1 미구현 |
| EXP-14 | Qt 파일시스템 모델 사용만으로 REA 복원이라고 주장하지 않는다. | 일반 API 사용과 실제 REA로 얻은 동작 차이를 분리한 감사. | Windows 자체 Shell adapter 사용·POSIX 일반 API와 구분; Qt모델만의 REA복원 주장 없음 |

측정의 한계: 모델명·버전, 토큰, 호출별 비용이 제공되지 않으면 추측해서 채우지 않는다. 시간은 실측 기록 시작 전 작업을 정밀 측정한 것처럼 소급하지 않는다. 비용 추정은 추정으로 표시하고 원시 사용량과 분리한다. 동일 모델을 확인할 수 없거나 독립 세션/컨텍스트 통제가 불가능하면 요구된 비교를 수행했다고 선언하지 않는다. CPU·메모리·warm/cold 중 미측정한 항목은 별도로 공개한다. 파일 크기/응답 바이트를 토큰·시간·비용의 대체 측정값으로 사용하지 않는다.

## 필수 테스트 14개 (§10.1)

읽기 테스트와 수정 테스트는 별도 suite/fixture로 분리한다. 아래 행마다 실제 원본과 재구현의 결과, 플랫폼, 실행 시각·빌드/해시, fixture, 명령/절차, 기대·실제 결과, 로그/화면, 제한을 연결한다. Windows에서 아래 실제 결과를 확보했다. P1/외부삭제/링크/최종종료 시나리오는 미완료이며 suite 통과가 14개 전부 통과라는 뜻은 아니다.

| ID | 필수 시나리오 | 검증 범위·필요 근거 | 분류 | 상태 |
|---|---|---|---|---|
| T-01 | 빈 폴더 열기 | 빈 목록·경로·0개 상태·안내, 원본 비교. | 읽기 | Windows adapter+UI exact0 tests통과; 원본 empty안정관찰 확보 |
| T-02 | 1,000개 이상 파일 폴더 | 실제 파일 수·완전성·정렬/반응성·진행률·CPU/메모리/시간. | 읽기 | Windows 1200 전체name/size/time/type/identity+UI model tests통과; 원본상태1200관찰, UIlatency/CPU/memory 미측정 |
| T-03 | 한글·특수문자 파일 이름 | 표시·경로 이동·검색·선택·긴 이름/Unicode 보조 케이스. | 읽기 | Windows 전Unicode manifest/model tests·desktop6개표시·한글파일Notepad실행; 장경로는별도미검증 |
| T-04 | 존재하지 않는 경로 | 종료 없이 명시적 오류와 복구, 경로 입력 보존/상태 정책. | 읽기/오류 | Windows NotFound/invalid+UI기존행/history보존 tests통과; native 복구 동작 추가 필요 |
| T-05 | 권한 없는 폴더 | 실제 권한 거부가 발생했는지 확인하고 생존·오류·복구 검사. 모의 결과와 구별. | 읽기/권한 | Windows source ACL 실제 adapter PermissionDenied통과; mock상태보존, Linux/desktop추가검증 남음 |
| T-06 | 뒤로/앞으로 기록 | 다단계 이동·분기 후 앞으로 기록·경계·상위 이동. | 읽기 | domain/session+실제Qt버튼 tests통과; 원본Back/Forward/Up관찰, native분기경계비교 남음 |
| T-07 | 여러 탭 독립 경로 | 생성·전환·닫기 후 각 탭 경로·기록·선택 상태 정책. | 읽기 | mock상태독립+Qt생성/닫기/비활성탭회귀 통과; 원본전환/닫기관찰, nativeBstress남음 |
| T-08 | 이름·크기·수정일 정렬 | 각 기준/방향과 동률·폴더 처리, 알려진 정답/원본 비교. | 읽기 | domain방향/tie/unknown+adapternumeric+UI열클릭 통과; 원본6파일 일부순서만, 전체일치비교 남음 |
| T-09 | 검색 정확성 | 현재 폴더 범위·일치 의미·Unicode·없음·정답 집합 비교. | 읽기 | Qt exact2/0/full literal결과 tests·desktop한글2개실제관찰; 원본SearchDialogscope차이 명시 |
| T-10 | 복사·이동 중 중복 충돌 | 단일/복수·확인/취소·덮어쓰기 정책·실패 원본 보존·내용 해시. | 수정/안전 | P1미구현/미실행; 충돌·확인취소·내용hash·실패원본보존 검증필요 |
| T-11 | 실행 중 외부 삭제 | 테스트 항목을 외부에서 삭제하고 목록/선택/열기 오류와 생존 확인. | 외부 fixture 수정 | mock NotFound/identity변경만; 실제외부삭제/교체fixture 시험 미실행 |
| T-12 | 파일·폴더 이름 변경 실패 | 충돌/권한/잠금 등의 실제 실패·원인 안내·원본 보존·상태 일관성. | 수정/오류 | P1rename미구현/미실행; 실제충돌/권한/잠금 실패검증필요 |
| T-13 | 링크·순환 경로 | symlink/reparse point 의미·루프 종료·깨진 링크·권한·자원 제한. 생성 권한 부재는 미검증. | 읽기; fixture 생성 분리 | Windows 실제 symlink생성 privilege없어 SKIP; 링크/순환 성공으로세지않음 |
| T-14 | 앱 종료·재실행 안정성 | 정상 종료·작업/로딩 중 종료 후 재실행·손상/유실/고아 작업 검사. 설정 복원은 구현 시 별도 검증. | 수명주기 | mock 늦은worker/open·close/shutdown+Qt종료 tests통과; 실제프로세스재실행/패키지/최종정리중취소 남음 |

## 역공학·플랫폼 검증 (§10.2–10.3)

| ID | 요구사항 | 완료를 입증할 근거 | 현재 상태 |
|---|---|---|---|
| VERIFY-01 | 분석한 실제 기능마다 REA 함수/호출, 원본 관찰, 재구현 동작, 최종 소스 구조를 비교한다. | 기능별 4방향 추적표와 서로 다른 신뢰 범위. | 정적근거/원본/B 부분 비교 확보; 전체 4방향 대조·최종 소스 접근 미실행 |
| VERIFY-02 | 디컴파일 의사코드를 원본과 동일하게 취급하지 않는다. | 사후 대조에서 차이·최적화·추정 타입·분석 한계 표시. | 계속 준수; source 최종대조 미실행 |
| WIN-01 | Windows 실제 빌드·실행·기능 테스트·배포 패키지 실행. | MSVC/CMake/Qt 버전·빌드 출력, 실제 앱·패키지 로그/스크린샷, 의존 DLL 확인. | 제품 빌드·4suite·실제 desktop 부분 실행; 배포 패키지 실행 미완료 |
| LINUX-01 | 환경 확보 시 실제 빌드, 공통/어댑터 테스트, 제한 기록. | 환경 탐색·빌드 시도·실제 결과·불가 원인/미검증 구분. | CMake configure 완료(6.4.2); 이 시점 B build/test/run 미검증 |
| MAC-01 | 공통 코드 호환성 검토, 환경 확보 시 실행 테스트, 미확인 항목 표시. | 소스/빌드 의존성 검토 및 환경별 실제 실행 여부. | 설계/조건부 코드만; macOS 호환성 전체 검토·build/run 미검증 |

## 산출물 (§11)

구조는 권장 구조이며, 각 책임·실질 산출물은 누락하지 않는다. 아래 경로가 존재한다는 사실만으로 내용 또는 검증이 완료되지는 않는다.

| ID | 산출물/권장 경로 | 확인할 내용 | 현재 상태 |
|---|---|---|---|
| OUT-01 | `src/ui/` | Qt GUI. | 작성·Windows빌드/Qt UI tests·desktop 부분 실행 |
| OUT-02 | `src/core/` | 공통 비즈니스 로직. | domain/session 작성·mock/domain tests 통과 |
| OUT-03 | `src/platform/` | OS별 구현·capability·오류. | Windows/POSIX 작성·Windows실제tests 통과; POSIX실행 미검증 |
| OUT-04 | `include/` | 공통 인터페이스·문서. | types/platform/session/UI 공개헤더·interfaces 문서 작성 |
| OUT-05 | `tests/` | 단위·통합·읽기/수정 분리·14시나리오 근거. | 4suite 작성·Windows통과, link1skip; P1/14시나리오 전체 미완료 |
| OUT-06 | `docs/analysis/` | 실제 REA 증거·분석·미확인/실패. `binary-overview.md` 포함. | 실제 REA bundle·전체inventory·3개 gate·구조/탐색/기능/Shell 보고서 보존 |
| OUT-07 | `docs/architecture/` | 계층·인터페이스·Qt 선정 ADR·플랫폼 결정. | ADR/architecture/interfaces/phase3-validation 작성·gate 통과 |
| OUT-08 | `docs/evaluation/` | A/B·모델/리소스·실험/최종 비교 보고. | 환경/프로토콜/P0 ledger/부분결과 있음; A/B/최종 보고 미완료 |
| OUT-09 | `docs/requirements.md` | 전체 요구사항 원본. | 존재·전체 열람; 충족 결과는 별도 |
| OUT-10 | `CMakeLists.txt` | 재현 가능한 빌드. | 작성·Windows실제 configure/build/test; Linuxconfigure |
| OUT-11 | `README.md` | 빌드·실행·사용·지원/제한·라이선스. | 미작성; 전체 사용·지원/배포/라이선스 문서 필요 |
| OUT-12 | Windows 실행 가능 애플리케이션 | 실제 빌드 및 패키지 실행. | build-windows/rea_explorer.exe 실제 빌드·실행; standalone패키지 미완료 |
| OUT-13 | 기능별 구현 체크리스트 | P0/P1/P2 구현·테스트·기여·남은 문제. | 이 전체 matrix 갱신; 각 기능 시험/근거와 남은 범위 구분 |
| OUT-14 | 원본/재구현 UI 비교 스크린샷 | 같은 시나리오·크기·대상/빌드 식별. | 원본/Win11참고/B 동일 Unicode fixture 캡처 보존; 상세 비교 설명 추가 필요 |
| OUT-15 | 주요 기능 시연 자료 | 재현 절차·실제 영상/연속 캡처·제한. | B 주소/열기/검색 실제 연속 관찰·캡처 있음; 주요 전체 기능 시연 미완료 |
| OUT-16 | 실제 REA 호출 기록 | 인자·엔진·대상/해시·Evidence·출력·실패. | 작은 PE·Explorer++ 실제 bundle/인자/provider/hash/limitations 보존 |
| OUT-17 | 기술적 한계·실패 사례 | 환경/분석/구현/검증의 실패와 복구·미완료. | 분석/Qt/시험 초기실패·UIA·symlink권한·미측정 한계 분산기록; 최종통합 미완료 |
| OUT-18 | 사용 모델·리소스 비교 보고 | A/B 모델·시간·CPU/메모리·토큰/비용·한계. | 미작성; 모델/리소스/시간/비용 비교 미측정 |

## 순차 단계 게이트 (§12)

Phase 5·6에는 명세에 별도의 `완료 조건` 문장이 없으므로 아래는 그 단계의 명시 작업과 최종 기준을 증명하기 위한 운영 게이트이다. 요구사항을 새로 축소하거나 P2를 필수로 바꾸지 않는다.

| Phase | 수행 범위 | 통과 근거·다음 입력 | 현재 상태 |
|---|---|---|---|
| 1 환경 | REA Skill/MCP·Ghidra, 실제 PE 분석, 대상 확보·해시. | 실제 함수·의사코드 분석 성공 + 재검토 가능한 결과. 전체 환경 점검·대상 manifest를 Phase 2로 전달. | 통과: 실제PE함수/의사코드·대상hash·WindowsSDKsmoke/제품build; 초기권한문제는ACLgate로해결 |
| 2 조사 | 원본 실행·시나리오, 핵심 함수·흐름, 분석/미확인 정리. | P0 주요 동작별 분석 자료 또는 실제 분석 불가 사유. 근거·관찰·불명확한 의미를 Phase 3으로 전달. | 최소자료gate 통과: P0자료/실제원본관찰·분석불가이유; 내부dispatch전체복원 성공아님 |
| 3 설계 | Qt 선택·공통 코어·파일시스템 추상화·OS 어댑터·테스트 구조. | 모듈 구조·정확한 인터페이스 확정. A/B 통제·fixture/성능 기준과 함께 Phase 4 입력. | 통과: Widgets/모듈/정확공개계약/테스트구조 확정 |
| 4 구현 | UI·탐색·탭/기록·정렬/검색·파일 관리. | P0 전 기능 구현 및 실제 테스트 통과. 초기 분석/구현 확정 범위·남은 P1/P2를 전달. | 진행중: Windows제품build/4suitepass·desktop부분성공; P0전체native검증/원본비교 잔여 |
| 5 확장/이식 | P1 추가·UI 개선·Windows 격리 확인·Linux 빌드 시도/결과. | P1 구현/시험 상태, UI/경계 감사, Linux 시도와 제한, macOS 검토. 미구현 공개 및 초기 보고서/구현 freeze. | 미완료: P1미구현, Linuxconfigure만, 패키지/OS감사·freeze 남음 |
| 6 평가 | 기능 비교·최종 소스 대조·A/B·모델/시간/비용·최종 보고. | clean-room 해제 전 freeze 확인, 실제 비교·평가·최종 기준 감사. | 미시작: 소스gate잠금, A/B결과·정량측정·4방향대조·최종감사 남음 |

## 최종 완료 체크 (§13)

- [x] FINAL-01 실제 REA MCP 및 Ghidra 분석 기록이 존재한다.
- [ ] FINAL-02 초기 역공학·구현에서 원본 소스 미사용을 검증 가능하게 유지했다.
- [x] FINAL-03 C++20/Qt6 Windows 앱이 정상 빌드된다.
- [ ] FINAL-04 P0 전체 기능 테스트가 통과한다.
- [ ] FINAL-05 주요 기능의 원본/재구현 결과 비교가 존재한다.
- [x] FINAL-06 UI·공통 코어·OS 어댑터가 분리되었다.
- [ ] FINAL-07 공통 코드 크로스플랫폼 이식성을 검토했다.
- [x] FINAL-08 Linux 실행 검증 여부·결과를 명확히 기록했다.
- [x] FINAL-09 REA 분석 근거와 AI 추론을 구분했다.
- [ ] FINAL-10 AI 모델·리소스·개발 시간·비용을 보고했다.
- [x] FINAL-11 남은 문제·미구현 기능을 명시했다.

체크박스는 항목별 현재 직접 근거만 반영한다. FINAL-01은 실제 REA bundle, FINAL-03은 Windows 제품 build, FINAL-06은 실제 모듈/target 분리, FINAL-08은 Linux configure와 실행 미검증의 명시적 기록, FINAL-09는 위 기능별 근거 구분, FINAL-11은 남은 범위 공개를 뜻한다. 나머지 소스미사용 최종freeze/전체P0/주요전체비교/이식성검토/모델·비용보고는 미완료이다. 체크된 일부 항목으로 §1–12의 범위를 생략하거나 전체 목표 완료를 선언하지 않는다.

## 에이전트 운영 지침 (§14)

| ID | 지침 | 준수/종료 증거 | 현재 상태 |
|---|---|---|---|
| AGENT-01 | 요구사항 기반 계획 수립. | `docs/plan.md`와 이 전체 매트릭스. | 전체 요구/plan/matrix/protocol 작성·유지 |
| AGENT-02 | 현재 환경 검사 후 가능한 단계부터 진행. | 실제 환경 inventory와 Phase 1 로그. | 환경실제조사·SDKsmoke·제품build 후 순차진행 |
| AGENT-03 | 실제 역공학 호출과 결과 기록. | 호출 원문·Evidence·보존 및 재검토. | 작은PE/대상 REA실제출력·재검토/한계 보존 |
| AGENT-04 | 기능별 구현·테스트 결과 지속 갱신. | 각 작업 후 상태·근거 갱신 이력. | 이 matrix·P0결과/실행로그 갱신; 최종감사 남음 |
| AGENT-05 | 임의 범위 축소·요구 삭제 금지. | 이 추적표와 원본 명세의 최종 대조. | 상시 준수 |
| AGENT-06 | 외부 소프트웨어 설치·시스템 변경·위험 파일 작업은 사전 승인. | 필요 조치·범위·시스템 영향이 구체적인 승인 기록. 기존 승인을 재사용하되 새 범위를 추정하지 않는다. | Qt추가·원본GUI실행·sourcegateACL 승인후수행; 새 위험작업은 별도범위 검토 |
| AGENT-07 | 분석 실패 원인과 복구 방안 검토. | 실제 실패 로그·원인/가설·재시도 근거·결과. | 실제실패/복구·원인/미확인 기록; 미검증을 통과로 전환하지 않음 |
| AGENT-08 | 소스 없이 구현한 사실을 검증 가능하게 유지. | 허용 입력·세션/권한 통제·freeze manifest. | 원본source미사용·읽기ACL잠금; finalfreeze·A/B강제격리 한계 남음 |
| AGENT-09 | 단계 완료 후 다음 단계 입력·미해결 문제 명시. | phase handoff 문서의 산출물/잔여 문제/게이트 근거. | Phase1/2/3 gate와 다음입력/미확인 전달; Phase4진행 |
| AGENT-10 | 미충족은 정직하게 미완료 보고. | 실제 근거에 따른 상태표·최종 감사. | 실제 suitepass와 desktop/skip/P1/플랫폼 미완료 구분 |

최종 목표는 REA와 AI를 사용한 실제 데스크톱 역공학·재구현 및 정확도/생산성의 정량 검증이다. 코드 작성이나 연결 성공만으로 목적 달성을 선언하지 않는다.
