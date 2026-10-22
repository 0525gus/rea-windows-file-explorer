# REA Explorer Reconstruction — Phase 6 최종 평가

2026-10-22 Asia/Seoul. 전체 기준은 [requirements.md](../requirements.md)다. Phase 1–6 PoC 수행과 평가 산출물을 완료했다. Windows 실행 앱, 소스 비공개 초기 실험 A/B, 실제 REA/Ghidra Evidence, Windows/Linux 테스트 및 사후 소스 대조 결과를 납품한다. **모든 기능의 완전 복원이나 전체 요구사항 100% 검증은 선언하지 않는다.** FR-17은 사용자 보조 실제 양방향 Copy 전달을 확인했으며 source-removing native Move 제스처는 아직 미검증이다. warm refresh 사전 목표도 부분 미달이며 아래 한계를 유지한다.

현재 결과는 [기계 판독 요약](phase6-summary.json), [기능 비교](phase6-functional-comparison.md), [완료 감사](phase6-completion-audit.md), [시간·리소스 원장](phase6-resources.md)으로 추적한다. 초기 시점 문서의 “source unread / Phase 5 진행 중” 문구는 당시 증거다. 이를 새 지식으로 덮어쓰지 않으며 현재 상태는 이 보고서와 Phase 6 successor 문서를 따른다.

## 실행 가능한 결과

Windows 패키지: `C:\dev\Windows_fileexplore\dist\windows-20261022T090415366\rea_explorer.exe`. 같은 폴더의 DLL·plugins·notices·source를 함께 유지한다. Qt 개발 PATH나 추가 Qt 설치 없이 서로 다른 PID의 초기 실행과 재실행이 정상 종료했고 Unicode 6개 모델 snapshot을 남겼다. app SHA-256은 `bed10b64b0dbfb13606b91d8647f7c99f1062fff455a4730547f7d8d51e3c1fb`다. 독립 감사에서 패키지 manifest 195개 파일을 대조했다. [실행1](measurements/b-final-package-initial.metrics.json), [실행2](measurements/b-final-package-restart.metrics.json), [시연 안내](../demo.md)를 참조한다.

UI 기준은 **호스트의 실제 Windows 11 파일 탐색기**다. Explorer++는 역분석·기능 동작 비교 대상이다. [Win11 참고](../observations/screenshots/windows11-reference.png), [최종 앱](../observations/screenshots/rea-final-package-initial.png), [UI 검토](ui-design-review.md)를 보존했다. 탭·주소 breadcrumb·명령 모음·실제 표준 폴더·목록·상태줄·세부 정보 영역을 구현했다. native frame, 아이콘, 영문 문구·날짜/바이트 표시와 사용자 고정 폴더에는 차이가 남는다. Microsoft UI 자산을 추출하지 않았다.

사용자가 보고한 DLL 문제는 직접 테스트 프로세스의 `Qt6Test.dll` 검색 실패로 재현됐다. CMake가 각 Windows executable의 runtime DLL을 출력 디렉터리에 복사하도록 초기 구현에서 수정했고 clean-PATH domain 시험 8 pass를 확인했다. 이 문제와 최종 앱 패키지 실행을 구분한다([기록](measurements/qt-dll-resolution.txt)).

## 순서와 소스 비공개 조건

| 단계 | 실제 확보한 결과와 게이트 |
|---|---|
| Phase 1 | 연결 확인에 더해 작은 Windows PE와 대상 EXE의 실제 REA/Ghidra 함수·호출·비어 있지 않은 의사코드 분석 및 재열람 |
| Phase 2 | 공식 대상 식별, 주요 원본 동작 관찰, P0 질문별 부분 근거와 구체적 분석 한계 기록. 모든 이벤트 복원 성공과 다름 |
| Phase 3 | Qt Widgets ADR, domain/application/platform/UI 계약과 테스트 경계 확정 |
| Phase 4 | Windows C++20/MSVC 제품과 P0 전체 기능의 실제 시험 통과 |
| Phase 5 | P1 구현, 실제 Win11 기반 UI 개선, Windows/Linux suite·package·정적 분석. native cross-app drag 및 아래 제한 보존 |
| Phase 6 | 초기 A/B snapshot 고정·재해시·Git commit을 검증한 다음 ACL 복원·원본 tag source 획득. 사후 구조 정확도·기능·성능·리소스 비교 |

