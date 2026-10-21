# REA Explorer Reconstruction

C++20/Qt6 기반 Explorer++ 동작 재구현 실험이다. UI는 실제 Windows 11 탐색기 관찰을 참고한다. 원본 프로그램 코드나 UI 자산은 포함하지 않는다.

현재 Phase 4 P0 검증 중이다. Windows 빌드와 네 개 자동화 suite가 통과했으며 실제 데스크톱에서 폴더 탐색·검색·파일 연결 프로그램 실행을 확인했다. P1 파일 관리 기능은 아직 구현하지 않았다. 전체 요구사항 완료나 원본의 완전한 복원을 주장하지 않는다. [전체 명세](docs/requirements.md), [검증 상태](docs/evaluation/p0-validation.md), [요구 추적표](docs/evaluation/requirements-matrix.md)를 함께 확인한다.

## Windows 빌드와 실행

필요 도구는 MSVC x64, CMake/Ninja, Qt6 Core/Gui/Widgets/Concurrent/Test다. 검증 환경과 프로젝트 내부 Qt 설치 위치는 [환경 기록](docs/evaluation/environment-audit.md), [Qt 설치 기록](docs/evaluation/qt-setup.md)에 있다. 아래 스크립트는 기존 도구를 사용하며 설치·시스템 PATH 변경을 하지 않는다.

```powershell
python scripts/create_fixtures.py --check
.\scripts\build-windows.ps1 -RunTests
.\tools\qt\6.8.3\msvc2022_64\bin\windeployqt.exe --release --no-translations .\build-windows\rea_explorer.exe
.\build-windows\rea_explorer.exe
```

QtTest 결과는 `build-windows/*-results.txt`와 `*-results.xml`, 단계별 명령·CPU·메모리·시간은 `build-windows/logs/`에 남는다. Windows 링크 권한 부족으로 건너뛴 시험은 통과로 계산하지 않는다. 배포 패키지 검증은 별도 단계다.

UI의 주소창에 폴더를 입력하거나 왼쪽 트리를 선택한다. Alt+D/Ctrl+L은 주소, Ctrl+F는 현재 폴더 이름 검색, Ctrl+T/W/Tab은 탭 생성/닫기/전환, Alt+Left/Right/Up은 탐색, F5는 새로고침이다. 파일 더블클릭/Enter는 OS 기본 앱에 위임한다. 초기 P0는 자세히 보기와 현재 폴더 literal 검색을 제공한다.

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

원본 소스는 Phase 6의 초기 구현·보고서 freeze 전까지 다운로드·열람하지 않는다. `verification/source`의 ACL은 그 게이트 전 복원하지 않는다. A/B는 별도 컨텍스트와 입력 지침으로 분리하며 같은 사용자 아래의 강제 권한 격리는 확보하지 못했다. 정확한 모델 배포 ID·토큰/API 과금도 접근할 수 없어 미측정으로 남긴다.

Qt는 동적 링크를 사용한다. 배포 전 [외부 구성요소·라이선스 기록](docs/evaluation/dependencies.md)을 확인한다. 원본 Explorer++ 실행파일·Qt SDK·빌드 산출물·검증용 원본 소스는 Git에 포함하지 않는다.
