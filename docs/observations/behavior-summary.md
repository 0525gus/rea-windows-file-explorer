# 원본 실행 관찰 — 공통 실험 입력

대상: 공식 Explorer++ 1.4.0.2293 x64. SHA-256 `22ac20341c5525658ebe5e9c4b9ac9f7c152b18080ff347861fac3f04d56a2d6`. [실행 관찰 JSON](original-runtime.json)의 실제 UI 조작과 안정된 재조회만 근거로 한다. 이 문서는 REA 분석 자료를 포함하지 않으며 A/B 공통 입력이다. 앱을 새로 만들거나 아래 동작을 재구현한 결과가 아니다.

- 주소창에 fixture의 `read-only`를 입력하고 Enter를 누르면 bulk/empty/errors/navigation/unicode 다섯 폴더가 나타났다.
- empty 더블클릭 후 안정된 상태는 주소가 empty이고 `0 items / 0 bytes`였다. Back으로 read-only의 다섯 폴더로 돌아왔고 Forward가 활성화됐다.
- Ctrl+T는 두 번째 탭을 **내 PC**로 열었다. 첫 탭의 read-only 경로는 유지됐다. 두 번째 탭을 unicode로 옮긴 후 첫 탭 전환 시 read-only가 유지됨을 확인했다. 탭 닫기·탭별 전체 history는 아직 검증하지 않았다.
- bulk 더블클릭 후 주소와 `1,200 items / 939 KB` 상태를 관찰했다. 접근성 목록은 child_limit으로 잘렸으므로 1,200개 이름 모두를 UI에서 대조했다고 하지 않는다. 로딩 시간·프레임 응답성·CPU·메모리는 이 관찰에서 측정하지 않았다.
- unicode 폴더는 6개 항목을 표시하며 한글·공백·특수문자·é·emoji·긴 파일 이름의 접근성 원본 값이 보존됐다. 화면의 좁은 열에서 일부 이름은 말줄임된다.
- View > Details는 Name/Type/Size/Date Modified 열을 표시했다. 파일 크기는 25, 33, 40, 46, 47, 185 bytes이고 날짜는 fixture UTC 시각에 대응하는 로컬 표시였다. 크기 첫 클릭 오름차순, 날짜 안정된 재조회는 11:00→11:05 오름차순이었다. 이름 순서는 café, long_, special_, 공백, 한글_특수문자, 한글_파일이었다. 방향 토글·동률·폴더 우선순위/locale 일반화는 미검증이다.
- 실제 Search 단추는 별도 Search 대화상자를 열었다. Directory는 마지막 성공 위치 unicode였다. Filename `한글`, regex 미선택, Case Insensitive 미선택, Search Subfolders 기본 선택 상태에서 Enter를 실행하면 **0 folders, 2 files found**와 `한글_특수문자_é_😀.txt`, `한글_파일.txt`가 표시됐다. 이 폴더에는 하위 폴더가 없으므로 재귀 정확성을 검증한 사례가 아니다.
- 없는 `errors/does-not-exist` 주소를 제출한 뒤 관찰 창에서는 기존 unicode 목록이 유지됐다. 입력 텍스트는 남았고 조회한 창 목록에 오류 대화상자는 없었다. 비프·모든 오류 경로·장시간 안내 부재까지 입증하지 않는다.
- café.txt를 열려던 첫 탐침은 기본 앱 창을 확인하지 못했고 자동화 geometry 오류도 있어 성공으로 세지 않는다. 후속 OBS-015에서는 café.txt를 실제 더블클릭한 뒤 `café.txt - 메모장` 창을 확인했다. 연결 프로그램에서 파일 내용을 수정하지 않았다.
- OBS-016은 unicode에서 Alt+Up으로 read-only 이동 후 Alt+Left로 unicode 복귀, Forward 활성화를 확인했다. OBS-017은 Alt+Right로 read-only 복귀와 Forward 비활성화를 확인했다.
- OBS-018은 Ctrl+Tab으로 남아 있던 bulk 탭을 전환해 경로와 `1,200 items / 939 KB` 유지, OBS-019는 Ctrl+W로 bulk 탭 닫기 후 남은 read-only 탭의 경로와 다섯 폴더 유지를 확인했다. rename/파일 관리 오류는 아직 별도 검증 대상이다.

조작 직후 일부 UIA 스냅샷은 주소/상태/제목이 서로 다른 시점의 값이었다. JSON에는 즉시 스냅샷과 후속 안정된 상태가 함께 있으며, 즉시 결과를 최종 상태로 간주하지 않는다. 접근성 Description은 공통 tooltip이 여러 행에 반복될 수 있어 행의 실제 텍스트/자식 열 값을 우선한다.

[원본 Details 화면](screenshots/explorerpp-unicode-details.png), [원본 검색 결과](screenshots/explorerpp-search-unicode.png)를 보존했다. 원본 화면의 폴더 pane은 캡처 시 접었다.

## UI 디자인 기준

사용자의 2026-10-21 명확화: Explorer++의 옛 UI를 재현하지 않는다. **실제 호스트 Windows 11 파일 탐색기**를 UI 기준으로 사용한다. [같은 fixture의 실제 Windows 11 화면](screenshots/windows11-reference.png)을 확인했다. 디자인 참고일 뿐 Windows 탐색기의 코드나 자산을 추출한 자료가 아니다.

관찰 레이아웃: 상단 둥근 탭과 +, 그 아래 뒤/앞/상위/새로고침과 breadcrumb 주소+오른쪽 검색, 별도 낮은 명령 모음(새로 만들기/파일 작업/정렬/보기/더 보기), 왼쪽 탐색 영역, 중앙 details 표, 하단 상태줄. 호스트는 어두운 테마다. 재구현은 자체 UI 코드/허용된 OS 아이콘 API·시스템 폰트를 사용한다. 스크린샷을 잘라 UI 자산으로 포함하지 않는다. 선택적 오른쪽 details pane은 P0 필수항목이 아니며 구현 여부는 별도 표시한다.