원본 대상은 Explorer++ **1.4.0.2293 x64**, 4,100,096 bytes, SHA-256 `22ac20341c5525658ebe5e9c4b9ac9f7c152b18080ff347861fac3f04d56a2d6`([manifest](../analysis/target-manifest.json)). freeze commit은 **`64032d766cf634fe00b45aa0d8c951878e0ef5c3`**다. B snapshot 354개, A snapshot 124개 파일을 소스 접근 전과 사후에 실제 재해시했다. 초기 주장 원장 SHA-256 `aaa44d0e410267784b116a95cf3ce9e46c0182b212221ce724ab3e385a02156a`와 B 제품·테스트 바이트는 사후에도 유지됐다. A/B snapshot 및 초기 보고서를 사후 소스 지식으로 수정하지 않았다.

검증된 freeze는 `2026-10-22T00:06:24Z`, 승인된 source directory DACL 복원은 `00:06:47Z`, 첫 원본 source acquisition 기록은 `00:07:07Z`다. [freeze 검증](../../verification/phase6-freeze-verification.json), [ACL 복원](../../verification/phase6-access.json), [획득 메타데이터](../../verification/source-acquisition.json)가 순서를 보존한다. 초기 실제 NTFS list/read 거부와 백업은 별도 보존했다. 같은 사용자·owner/관리자의 복원 능력까지 막는 보안 경계는 아니며 A/B 도구 차단도 강제하지 못했다.

## 실제 REA 분석과 사후 정확도

REA 6.7.0/Ghidra 12.1.4/JDK21을 실제 사용했다. 9,610개 함수·4,962개 정의 문자열 inventory, 519 imports/22 DLL, PE 구조·resources 및 native pseudocode/참조를 보존했다. [분석 개요](../analysis/binary-overview.md), [초기 주장 25개](../analysis/initial-claims.json), [탐색 소스 대조](phase6-navigation-source.md), [Shell 소스 대조](phase6-shell-source.md)에 근거와 한계가 있다. UI 관찰·보충 바이트 파서를 REA Evidence로 둔갑시키지 않는다.

| 고정된 초기 질문 25개 | 사후 판정 |
|---|---:|
| 좁은 양의 주장 내용이 같은 release source와 부합 | 12 |
| 일부만 확인: Boost mkdir wrapper 귀속 | 1 |
| 처음부터 unknown, 초기 회복 성공 credit 없음 | 12 |
| 정확한 native VA ↔ source symbol linkage 입증 | 0 |
| 완전한 end-to-end 기능 역공학 성공 입증 | 0 |

부분 양의 근거 확보 범위는 **13/25(52%)**다. 모든 양의 질문도 end-to-end 일부만 설명한다. 양의 주장 중 12/13 내용 부합·1부분이라는 값은 full-feature 정확도나 exact binary mapping 정확도가 아니다. unknown을 정답인 음성으로 계산하지 않고, 공유 근거를 독립 관측으로 세지 않는다.

공식 `version-1.4.0` tag commit `384c2f687fd55c1e71e9fcb272f9113de009a248`, archive SHA-256 `3531f0ba75b5bfb64be4674a960b57c62c2adf62a3b04578e485e2442fdbc655`를 사후 획득했다. source의 1.4.0은 확인되지만 네 번째 build number는 환경에서 주입된다. `.2293`의 정확한 빌드 기록·PDB·link map·재현 빌드는 확보하지 못했으므로 VA에 source 함수명을 확정해 붙이지 않는다. 양쪽 소스 감사는 총 133개 해시·행 범위 reference를 보존한다.

source가 확인한 PIDL/Shell 열거·metadata·list materialization·비교·조건부 검색 worker·빈 verb의 PIDL 실행·watcher·drop 위임·label-edit 시작은 초기 좁은 내용과 부합한다. **SHFileOperation 전송 wrapper와 다른 IFileOperation 명령 경로**, label-edit 시작과 rename commit, Boost helper와 New Folder UI를 분리해야 한다. source에서 새로 알게 된 오류·thumbnail·properties·delete/rename/mkdir 경로는 초기 REA 성과로 소급하지 않는다.

