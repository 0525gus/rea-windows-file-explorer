# B 정적 분석 — 2026-10-22

최종 UI 변경 뒤 해당 단위만 다시 실행했다. [09:03 실제 runner](measurements/b-final-ui-analysis.metrics.json)는 같은 clang-analyzer checker/MSVC SDK 인자로 exit0, stdout/stderr 모두0bytes, wall16.133748s/CPU15.53125s/peak531,296,256 committed bytes를 기록했다. 검사한 `src/ui/main_window.cpp` SHA256은 `7845db3e0f538c031ef10d5468573fd7f7d33edd9637baad1db79cce5f390e3d`다. 나머지5개 단위는 아래 검증 뒤 수정하지 않았다. 최초6개 검사 표와 새 UI 검사를 혼합해 한 번의 원래 실행으로 부르지 않는다.

Windows의 실제 C++ 분석은 **6개 production translation unit 모두 파싱/실행 성공, 경고 1개**다. 해당 경고는 아래 QObject 소유권 검토에서 false positive로 판정했다. Linux에서는 설치된 C++ 전용 analyzer를 찾지 못했다. GCC 13의 실험적 분석 결과와 한계를 별도로 보존하며, 이를 지원되는 C++ 분석 통과로 세지 않는다. 이 문서는 B 코드만 다루며 A나 원본 소스, 추가 REA 질의를 사용하지 않았다.

## Windows: 실제 clang-tidy

[재현 스크립트](../../scripts/analyze-windows.ps1)는 Windows compile database의 `src/` 6개 단위를 기존 clang-tidy 22.1.3으로 직렬 실행했다. 정확한 선택은 `--checks=-*,clang-analyzer-* --quiet`이고, 설치된 MSVC 14.51.36231 및 Windows SDK 10.0.26100.0 include roots를 명시했다. 60초 단위 timeout을 적용했으며 새 도구를 설치하지 않았다. [보고서](../../build-windows/analysis-20261022T083555323/report.json)에 전체 입력 경로·source SHA-256·각 log/metrics 경로가 있다. 검토 시 6개 source digest가 현재 파일과 일치했다.

| 단위 | exit | 경고 | elapsed s | family CPU s | peak family committed MiB |
|---|---:|---:|---:|---:|---:|
| `src/core/domain.cpp` | 0 | 0 | 5.85 | 5.80 | 543.52 |
| `src/core/session.cpp` | 0 | 1 | 16.15 | 15.72 | 461.39 |
| `src/platform/windows/platform.cpp` | 0 | 0 | 12.93 | 12.67 | 463.43 |
| `src/platform/common/file_operations.cpp` | 0 | 0 | 6.85 | 6.81 | 429.32 |
| `src/ui/main_window.cpp` | 0 | 0 | 11.51 | 11.23 | 490.71 |
| `src/main.cpp` | 0 | 0 | 2.57 | 2.48 | 301.04 |

단위 elapsed 합계 **55.85 s**, process-family CPU 합계 **54.72 s**다. 직렬 실행 중 가장 큰 family committed peak는 **543.52 MiB**다. Windows Job Object의 동시 committed memory이며 RSS/working set과 다르다. 각 runner는 completion 때 active family process 0, 강제 종료한 잔여 owned process 0을 기록했다. 캐시 상태를 통제한 cold/warm 비교나 앱 실행 성능 측정은 아니다. agent token 비용은 측정하지 않았다.

### 남긴 경고와 판정

[원문 경고](../../build-windows/analysis-20261022T083555323/src_core_session_cpp.stdout.log)는 `session.cpp:158`에서 `clang-analyzer-cplusplus.NewDeleteLeaks`: `watcher`의 potential leak이다. 분석 경로는 `createTab → navigate → request`에서 `session.cpp:91`의 `new QFutureWatcher<Result<Listing>>(this)`를 할당한 뒤 함수 반환을 leak으로 해석한다.

