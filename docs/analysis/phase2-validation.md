# Phase 2 — 조사 자료 게이트

**최소 게이트 통과, 역공학 정확도·재구현 성공은 미검증.** 요구사항의 완료 조건은 P0 주요 동작에 대한 분석 자료 확보 **또는 분석 불가 사유 기록**이다. 각 P0 질문에 실제 정적 자료/부분 결과와 불확실성을 연결했고 원본을 실행해 주요 외부 동작을 관찰했다. 아래 미해결 항목을 성공으로 세지 않고 Phase 3 계약·Phase 4 테스트·Phase 6 정확도 평가로 넘긴다.

## 실제 입력·수행

- 대상과 출처: [target-manifest.json](target-manifest.json)의 공식 EXE 1.4.0.2293/동일 SHA-256. 원본 소스·PDB·기존 역공학 결과를 사용하지 않았다.
- REA Skill v42 + MCP rea-agents 6.7.0 + Ghidra 12.1.4를 실제 사용했다. [71-record 정적 bundle](evidence/phase2-static-bundle.json)은 38,872,790 bytes, [완전 inventory를 추가한 73-record bundle](evidence/phase2-inventory-bundle.json)은 42,720,983 bytes이다. 함수 9,610개와 정의 문자열 4,962개의 실제 목록을 보존했다.
- `list_names`는 101,957개 심볼을 처리했으나 MCP 10 MiB 응답 한계를 넘었다. 완료된 분석은 retained Evidence `ev_bcf098d43636ed7566f5a1628c11403e9db45a26d563a614a0d81fea21369d00`로 남아 bundle export로 재검토했다. 재분석하지 않았다. 이 탐색/serialization 비용은 B의 추가 분석 비용이며 바이트 수로 토큰 비용을 대체하지 않는다.
- 헤더·import/export는 독립적인 보충 바이트 파서로 교차검증하고 REA 자료와 구분했다. [binary-overview.md](binary-overview.md).
- 사용자 승인 후 실제 Explorer++를 실행했다. [공통 실행 관찰](../observations/behavior-summary.md)과 [UIA 원본 기록](../observations/original-runtime.json), 원본 Details/검색 스크린샷을 보존했다. UIA는 일부 조작 직후 혼합 시점 상태·대량 목록 child_limit·Search modal element cache 오류·나중의 반복된 `coordinate input geometry is unavailable`를 보였다. 확인된 안정된 상태만 외부 동작 근거로 사용한다.
- 실제 호스트 Windows 11 탐색기의 같은 fixture 화면도 확인/보존했다. **재구현 디자인은 Win11, 기능 조사 대상은 Explorer++**이며 legacy Explorer++ UI를 구현 결과로 제시하지 않는다.

## P0 자료와 남은 범위

| 기능 | 확보한 자료 | 미확인/후속 검증 |
|---|---|---|
| FR-01 | 실제 원본 주소/트리/목록/탭/상태/검색 화면 및 Win11 디자인 기준 | 재구현 UI/키보드/resize는 아직 없음 |
| FR-02 | REA PIDL→Shell bind→열거→metadata→목록 반영; 원본 empty/Unicode/1,200 items | 전체 항목 이름 대조·비동기 응답성·access denied·link/대규모 성능 |
| FR-03 | REA 경로 parse 및 browse 함수; 주소 Enter와 폴더 더블클릭 실행 관찰 | UI notification에서 간접 dispatch로 이어지는 완전한 주소 매핑 |
| FR-04 | NavigationController RTTI와 index/객체 분기; Back 및 Forward 활성화 관찰 | 상위/Forward 실행·분기 history·탭별 전체 history·내부 command mapping |
| FR-05 | REA FindData/PIDL 정보와 실제 6개 파일의 이름/형식/크기/날짜 | metadata unavailable 표시·Shell 특수 항목·장경로·재파싱 지점 |
| FR-06 | 실제 comparator의 자연 이름·FILETIME·크기 비교; 6개 파일 열 클릭의 순서 | 방향 토글·동률·locale·folder-first 전체 정책·정렬 적용 handler |
| FR-07 | Search command→thread→열거/조건부 재귀→결과 메시지; Unicode 검색 2개 정확 결과 | 현재 폴더만 옵션·case/regex/취소·권한 실패·링크 순환·정확 UI-to-command mapping |
| FR-08 | TabContainer 단서; Ctrl+T 내 PC와 탭별 독립 경로 실행 관찰 | 내부 소유자/생성·닫기 handler·닫기·탭별 history 경계 |
| FR-09 | 입력 PIDL을 ShellExecuteExW에 넘기는 실제 의사코드 | UI→동적 vtable 대상 연결 및 실제 연결 앱 실행을 이번 탐침에서 입증 못함 |
| FR-10 | HRESULT 조건/FindData fallback/변경 감지 정적 경로; 없는 경로에서 기존 목록 유지 관찰 | 실제 access denied·삭제 race·복구 UX. 원본 종료/실패로 일반화하지 않음 |

내부 이벤트 매핑이 미해결인 이유: 이 배포본은 PDB 참조만 있고 Ghidra load state unknown이며 REA는 caller-selected PDB companion을 받지 않는다. 정적 caller 그래프는 해석된 참조 범위이고 여러 COM/가상 호출의 동적 대상은 미해결이다. Windows Ghidra P0는 read-only 분석이며 현재 native runtime-call/UI observation provider는 macOS 전용이다([session capabilities](evidence/phase1-session.json)). UI 자동화는 외부 동작을 관찰하지만 해당 호출의 실행 주소를 증명하지 않는다. 이 경계에서 실제로 얻지 못한 내부 의미는 **미확인**으로 남긴다.

RE-02/03 도식·각 주소·추론·도구 한계는 [file-navigation.md](file-navigation.md), [feature-map.md](feature-map.md)에 있다. RE-04 모든 Shell 예시는 [shell-integration.md](shell-integration.md)의 확보/미확인으로 구분했다. 아이콘/썸네일·properties·recycle·rename 전체를 복원했다고 주장하지 않는다.

## Phase 3 입력과 검증 조건

- 전체 요구사항/14필수 시나리오를 유지한 [matrix](../evaluation/requirements-matrix.md) 및 [실험 protocol](../evaluation/experiment-protocol.md).
- fixture 1,217 data files / bulk 1,200 / 962,374 bytes. 생성·재실행·읽기 검사 통과, manifest SHA-256 `278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1`. 이것은 원본/재구현 기능 테스트 통과가 아니다.
- Windows MSVC/Qt6 실제 smoke compile/link/QtTest 통과. Linux Qt6 기존 환경은 있음, 제품 빌드는 아직 미실행.
- 승인된 소스 접근 게이트: 새 verification/source에는 비소스 probe만 있고 실제 동일 토큰의 파일 읽기·디렉터리 열거가 접근 거부됨을 검증했다. [backup/status](source-gate-status.json). 원본 소스 부재·비열람 유지. Phase 6 freeze 검증 이전 unlock 금지.
- A/B는 아직 실행하지 않았다. A에는 위 공통 실행 관찰/동일 EXE/fixture/요구만 전달하며 REA 자료·B 설계는 제공하지 않는다. 동일 상속 모델의 fresh contexts와 동등한 실험 범위를 실제 시작 시 기록한다.

초기 분석의 총 CPU/메모리/정확 토큰/API 비용과 일부 작업 시간은 미측정이다. 이를 0으로 채우지 않으며 제품/실험 지표와 분리한다. P0/P1 구현·제품 빌드·테스트·최종 소스 정확도·생산성 비교는 모두 다음 단계의 미완료 작업이다.

