# REA Explorer Reconstruction

C++20/Qt6 기반 Explorer++ 동작 재구현 실험이다. UI는 실제 Windows 11 탐색기 관찰을 참고한다. 원본 프로그램 코드나 UI 자산은 포함하지 않는다.

Phase 1–6 PoC 수행·평가를 완료하고 구현·독립 실험·사후 검증 산출물을 제공한다. **실제 OS 앱 간 양방향 Copy는 사용자 보조 검증됐으며 native Move 제스처와 정밀 원본 내부 latency는 미검증/미측정이고 전체 100% 완료는 선언하지 않는다.** [최종 평가](docs/evaluation/final-report.md), [완료 감사](docs/evaluation/phase6-completion-audit.md), [실행·시연 안내](docs/demo.md)를 확인한다. [전체 명세](docs/requirements.md), [초기 결과](docs/evaluation/initial-report.md), [P1 검증](docs/evaluation/p1-validation.md), [초기 요구 추적표](docs/evaluation/requirements-matrix.md)는 동결 시점 증거를 보존한다.

바로 실행할 패키지는 `dist/windows-20261022T090415366/rea_explorer.exe`다. 같은 폴더의 DLL·plugins를 함께 유지한다. 개발용 Qt PATH 없이 독립 두 프로세스 실행을 확인했다. UI 디자인은 실제 Windows 11 탐색기를 기준으로 하며 Explorer++는 기능·동작 비교 대상이다.

## Windows 빌드와 실행

필요 도구는 MSVC x64, CMake/Ninja, Qt6 Core/Gui/Widgets/Concurrent/Test다. 검증 환경과 프로젝트 내부 Qt 설치 위치는 [환경 기록](docs/evaluation/environment-audit.md), [Qt 설치 기록](docs/evaluation/qt-setup.md)에 있다. 아래 스크립트는 기존 도구를 사용하며 설치·시스템 PATH 변경을 하지 않는다.

```powershell
python scripts/create_fixtures.py
python scripts/create_fixtures.py --check
.\scripts\build-windows.ps1 -RunTests
.\tools\qt\6.8.3\msvc2022_64\bin\windeployqt.exe --release --no-translations .\build-windows\rea_explorer.exe
.\build-windows\rea_explorer.exe
```

QtTest 결과는 `build-windows/*-results.txt`와 `*-results.xml`, 단계별 명령·CPU·메모리·시간은 `build-windows/logs/`에 남는다. CMake가 app/test의 runtime DLL을 출력 폴더로 복사하고, build script는 테스트 plugin 경로를 선택한다. Windows 링크 권한 부족으로 건너뛴 시험은 통과로 계산하지 않는다. 배포 패키지 검증은 별도 단계다.

UI의 주소창에 폴더를 입력하거나 왼쪽 트리를 선택한다. Alt+D/Ctrl+L은 주소, Ctrl+F는 현재 폴더 이름 검색, Ctrl+T/W/Tab은 탭 생성/닫기/전환, Alt+Left/Right/Up은 탐색, F5는 새로고침이다. 파일 더블클릭/Enter는 OS 기본 앱에 위임한다. 초기 P0는 자세히 보기와 현재 폴더 literal 검색을 제공한다.

UI 디자인은 [실제 Windows 11 탐색기와 비교](docs/evaluation/ui-design-review.md)한다. Explorer++ 화면은 기능 동작 비교에만 사용한다. 포커스가 없는 주소창에서는 경로 조각을 클릭해 상위로 이동하고, Ctrl+L/Alt+D로 전체 경로를 편집한다. 왼쪽 바로가기는 실제 사용자 표준 폴더 경로를 사용한다.

파일 목록에서 Ctrl/Shift로 선택하고 Ctrl+C/X/V, F2, Ctrl+Shift+N, Delete로 파일 관리를 요청한다. Delete는 확인 후 휴지통으로 보내며 영구삭제로 대체하지 않는다. 충돌은 덮어쓰지 않고 항목별 실패로 알리며 `Keep both`로 충돌 항목만 새 이름으로 재시도할 수 있다. 아이콘/목록/자세히 보기는 Ctrl+Shift+1/5/6 또는 View 메뉴로 바꾼다. 진행 중 작업은 탭을 바꿔도 유지되며 취소·정리 재시도 상태를 명시한다.

`scripts/package-windows.ps1`는 새 dist 하위 폴더에 앱·Qt DLL/필요 플러그인·MSVC runtime·라이선스·앱 소스를 모으고 파일별 SHA256 manifest를 만든다. 설치 프로그램은 실행하지 않는다. `--screenshot`, `--snapshot`, `--view`, `--benchmark` 옵션은 자체 화면·모델·계측 결과를 지정 경로에 저장한다. benchmark는 최초 로딩과 동일 프로세스 5회 refresh를 기록하며 OS cache를 비우지 않는다.

## Linux 빌드

Qt6 개발 패키지와 C++20 컴파일러를 이미 갖춘 환경에서 실행한다.

```sh
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel 2
ctest --test-dir build-linux --output-on-failure
```

Windows 전용 실제 ACL 시험은 Linux에서 적용되지 않는다. macOS 실제 빌드/실행은 아직 검증하지 않았다.

## 실험과 증거

- [분석 대상 및 해시](docs/analysis/target-manifest.json), [실제 REA 분석](docs/analysis/binary-overview.md), [Evidence 인덱스](docs/analysis/evidence/phase2-index.json)
- [폴더 탐색 분석](docs/analysis/file-navigation.md), [Shell 연동](docs/analysis/shell-integration.md), [기능별 근거](docs/analysis/feature-map.md)
- [UI 관찰](docs/observations/behavior-summary.md), [구현의 실제 데스크톱 기록](docs/observations/rea-p0-runtime.json)
- [구조](docs/architecture/architecture.md), [공개 계약](docs/architecture/interfaces.md), [A/B 비교 조건과 한계](docs/evaluation/experiment-protocol.md)

원본 소스는 초기 구현·보고서 freeze commit `64032d766cf634fe00b45aa0d8c951878e0ef5c3` 및 A/B snapshot 재해시 후에만 Phase 6에서 획득·열람했다. [검증 기록](verification/phase6-freeze-verification.json)→[승인된 ACL 복원](verification/phase6-access.json)→[source 획득](verification/source-acquisition.json) 순서를 보존한다. 초기 분석·제품·snapshot에 사후 source 지식을 소급 반영하지 않았다. A/B는 별도 컨텍스트와 입력 지침으로 분리했으며 같은 사용자 아래의 강제 권한 격리는 확보하지 못했다. 정확한 모델 배포 ID·토큰/API 과금은 접근할 수 없어 미측정으로 남긴다.

Qt는 동적 링크를 사용한다. 배포 전 [외부 구성요소·라이선스 기록](docs/evaluation/dependencies.md)을 확인한다. 원본 Explorer++ 실행파일·Qt SDK·빌드 산출물·검증용 원본 소스는 Git에 포함하지 않는다.

## 추가 기능 구현

즐겨찾기, 설정·탭 복원, 변경 자동 반영, 미리보기, 인라인 이름 변경, 상세·재귀 검색, 목록 썸네일, Undo/Redo, Windows 네이티브 메뉴와 Shell 특수 위치 탐색을 추가했다. 사용법과 남은 범위는 [현재 기능 안내](docs/current-features.md)를 참고한다. 이번 변경은 테스트 없이 구현했으며 기존 동결 평가 결과와 구분한다.

현재 구현·빌드·실제 UI 확인·미완료 항목은 [작업 상태](docs/work-status-20261022.md)에 분리해 기록했다.
