# ADR-001 — Qt6 Widgets와 Windows 11 시각 설계

결정일: 2026-10-21 (Asia/Seoul). 결정: **C++20 + Qt6 Widgets**. 공통 최소 Qt 버전은 6.4로 두고 Windows의 현재 SDK 6.8.3과 Ubuntu WSL의 6.4.2를 각각 검증한다. 이는 설계 결정이며, 상위 작업이 Phase 2 게이트를 기록하기 전 구현을 시작하지 않는다. 아직 앱 빌드·실행·성능을 검증한 결정은 아니다.

## 입력과 비교

기능 동작의 입력은 [요구사항](../requirements.md), [REA 탐색 분석](../analysis/file-navigation.md), [기능 근거 지도](../analysis/feature-map.md), [원본 실행 관찰](../observations/original-runtime.json)이다. UI의 직접 참고는 실제 호스트 Windows 11 탐색기의 [스크린샷](../observations/screenshots/windows11-reference.png)이며, 이 결정을 작성할 때 이미지를 직접 확인했다. Explorer++의 오래된 메뉴·툴바·배경을 UI 시안으로 복제하지 않는다. 원본 동작과 새로운 시각 설계는 각각 별도로 평가한다.

Windows에는 `tools/qt/6.8.3/msvc2022_64/lib/cmake/Qt6/Qt6ConfigVersionImpl.cmake`의 버전 `6.8.3`과 Widgets 설정·헤더가 현재 존재한다. Linux 6.4.2의 Widgets/Test/Concurrent 가용성은 [환경 조사](../evaluation/environment-audit.md)에 있다. 환경 조사의 초기 Windows Qt 미발견 기록은 당시 관찰이며 현재 SDK 존재와 구분한다. 파일 존재는 링크·실행 성공 증거가 아니다.

아래 성능·개발비 비교는 **설계 예상**이며 Widgets/QML 벤치마크 결과가 아니다.

| 기준 | Qt Widgets | Qt Quick/QML | 결정에 미친 영향 |
|---|---|---|---|
| Windows 탐색기형 UI 재현성 | QTabBar, QTreeView, QTableView, QListView로 탭·트리·상세 목록·선택·헤더를 구성할 수 있다. 평평한 다크 스타일과 상단 바는 직접 조정한다. | 유연한 시각 구성과 애니메이션에 적합하다. 파일 표·트리·키보드 선택·접근성 동작을 별도로 세밀하게 연결해야 한다. | 이 PoC의 핵심은 탭/폴더/표/파일 작업이다. Widgets model/view를 선택한다. |
| 개발 난이도 | C++ 한 언어로 모델·이벤트·테스트를 연결한다. 기존 SDK subset으로 착수할 수 있다. | C++/QML 경계, 타입 등록, context와 배포 모듈을 추가로 다뤄야 한다. | 독립 A/B에 동일한 작은 개발 환경을 제공하기 쉽다. |
| 성능·메모리 | CPU 기반 표와 지연 아이콘 조회가 적합할 것으로 예상한다. 모든 파일을 widget 객체로 만들지 않는다. | scene graph와 GPU 렌더링을 활용할 수 있다. 이 파일 목록에서 어느 쪽이 빠르거나 가벼운지는 미측정이다. | 미측정 우위를 주장하지 않는다. 실제 대량 목록·CPU·메모리 시험으로 확인한다. |
| 사용자 정의 UI | QProxyStyle/stylesheet, delegate와 제한된 QPainter 아이콘으로 Win11 구조를 구현한다. blur/Mica의 정확한 재현은 별도 OS 기능이다. | 세밀한 효과·전환·custom control 표현이 편리하다. | flat 다크 UI의 우선순위가 높고 애니메이션은 필수가 아니므로 Widgets면 충분하다는 판단이다. |
| 크로스플랫폼 동작 | Windows/Linux/macOS Widgets 공통 코드를 재사용한다. OS 파일·Shell 기능은 어댑터로 분리한다. | QML도 공통 UI가 가능하나 현재 Windows 최소 SDK에는 Quick/QML 배포를 검증하지 않았다. | 현재 준비된 Widgets 환경을 양 OS에서 먼저 검증한다. |
| 유지보수성 | C++20 계약·QtTest·CMake target과 model/view 경계가 일치한다. 과도한 stylesheet와 개별 셀 widget은 피한다. | 선언형 UI의 장점이 있지만 QML API/asset/모듈 배포를 추가 유지한다. | 이번 PoC는 기존 C++ 도구와 명시적 계약의 단순성을 우선한다. |

