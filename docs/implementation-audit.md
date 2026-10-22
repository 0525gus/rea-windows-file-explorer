# 현재 기능 구현 감사 (2026-10-22)

범위: 사용자 요청인 남은 기능 전체 구현. 테스트와 앱 실행은 수행하지 않는다. 이 감사는 소스 연결, 컴파일, 배포 입력에 대한 확인이며 기능 동작 테스트 통과를 의미하지 않는다. 기존 동결 연구 보고서의 수치와 분리한다.

## 이번 남은 다섯 항목

| 항목 | 실제 구현 및 사용자 진입점 | 확인 범위 |
|---|---|---|
| Undo/Redo | `FileOperations::execute/history/replay`, OS 휴지통 receipt/restore, `ExplorerSession::beginOperation`, `MainWindow::replayOperation`, Ctrl+Z/Y/Shift+Z 및 작업 메뉴 | 복사/이동/이름 변경/새 폴더/휴지통 성공 항목의 기록·반대 작업 연결, 부분 실패/취소 분리, 객체/전체 트리 검증·충돌 방지·교차 볼륨 식별자 연결 소스 확인 |
| 네이티브 메뉴 | `windows_shell::menu`, `WindowsPlatform::showContextMenu`, `MainWindow::showNativeMenu/showFileMenu` | IContextMenu와 IContextMenu2/3 메시지 전달·명령 실행, 항목/배경 메뉴, COM/menu/PIDL 수명 관리, Shift+우클릭과 앱 메뉴 연결 |
| Shell 특수 위치 | `windows_shell::parse/parent/list/open`, Windows adapter 분기, 왼쪽 Windows Shell 그룹 | 가상 namespace 분리, 실제 열거·상위/하위 탐색·기본 동작, 파일시스템/가상 항목 구분, 탭/즐겨찾기 복원 경로 |
| 목록 썸네일 | `EntryModel::requestThumbnail/data`, Qt 이미지 디코더, `windows_shell::thumbnail` | 비동기 worker, 요청 수/이미지 크기/캐시 한도, 메타데이터 기반 캐시 키, 디코더·네이티브 공급자·기본 아이콘 경로, View 토글·설정 저장 |
| 재귀 검색 | `recursiveListing`, `setSearch/setRecursiveSearch/cancelSearch`, 검색 옵션 메뉴 | 이름·상세 필터, 폴더 트리 탐색, 링크 진입 방지, 중복 방문/자원 한도, 접근 오류 경고, 요청 세대별 취소, 입력 지연 처리, 실제 부모에서 이름 변경·원래 폴더 열기 |

## 요구사항 §5.3의 전체 P2 목록

| 항목 | 현재 구현 위치 |
|---|---|
| 썸네일 및 파일 미리보기 | `EntryModel`, `MainWindow::requestPreview/loadPreview`, `windows_shell::thumbnail` |
| OS 컨텍스트 메뉴 연동 | `windows_shell::menu`, `MainWindow::showNativeMenu` |
| 즐겨찾기 및 빠른 접근 | `MainWindow::toggleFavorite/rebuildFavorites` |
| 상세 검색 필터 | `filterEntries` (ext/type/size/after/before/name) |
| 파일 변경 실시간 반영 | `MainWindow::syncWatches`, `QFileSystemWatcher`·지연 새로고침 |
| 복사·이동 작업 취소 | 기존 `ExplorerSession::cancelOperation`, `FileOperations` stop-token 경로 |
| Undo/Redo | `FileOperations::history/replay`, 플랫폼별 휴지통 복원 |
| 다크 모드 | 기존 `MainWindow` 다크 스타일시트 |
| 사용자 설정 저장 | `MainWindow::saveSettings`와 생성자 복원, QSettings |
| Windows Shell 특수 위치 지원 | `windows_shell` 및 Windows adapter |

## 컴파일 및 패키지

- Windows: 설치된 MinGW-w64 UCRT64 GCC 16.2.0 / Qt 6.11.1, Release `rea_explorer` target.
- Linux: GCC 13.3.0 / Qt 6.4.2, Release `rea_explorer` target.
- 두 앱 target 모두 컴파일·링크 성공. 테스트 target 빌드와 CTest, 앱 실행은 수행하지 않음.
- 새 Windows 패키지는 실제 변경된 작업 트리 소스와 새 `shell_services` 파일을 포함한다. JPEG/GIF/ICO 플러그인과 각 PE의 비시스템 DLL 의존성을 포함한다.
- 패키지 `verification/`에 두 컴파일 로그, 소스 해시, 의존성 목록과 `tests_run: false`, `runtime_validation_run: false`를 기록한다.

OS/볼륨·namespace 공급자·휴지통·자원 한도 및 사용자 사용법은 [현재 기능 안내](current-features.md)에 명시한다. 새로 명시된 다섯 기능 중 소스 구현이 빠진 항목은 없다. 동작·회귀 검증은 사용자 요청에 따라 후속 단계로 남긴다.

후속 UI/UX 일부 실행 확인과 중단 시점의 완료/미완료 구분은 [작업 상태](work-status-20261022.md)를 참고한다. 위의 테스트·실행 생략 기록은 기능 구현/컴파일 단계의 범위이다.
