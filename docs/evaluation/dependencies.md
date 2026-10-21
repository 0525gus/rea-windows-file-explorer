# 외부 구성요소와 라이선스

현재 Windows 빌드의 직접 의존성은 C++20 표준 라이브러리와 Qt6 Core/Gui/Widgets/Concurrent다. QtTest는 테스트용이다. Qt6.8.3 공식 SDK는 프로젝트 내부 `tools/qt`에 설치했으며 동적 DLL로 링크한다. Qt LGPLv3/GPLv3/상용 선택권과 개별 모듈·제3자 라이선스는 배포물의 실제 notices를 기준으로 확인해야 한다. 최종 배포에는 사용된 notices·교체 가능한 Qt DLL 및 요구되는 안내를 포함하며, 이 문서만으로 배포 조건 충족을 선언하지 않는다.

MSVC/Windows SDK는 기존 호스트 도구다. Windows API/COM은 Windows 플랫폼 어댑터에서만 호출한다. Linux/macOS는 표준 파일시스템과 Qt Core를 사용한다. 파일 기본 앱 위임은 Windows ShellExecuteExW, Linux xdg-open, macOS open helper다. POSIX helper 실행 성공은 연결 앱의 파일 로딩 완료와 구분한다.

Explorer++ 공식 배포 EXE와 License.txt는 분석 입력이다. 원본 코드나 리소스·아이콘을 제품에 포함하지 않았다. 자체 QPainter 아이콘과 시스템 폰트를 사용하며 스크린샷은 관찰 문서용이다.

Python 표준 라이브러리는 fixture·PE 보조 조사·빌드 소유권/측정 스크립트에만 사용한다. REA/Ghidra/JDK는 분석 도구로 제품 런타임에 포함하지 않는다. 실제 버전과 획득 해시는 environment-audit/qt-setup/target-manifest를 따른다. 최종 패키지 파일별 의존성·notices 검증은 Phase 5 배포 검증에서 완료한다.