실제 소유자는 `ExplorerSession` QObject다. 생성자의 parent 인자 `this`가 watcher를 child로 소유하고, `finished` 연결은 같은 session을 context로 사용하며 94–95행에서 결과를 받고 `deleteLater()`를 호출한다. completion이 처리되기 전 session이 파괴되더라도 destructor는 stop을 요청하고 pool 종료를 기다린 뒤 QObject base가 남은 child를 제거한다. worker는 adapter/입력/stop/progress를 값 또는 shared ownership으로 보유하며 watcher raw pointer를 worker로 넘기지 않는다. 이 객체 소유권은 [Qt 6.8 Object Trees](https://doc.qt.io/qt-6.8/objecttrees.html)와 [QObject destructor](https://doc.qt.io/qt-6.8/qobject.html#dtor.QObject) 계약에 따른다. 정상 completion의 deferred deletion은 [deleteLater 계약](https://doc.qt.io/qt-6.8/qobject.html#deleteLater)에 따른다. event loop 종료 뒤에는 deferred event만으로 삭제를 보장할 수 없으므로 parent 소유를 함께 확인했다.

따라서 **이 특정 경고는 Qt parent/event-loop 소유권을 analyzer가 모델링하지 못한 false positive**로 판정한다. `NOLINT`, checker 비활성화, 임의의 추가 delete 또는 소유권 wrapper를 넣지 않았다. 이 판단은 모든 비동기 동작의 결함 부재나 실행 중 blocking OS call의 강제 취소를 증명하지 않는다.

## Linux: 컴파일 진단과 실험적 GCC 분석

WSL Ubuntu의 설치 도구 확인에서 `clang-tidy`, `clang++`, `cppcheck`, `scan-build`를 찾지 못했다. 사용 가능한 컴파일러는 GCC **13.3.0**이며 Qt **6.4.2**, CMake **3.28.3**, Linux x86_64 환경이다. 기존 build를 `cmake -S ... -B build-linux -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`으로 재구성해 정상 성공했다. 새 패키지를 설치하지 않았다.

GCC 공식 문서는 **13.3 analyzer가 C에만 적합하며**, 완전하거나 sound한 증명 도구가 아니라고 명시한다. 따라서 아래 C++ 실행은 탐색적 진단이지 Linux C++ static-analysis gate 통과가 아니다. `-Wanalyzer-too-complex`는 내부 한도로 분석을 중단한 위치를 드러내기 위해 켰다. [GCC 13.3 공식 설명](https://gcc.gnu.org/onlinedocs/gcc-13.3.0/gcc/Static-Analyzer-Options.html)

첫 `-O0 -fanalyzer` 시도에서 domain은 성공했지만 session의 QtConcurrent/std::variant 경로 확장 중 약 3.6 GiB RSS가 관찰되어 해당 소유 compiler family를 중지했다. [첫 시도 기록](../../build-linux/static-analysis/)을 남겼다. 그 RSS는 순간 관찰값이며 peak/process-family 측정이 아니고, 중단된 session의 완주 시간·CPU를 주장하지 않는다.

후속 [bounded runner](../../build-linux/static-analysis/run_gcc_analyzer_bounded.py)는 원래 compile flags의 `-O3`만 `-O1`로 바꾸고 `-fanalyzer -Wanalyzer-too-complex`를 추가했다. analyzer 한도는 program point당 enodes 4, svalue depth 4, basic-block explosion factor 2, recursion depth 1이다. 직렬 실행, 주소 공간 2 GiB, CPU soft limit 55초, wall timeout 60초와 timeout 시 소유 process-group 종료를 적용했다. `-Wall -Wextra -Wpedantic`도 유지했다. [전체 명령·source digests·결과](../../build-linux/static-analysis/bounded-o1/results.json) 및 단위별 원문 log/time 파일을 보존했다. 모든 source의 실행 전후 digest가 같았고 검토 시 현재 파일과도 일치했다.

| 단위 | exit | unknown 초기화 경고 | leak 경고 | complexity 경고 | elapsed s | CPU s | max RSS KiB |
|---|---:|---:|---:|---:|---:|---:|---:|
| domain | 0 | 0 | 0 | 0 | 2.924 | 2.33 | 329864 |
| session | 0 | 6 | 2 | 2 | 5.128 | 4.55 | 593256 |
| POSIX platform | 0 | 5 | 0 | 0 | 4.327 | 3.67 | 465676 |
| common operations | 0 | 0 | 5 | 18 | 5.541 | 4.95 | 557996 |
| UI | 0 | 5 | 2 | 0 | 12.617 | 11.71 | 1070012 |
| main | 0 | 0 | 0 | 1 | 3.424 | 2.75 | 358640 |

총 **46 경고 = unknown 초기화 16 + leak 9 + complexity/early bailout 21**, 일반 컴파일 오류 0, timeout 0이다. wall 합계 **33.961 s**, user+system CPU 합계 **29.96 s**다. GNU time의 단위별 maximum RSS 중 최대는 **1070012 KiB**다. 동시 process-family RSS 합계를 측정한 값이 아니며 Windows committed metric과 직접 비교하지 않는다. optimization/탐색 한도가 달라 첫 시도와 동등한 분석 범위의 속도 개선이라고 주장하지 않는다.

진단 경로를 다음과 같이 검토했다. 원문을 지우거나 suppression하지 않았다.

- **unknown 초기화 16개:** POSIX `ioError`/`operationNodeValue` 반환과 `NativeOperations hooks`, session/UI의 variant 반환 및 virtual adapter 결과를 가리킨다. 관련 Error/Node scalar는 default 또는 aggregate 초기화되고 QString/std::function은 생성자를 거친다. native stat은 `{}` 초기화 후 성공을 확인한다. 실제 초기화 누락을 특정하지 못했으며, GCC 13의 C++ aggregate/variant/callback 모델 한계가 유력하다. 개별 경고 전체를 재현된 product defect 또는 증명된 false positive로 일괄 판정하지 않는다.
- **Qt future leak 4개:** `qresultstore.h:155`의 ResultStore `new T(...)`를 std::variant move constructor에서 leak으로 보고한다. 설치된 Qt 헤더는 result store로의 소유 이전과 `clear<T>`의 delete를 제공한다. 이 진단은 그 외부 store 수명까지 추적하지 않는다. 소유 이전 모델 한계로 분류하되 leak detector 실행 증거로 바꾸지 않는다.
- **tree leak 5개:** QMap이 쓰는 libstdc++ `_Rb_tree::_M_copy`의 재귀 node 복사를 가리킨다. 설치 헤더는 parent 연결 및 예외 시 `_M_erase`를 제공하고, 같은 영역의 탐색이 complexity 한도에 도달했다. 표준 컨테이너 소유권/재귀 모델 제한이 유력하며 B의 실제 소유권 상실 경로는 확인되지 않았다.
- **complexity 21개:** session의 QtConcurrent return, common의 tree copy, main의 Qt event 경로가 일부 생략되었다. exit 0이어도 완전 조사로 해석할 수 없다.

## 수동 경계 검토와 남은 범위

domain/session 공개 계약은 QtCore/C++20이고 session의 async 실행은 QtConcurrent다. core에 Win32/Shell/COM 호출이 없고, CMake는 Windows와 POSIX adapter 중 하나만 선택한다. shared operation engine의 path native 변환은 해당 경계에 모여 있다. worker adapter shared ownership, tab ID/generation 확인, QObject context completion, owned cleanup manifest/ledger 경계를 source로 확인했다. 플랫폼의 마지막 path lookup과 native mutation 사이 외부 race 및 crash 뒤 ledger 복원 부재는 [기존 구현 한계](platform-implementation.md) 그대로다. 이 검토로 race를 제거했다고 주장하지 않는다.

Windows analyzer 결과는 POSIX/macOS 분기를 컴파일하지 않는다. Linux GCC 실행 역시 macOS API와 Windows 분기를 조사하지 않는다. 생성된 MOC, 테스트 TU, Qt/SDK 구현, OS 호출 내부, 외부 helper 완료, sanitizer/leak-detector 실행 및 전체 경로 exhaustiveness는 이 분석 범위 밖이다. macOS는 [호환성 검토](macos-review.md)만 수행했으며 build/run은 미검증이다. 정적 분석은 실제 기능/권한/취소/cleanup/runtime 검증을 대체하지 않는다.
