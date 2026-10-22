# 실행과 주요 기능 시연

최종 Windows 앱은 `C:\dev\Windows_fileexplore\dist\windows-20261022T090415366\rea_explorer.exe`다. DLL과 plugins가 든 전체 폴더를 유지해 실행한다. 설치나 개발용 Qt PATH는 필요하지 않다. [최종 평가와 제한](evaluation/final-report.md)을 먼저 확인한다.

## 같은 입력의 화면

| 목적 | 화면 |
|---|---|
| UI 디자인 기준: 실제 Windows 11 탐색기 | [참고 화면](observations/screenshots/windows11-reference.png) |
| 재구현 B 최종 패키지 | [같은 Unicode 폴더](observations/screenshots/rea-final-package-initial.png) |
| 원본 Explorer++ 기능 비교 | [Details](observations/screenshots/explorerpp-unicode-details.png), [실제 검색 결과](observations/screenshots/explorerpp-search-unicode.png) |
| 독립 REA 미사용 A | [동결 A의 최종 render](observations/screenshots/a-frozen-unicode.png) |

자체 QWidget render는 native frame/실제 조작의 증거와 다르다. 실제 desktop 조작은 [B P0](observations/rea-p0-runtime.json), [B P1](observations/rea-p1-runtime.json), [원본](observations/original-runtime.json)에 행동·상태·결과를 기록했다.

## 읽기 기능 시연

1. Ctrl+L을 누르고 `C:\dev\Windows_fileexplore\test-data\fixture-v1\read-only\unicode`를 입력한다. 6개 파일의 한글·공백·é·emoji·긴 이름을 확인한다.
2. Ctrl+F로 `한글`을 검색한다. 현재 폴더의 정확한 두 항목이 남는다. 검색을 비우면 6개로 돌아온다.
3. 주소 breadcrumb의 상위를 클릭하고 Alt+Left/Right, Alt+Up으로 경로와 history를 확인한다. 잘못된 경로를 입력하면 기존 목록이 보존되며 오류가 표시된다.
4. Ctrl+T로 새 탭을 열고 다른 폴더로 이동한다. Ctrl+Tab 전환 시 각 경로가 유지되며 Ctrl+W로 닫는다.
5. 목록의 Name/Date modified/Size header로 정렬하고 Ctrl+Shift+1/5/6으로 Icons/List/Details를 바꾼다.
6. 파일을 더블클릭하면 OS 기본 연결 앱에 위임한다. 원본/B 모두 실제 Notepad 창을 확인한 기록이 있다.

이 가이드는 재현 절차이며 새 실행의 pass를 자동 선언하지 않는다. 1200개 전체 정답·권한 거부·race·late completion은 별도 실제 suite 원장을 따른다.

## 파일 관리 시연과 검증 경계

수정은 읽기 기준 fixture 대신 새 테스트 폴더에서 한다. Ctrl+Shift+N 새 폴더, F2 단일 이름 변경, Ctrl/Shift 다중 선택, Ctrl+C/X/V 복사·이동과 진행 상태를 지원한다. 충돌은 기존 파일을 덮어쓰지 않으며 Keep both로 새 이름 재시도할 수 있다. Delete는 확인 후 휴지통만 요청하고 영구삭제로 대체하지 않는다.

기존 실제 P1 기록은 Unicode 폴더 생성→F2 이름 변경→Ctrl+C/Paste→내용 SHA256 보존, 실제 앱 내부 drag Copy와 같은 hash를 포함한다. OS 앱 간 native drag는 [사용자 보조 outbound Copy](observations/phase6-user-assisted-native-copy.json)의 양끝 25-byte SHA256 일치를 확인했다. [반대 방향 inbound](observations/phase6-user-assisted-native-inbound.json)도 양끝 26-byte SHA256이 일치했다. source가 남아 있어 양방향 Copy 결과이며 native Move 제스처는 미검증이다. 합성 Qt URL 이벤트와 실제 내부 드래그를 이 증거로 대체하지 않는다. 삭제/대량 작업을 시연용으로 자동 실행하지 않는다.