구현에서 REA를 실제 활용한 하위 범위는 FR02/03/05/06/09의 경로·열거·metadata·비교·Shell 위임이다. FR07은 요구사항의 현재 폴더 literal inline 검색으로 새로 설계했으며 원본 SearchDialog의 기본 재귀 UX와 다르다. P1 staging/no-replace/identity/cleanup·Qt clipboard/DnD·trash는 일반 API와 새 안전 정책이다. 원본에도 같은 engine이 있었다고 주장하지 않는다([기여 표](requirements-matrix.md#기능별-구현기여검증-5)).

## 기능·동작·안정성·이식성

P0 FR01–10은 Windows 실제 기능 시험을 통과했다. P1 FR11–20은 구현과 자동화 실행 경로가 있으나 FR17은 실제 사용자 보조 양방향 Copy와 Qt Move/drop 시험이 있으며 source-removing native Move 제스처는 아직 미검증이다. [고정 FR20/T14 비교표](phase6-functional-comparison.md)에서 초기 A/B 모두 **19/20 핵심 기능 경로에 실행 근거, FR17은 부분 확인**으로 집계했다. 사후 사용자 보조 양방향 Copy와 실제 Qt Move/drop·내부 native Copy를 합친 현재 B의 제한된 core-route inventory는 **20/20**, A는 **19/20**이다. 이는 모든 modifier·platform·충돌·원본 충실도를 검증한 100% 비율이 아니며, B만 사후 사용자 보조 검증이 추가된 조건 차이도 보존한다. 고정 14시나리오 중 세 앱의 **좁은 공통 결과 6개, 부분 비교 2개, 원본 쪽 미검증 6개**다. 6/14를 완전 동작 일치율로 바꾸지 않으며 full-scenario 일치율은 미확정이다. 원본과 비교한 관찰 결과가 있는 항목만 일치 credit을 준다. 같은 예외 프로토콜의 완전한 A/B 안정성 비교율도 미확정이며 개별 실제 시험 결과와 skips를 아래에 그대로 보고한다.

| 실제 최종 회귀 | B | A (독립·REA 미사용) |
|---|---|---|
| Windows suites | 5/5, 70 pass/4 skip | 5/5, 51 functional pass/2 skip |
| Linux suites | native ext4 5/5, 69 pass/5 skip | 5/5, 50 functional pass/3 skip; WSL mount 제한 별도 |
| pass 집계 경계 | 각 B 합계에 init/cleanup 10개 포함: functional Windows60/Linux59 | A functional 합계는 init/cleanup 제외 |
| macOS | source/API 호환성 검토, build/run 미검증 | source 검토, build/run 미검증 |

다른 suite의 case 수를 직접 우열이나 기능 구현률로 바꾸지 않는다. B [보존 회귀 로그](validation-logs/b-initial-final-20261022T090136/test.stdout.log), [Linux 로그](validation-logs/b-initial-final-20261022T090136/linux-ctest.txt), [A 최종 보고서](../../experiments/A/docs/evaluation/p1-report.md), [P1 상세](p1-validation.md)를 따른다. 초기 실패도 남겼다.

실제 Windows ACL 거부·missing/stale 외부 삭제·rename/copy/move 충돌·원본/이웃 해시·mixed 결과·취소·shutdown drain·cleanup 실패 후 identity replacement 거부/재시도를 검증했다. Windows junction/cycle와 Linux symlink no-follow는 실제 시험했다. 일반 Windows symlink 권한 부족은 skip이다. B trash는 별도 opt-in 시험에서 실제 소유 파일이 사라지고 이웃 해시가 보존됐으며 영구삭제 fallback이 없다. 휴지통에서 복원까지 검증한 것은 아니다. native cross-volume move·장경로·모든 reparse 변형·악의적 TOCTOU 원자성·crash 후 cleanup journal은 미검증/미지원이다.

Linux native ext4 성공이 DrvFS 결과를 지우지 않는다. B DrvFS directory publication은 별도 **12 pass/2 fail/1 skip**이며 directory copy/mkdir no-replace가 Unsupported다. A의 DrvFS Qt rename fallback도 적대적 경합에 대한 원자성을 입증하지 못했다. [macOS 검토](macos-review.md)는 실행 증명이 아니다.

## 같은 fixture의 성능 비교

같은 Windows 호스트·Qt6.8.3/native windows QPA·1200개 fixture·shown Details UI에서 A→B를 순차 실행했다. 최초 목록+동일 process 5 refresh 모두 정확한 전체 이름 hash `ff84bde034b35b40f6a7d8ef0e85c04966d069644ac12ea9db0408591d4fa8b0`다. OS cache는 flush하지 않았다. 한 trial이므로 통계적 우열을 일반화하지 않는다.

| 측정 | A | B |
|---|---:|---:|
| 첫 listing, ms | 120 | 1064 |
| warm refresh 5회, ms | 123,124,230,217,113 | 1036,999,1006,1013,1048 |
| warm median, ms | 124 | 1013 |
| 5ms timer 최대 gap, ms | 121 | 14 |
| process-family elapsed, s | 1.9018162 | 6.4730558 |
| process-family CPU, s | 1.875 | 6.609375 |
| peak family committed, bytes | 57,180,160 | 27,992,064 |

A [raw](measurements/a-phase6-common.json)/[Job metrics](measurements/a-phase6-common.metrics.json), B [raw](measurements/b-phase6-common.json)/[Job metrics](measurements/b-phase6-common.metrics.json)를 보존했다. A의 validation-only harness는 기존 A library·동결 public header에 링크했다([source/동일성 manifest](../../verification/benchmarks/README.md)). 기존 A hidden-window/3-refresh 결과를 이 비교로 대체하지 않는다. A는 public global-idle API가 없어 committed folder/loading/operation busy/정확 model names 경계이고 B는 session idle까지 확인한다. A는 일반 extension type, B는 Windows Shell type·추가 identity를 조회한다. UI/style와 처리 내용이 달라 REA 사용만의 속도 효과로 해석하지 않는다. A setup 중 328ms event는 listing timer 시작 전이며 최대 heartbeat만으로 전체 startup 응답성을 주장하지 않는다.

B 사전 목표(첫≤2s, 모든 warm≤1s, GUI gap≤100ms, peak≤256MiB)는 첫 listing·gap·memory를 충족했고 **warm은 1/5만 충족**했다. 목표를 소급 완화하지 않았다. 초기 Type 열 font paint의 약330ms 지연을 명시 font로 완화한 증거와 실패한 probe도 초기 원장에 보존했다. 비교 시점 제품 코드는 source 접근 전 freeze와 같다.

원본도 같은 bulk에서 Details 전환 후 F5 5회와 매회 1200 status, 정상 종료를 관찰했다([원본 작업](measurements/original-phase6-bulk-observations.json), [Job metrics](measurements/original-phase6-bulk.metrics.json)): whole process143.5496274s/CPU104.265625s/peak78,331,904 bytes. 원본의 첫 icon→Details 전환·Shell tree·viewport·설정·CUA 대기/관찰, 내부 ready/heartbeat 미확보가 A/B와 다르다. 이 wall 값을 listing latency로 쓰거나 숫자로 원본보다 빠르다고 결론내리지 않는다. 이 관측만으로 정밀 내부 ready latency를 비교할 수 없으며, 아래의 별도 공통 외부 작업 비교와 구분한다. 앞선 중단된 exploratory original run도 별도 보존했다.

추가로 원본/B 모두 빈 폴더에서 Details를 준비하고 **2560×1392 최대화**, 같은 bulk 경로 Enter 및 F5 5회를 외부 Computer Use 관측으로 수행했다([사전 프로토콜](../../verification/benchmarks/external-protocol.json), [공통 외부 비교](phase6-external-comparison.md)). 이것은 같은 외부 작업 조건의 비교이며 앞선 원본 icon→Details exploratory run과 별도다.

| 공통 외부 작업 전체 process family | 원본 | B |
|---|---:|---:|
| elapsed, s | 189.833273 | 179.392269 |
| CPU, s | 122.25 | 50.046875 |
| peak committed, bytes | 70,504,448 | 35,995,648 |

양쪽 모두 6번의 action 뒤 올바른 bulk 경로·1200 status, exit0·잔여 process0을 확인했다. 측정에는 startup/preparation/UIA provider 작업·도구/agent 대기·종료가 포함되며 listing 전용 CPU가 아니다. 별도 도구/model CPU는 미측정이다. 원본 준비의 사용자 입력 중단·Details 선택과 B 최대화 UIA geometry 실패 후 복구도 포함한다. B 첫 탐색과 F5 3에서는 Loading→ready를 관찰했지만 나머지 F5 4개는 즉시 기존 1200 status여서 새 generation 완료를 증명하지 못했다. **외부 작업 관측 비교는 완료했으며 내부 ready·paint·heartbeat의 정밀 원본 계측은 미측정**이다. subsecond status 확인을 새 listing 시간으로 쓰거나 속도 비율·REA 효과를 산출하지 않는다.

## 개발 리소스·모델·비용과 REA의 효과

| 초기 supervised Windows 기록만 집계 | A | B |
|---|---:|---:|
| 중복 제거 stage / 실패 | 55 / 9 | 81 / 10 |
| 기록 wall 합, s | 456.059244 | 839.1797683 |
| 기록 CPU 합, s | 524.4375 | 852.390625 (80/81 coverage) |
| 최대 관찰 Job committed, bytes | 743,936,000 | 727,027,712 |

원자료·해시·alias와 공유 준비/Phase6 제외 경계는 [리소스 감사](phase6-resources.md)에 있다. B timeout 1개는 CPU/peak가 없어 합계가 부분적이다. 겹친 stage wall 합은 활성 개발 시간이나 calendar duration이 아니다. failed/incremental/no-work build도 포함한다. max peak는 서로 다른 Job peak의 합이 아니다. 이 계측에는 full REA/Ghidra process family, IDE/에이전트, Linux 전체 누적 resource가 포함되지 않는다.

REA canonical Evidence는 82개 ID/40,915,307 stored bytes가 식별되지만 전체 조사 task의 elapsed/CPU/peak는 미측정이다. 설정 memory budget이나 PE timestamp를 소비량/분석시간으로 읽지 않았다. 모델은 parent model/reasoning 상속·override 없음만 확인되며 정확 배포 ID·동일 weights/샘플링·per-arm input/output/cache/reasoning token·API billing 자료는 접근 불가다. 비용을 0으로 쓰거나 파일 크기로 토큰/과금을 꾸미지 않는다. Usage interruption을 포함한 wall span도 정밀 활성 개발 시간으로 소급하지 않는다.

실험은 REA가 **내부 동작의 검증 가능한 설명**을 제공했고 이를 새 platform 계약에 활용할 수 있음을 보였다. A도 REA 없이 주요 일반 파일 탐색기 기능을 구현했다. 동일 모델 배포·예산·강제 정보 격리·처리량이 완전히 통제되지 않았고 B 분석 선행 비용도 전부 계측하지 못했으므로 “REA가 더 빠르다/싸다/정확한 제품을 만든다”는 인과적 생산성 이점은 입증하지 못했다. 이 한계를 포함한 독립 A/B 산출물 비교가 결과다.

## 남은 범위와 완료 판정

UI/공통 코어/platform adapter 분리, 실제 Windows P0·package, Linux 실행 결과, 소스 접근 전 freeze, 실제 REA 근거/추론 구분, 사후 source 대조와 A/B 리소스 보고가 있다. 정적 분석·license/notices·공개 계약·시연 자료도 제공한다([static](static-analysis.md), [dependencies](dependencies.md), [interfaces](../architecture/interfaces.md)).

FR17의 source-removing native Move 제스처와 정밀 원본 내부 latency는 **미검증/미측정 범위**다. [최신 native 시도](../observations/phase6-native-drag-attempt.json)는 fixture 해시를 보존했지만 실제 전달을 입증하지 못했다. 이후 [사용자 보조 outbound Copy](../observations/phase6-user-assisted-native-copy.json)는 실제 source `app-source/app-target/outbound.txt`와 destination `os-source/outbound.txt`가 모두 25 bytes, 기존 baseline SHA256과 같음을 확인했다. 사용자 요청/보고와 파일 결과를 근거로 하며 자동화 gesture 캡처나 REA Evidence는 아니다. 중간 폴더 이동은 관찰하지 않았다. 이후 [사용자 보조 inbound 전달](../observations/phase6-user-assisted-native-inbound.json)도 실제 도착을 확인했다. OS source와 B destination의 `inbound.txt`는 각각 26 bytes이고 baseline SHA256이 같다. 양쪽 원본이 모두 남아 있으므로 양방향 실제 결과는 Copy이며 source-removing Move로 바꾸지 않는다. 내부 native Copy·Qt external-URL 수신 4 rows의 pass도 별도 범위로 유지한다. 사용자가 실제 양방향 전달을 수행했고 위 두 record에서 양끝 파일을 확인했다. 사용자는 추가 native Move를 미검증으로 남기기로 했으므로 더 요구하지 않는다. B warm 속도 목표 미달도 남은 성능 개선이다. 이번 초기 실험은 freeze됐으므로 후속 개선은 별도 revision으로 구분해야 한다.

P2는 부분 구현이다. dark UI와 일부 안전 취소/cleanup 흐름은 있으며 thumbnail/preview, native OS context menu, 고급/재귀 검색, 실시간 watcher, Undo/Redo, 설정 저장, Windows Shell 특수 위치 등은 완성으로 표시하지 않는다. Windows11 화면과 완전히 같은 native chrome/아이콘/문구도 목표 결과로 주장하지 않는다.

[완료 감사](phase6-completion-audit.md)에서 §13의 11개 완료 기준과 Phase 1–6 산출물을 실제 증거로 대조했다. FR17/T10은 별도의 물리적 Shift-drag 전수 재현을 명시하지 않는다. 양방향 사용자 보조 native Copy, 실제 파일 source 제거와 destination hash를 검사하는 Qt Move/drop, 별도 실제 충돌 시험을 종합해 제한된 PoC 수행·평가는 완료했다. 미검증 gesture 변형·정밀 내부 timing·platform/비용 한계·목표 미달과 완전 복원 주장을 구분하며, 100% 보편 검증으로 표시하지 않는다.
