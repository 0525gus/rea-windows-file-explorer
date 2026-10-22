# Windows 11 UI 관찰과 구현

디자인 기준은 실제 호스트 **Windows 11 파일 탐색기**다. Explorer++는 기능 동작·REA 분석·성능 비교의 대상이며 디자인 원본으로 사용하지 않는다. 2026-10-22 실제 Windows 탐색기에서 같은 `fixture-v1/read-only/unicode`를 다시 열어 탭, 주소 경로, 명령 바, 탐색 영역, 자세히 목록과 세부 정보 영역을 확인했다.

[Windows 11 참고 화면](../observations/screenshots/windows11-reference.png)과 [개선된 자체 앱 화면](../observations/screenshots/rea-win11-aligned-final.png)은 같은 6개 입력 파일을 표시한다. 후자는 앱의 QWidget 렌더링 캡처로 native window frame은 포함하지 않는다. 직접 desktop 조작은 [별도 관찰 원장](../observations/rea-p1-runtime.json)에 보존한다.

이번 변경은 주소의 클릭 가능한 상위 경로와 Ctrl+L/Alt+D 전체 경로 편집, 실제 Qt standard locations의 Home/Desktop/Downloads/Documents/Pictures 바로가기, 22px 목록 행, 더 잘 맞는 열 너비, 더 큰 세부 정보 영역, 선택 파일에 맞는 자체 파일/폴더 아이콘이다. 바로가기는 OS가 반환한 실제 경로로 이동하며 가짜 OneDrive/Gallery/Network 항목을 넣지 않는다. 기존 root tree는 비동기 열거를 유지한다. tree worker의 대상 소유권도 raw-pointer map에서 persistent model index로 바꾸어, 행이 제거되면 late completion이 무효 대상을 사용하지 않게 했다.

Microsoft의 아이콘이나 UI 리소스를 추출하지 않았다. QPainter로 만든 아이콘과 시스템 폰트를 사용한다. 모든 Windows 11 Shell 기능을 그대로 복제했다는 뜻은 아니다. Native title frame, 주소 경로 생략 정책, 영문 UI 문구, 파일 크기 단위, 사용자 고정 폴더 및 세부 정보 내용은 실제 Windows 탐색기와 차이가 남는다. 파일 이름/원시 metadata 및 오류 의미를 시각적 유사성에 맞추어 바꾸지 않는다.

경로 클릭은 Qt가 mousePress 전에 focus를 주는 경계를 실제 UI에서 확인하여 수정했다. 마지막 breadcrumb paint의 hit map을 사용하고 keyboard focus 때는 편집을 유지한다. 주소의 접근성 값과 caller input은 전체 경로를 보존한다. 별도 Qt 회귀 시험은 상위 클릭→정확한 부모 항목→history→뒤로→Ctrl+L 전체 경로→잘못된 입력의 기존 목록 보존을 검증한다. 실제 통과 여부는 최신 P1 검증 원장을 따른다.

Qt DLL 오류는 사용자 앱 기능과 구분해 기록했다. 직접 CTest 실행에서 Qt bin PATH 누락으로 `domain_tests.exe`의 `Qt6Test.dll` 오류가 실제 발생했고, 정상 스크립트 실행은 통과했다. CMake Windows POST_BUILD에서 각 executable의 `TARGET_RUNTIME_DLLS`를 해당 출력 폴더로 복사하도록 바꿨다. `Qt6Test.dll`은 SDK와 같은 SHA-256 `be5ea9ff96d2b48f4246c1a402f678a397924ab9acbbfa7edfba590b6962f594`다. Qt bin이 없는 PATH에서도 [domain 시험](measurements/qt-dll-resolution.txt)이 8 pass/0 fail로 실행되었다. 테스트 plugin 경로는 명시적으로 선택했으며 최종 앱 패키지는 별도 검증한다.

실제 앱 내부 드래그는 파일/폴더 행과 content hash로 검증했다. OS 앱 간 native gesture는 아직 미검증이다. Computer Use의 drag endpoint를 대상 창 바깥으로 지정한 창 배치 시도는 `point (-450, 15) is outside window bounds`로 거부됐다. 이는 UI 도구의 창 경계이며 제품의 drop 실패가 아니다. synthesized external file-URL Qt events와 실제 앱 내부 드래그를 native cross-application drop 성공으로 확대하지 않는다.