Qt의 [Widgets](https://doc.qt.io/qt-6.8/qtwidgets-index.html), [model/view](https://doc.qt.io/qt-6.8/model-view-programming.html), [Qt Quick](https://doc.qt.io/qt-6.8/qtquick-index.html) 공식 문서는 프레임워크 선택 근거다. Explorer++ 내부 구현을 입증하는 자료로 사용하지 않는다.

## 시각·상호작용 계약

참고 화면은 1241×961 pixels의 실제 Win11 다크 탐색기다. 상단의 활성 탭/닫기/새 탭, 그 아래 뒤·앞·상위·새로고침과 breadcrumb 주소/검색, 다시 그 아래 새로 만들기·파일 작업·정렬·보기·더보기의 command bar, 좌측 탐색, 중앙 상세 목록, 우측 세부 정보, 하단 상태줄을 관찰했다. 다음 수치는 초기 **DIP 설계 근사치**이며 화면의 정확한 픽셀 복제 또는 측정된 사용성 기준이 아니다.

| 영역 | 구현 계약 |
|---|---|
| 창/탭 | 기본 1240×860, 최소 960×600. 약 40 DIP 탭 줄, 선택 탭이 본문과 시각적으로 이어지고 `+`/닫기 제공. OS 창 버튼과 정상 resize·drag를 유지한다. |
| 탐색 줄 | 약 48 DIP. 뒤/앞/상위/새로고침은 작은 outline 아이콘. 주소는 넓은 둥근 dark field, 검색은 별도 field. breadcrumb는 실제 Location에서 만들고 클릭 시 해당 Location으로 이동한다. `Ctrl+L`/`Alt+D`에서는 원본 경로 입력 모드로 전환한다. |
| Command bar | 약 44 DIP. 새로 만들기, 잘라내기/복사/붙여넣기/이름 변경/공유 대신 지원 작업/삭제, 정렬, 보기, 더보기, 세부 정보 토글. 지원하지 않는 명령은 숨기거나 명확하게 비활성화한다. 긴 legacy menubar는 기본 화면에 두지 않는다. |
| Sidebar | 기본 약 190 DIP, splitter로 폭 조절. 홈 역할의 드라이브 목록, 실제 로컬 드라이브와 lazy 폴더 트리. 미구현 Gallery/Cloud/Quick Access를 작동하는 기능처럼 표시하지 않는다. |
| 중앙 | 기본 자세히 보기: 이름·수정한 날짜·유형·크기, 줄 높이 약 26 DIP, 헤더 정렬 표시, 얇은 구분선과 선택/focus 표시. 아이콘/목록 모드는 같은 항목 모델과 선택 키를 사용한다. |
| 세부 정보 | 우측 약 280 DIP, 닫거나 splitter로 조절 가능. 선택 항목 이름·종류·알려진 크기/수정일과 다중 선택 요약. 미확인 메타데이터는 비워 두거나 ‘알 수 없음’으로 표시한다. |
| 하단 | 약 24 DIP. 현재 항목/검색 일치/선택 수, 알려진 선택 크기와 진행 상태. 실제 로딩 중에는 최종 개수처럼 표시하지 않는다. |
| 색·글꼴 | 본문 약 `#191919`, bar `#202020`, 입력 `#2b2d30`, 테두리 `#353535`, 주 텍스트 `#f0f0f0`, 보조 `#aaaaaa`, 파란 focus accent. Windows에서는 가용한 Segoe UI 계열, 다른 OS에서는 시스템 글꼴을 사용한다. |

자산은 자체 제작한 단순 vector/paint 아이콘, Qt의 사용 가능한 표준 자산, 허용된 OS 아이콘 조회를 사용한다. 참고 screenshot이나 Explorer++ 리소스에서 이미지를 추출하여 배포하지 않는다. 정확한 Windows Mica/blur, OS context menu, 썸네일은 별도 capability/P2이며 다크 기본 테마와 혼동하지 않는다. 다크 테마는 이 설계에서 채택하지만 구현·시각 검증 전에는 P2 다크 모드 완료로 표시하지 않는다.

주소/검색/탭/열/목록/트리/작업 버튼에 accessible name, tooltip, 키보드 focus와 충분한 대비를 제공한다. `Ctrl+T`, `Ctrl+W`, `Ctrl+Tab`, `Alt+Left/Right/Up`, `F5`, `F2`, `Ctrl+C/X/V`, `Delete`, `Shift+Delete`, `Ctrl+A`를 정의하고 기능 capability에 따라 동작한다. 키 조합의 존재만으로 파일 작업의 성공을 주장하지 않는다.

FR-07은 **현재 폴더의 파일 이름 검색**으로 구현한다. 원본의 별도 SearchDialog는 `Search Subfolders` 기본 checked가 관찰되었다(OBS-013). 새 UI는 주소 옆 inline 검색을 기본 non-recursive literal 부분문자열 검색으로 제공한다. 이는 요구사항과 Win11 레이아웃에 따른 명시적 동작/디자인 차이이며 원본 검색을 그대로 복원했다는 주장이 아니다. recursive/regex/속성 검색은 별도 P2로 남긴다.

검증은 동일 fixture에서 실제 Win11 참고·Explorer++ 원본·재구현 화면을 각각 보존해 탭/줄 구성/비율/색/포커스/resize를 확인한다. 현재 ADR 작성만으로 시각 재현 또는 성능을 통과시키지 않는다. 모듈·수명주기·시험은 [architecture.md](architecture.md), 정확한 공개 계약은 [interfaces.md](interfaces.md)에 둔다.
