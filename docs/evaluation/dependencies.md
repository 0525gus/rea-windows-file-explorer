# 외부 구성요소와 라이선스

현재 Windows 빌드의 직접 의존성은 C++20 표준 라이브러리와 Qt6 Core/Gui/Widgets/Concurrent다. QtTest는 테스트용이다. Qt6.8.3 공식 SDK는 프로젝트 내부 `tools/qt`에 설치했으며 동적 DLL로 링크한다. Qt LGPLv3/GPLv3/상용 선택권과 개별 모듈·제3자 라이선스는 배포물의 실제 notices를 기준으로 확인해야 한다. 최종 배포에는 사용된 notices·교체 가능한 Qt DLL 및 요구되는 안내를 포함하며, 이 문서만으로 배포 조건 충족을 선언하지 않는다.

Qt6.8.3의 라이선스 텍스트 37개(230,133 bytes)를 공식 Qt 저장소의 **LICENSES 디렉터리만** 받아 [해시/출처 manifest](qt-licenses/manifest.json)에 보존했다. Qt 구현 코드와 Explorer++ 소스는 다운로드·열람하지 않았다. 이 묶음에는 Qtbase의 가능한 라이선스 텍스트가 폭넓게 포함되므로 모든 항목이 실제 런타임에 쓰였다는 뜻은 아니다. Qt는 모듈별 라이선스와 별도 제3자 저작권·조건을 설명하며, 정확한 사용 구성요소와 attribution은 [공식 Qt 6.8 라이선스 문서](https://doc.qt.io/qt-6.8/licensing.html)와 [제3자 목록](https://doc.qt.io/qt-6.8/licenses-used-in-qt.html)을 함께 확인한다. 미수정 Qt6.8.3 대응 소스는 [공식 qtbase archive](https://download.qt.io/official_releases/qt/6.8/6.8.3/submodules/)에서 제공된다. 패키지에는 동적 DLL과 해당 앱의 재빌드 가능한 소스를 포함한다.

MSVC/Windows SDK는 기존 호스트 도구다. 패키징 스크립트는 설치된 도구의 redistributable CRT DLL을 앱 폴더에 복사하며 installer를 실행하지 않는다. Windows API/COM은 Windows 플랫폼 어댑터에서만 호출한다. Linux/macOS는 표준 파일시스템과 Qt Core를 사용한다. 파일 기본 앱 위임은 Windows ShellExecuteExW, Linux xdg-open, macOS open helper다. POSIX helper 실행 성공은 연결 앱의 파일 로딩 완료와 구분한다.

Explorer++ 공식 배포 EXE와 License.txt는 분석 입력이다. 원본 코드나 리소스·아이콘을 제품에 포함하지 않았다. 자체 QPainter 아이콘과 시스템 폰트를 사용하며 스크린샷은 관찰 문서용이다.

Python 표준 라이브러리는 fixture·PE 보조 조사·빌드 소유권/측정 스크립트에만 사용한다. REA/Ghidra/JDK는 분석 도구로 제품 런타임에 포함하지 않는다. 실제 버전과 획득 해시는 environment-audit/qt-setup/target-manifest를 따른다. 최종 패키지 파일별 의존성·notices 검증은 Phase 5 배포 검증에서 완료한다.

정확한 Qt **6.8.3** attribution은 [고정 commit 묶음](qt-attributions/README.md)에 추가했다. qt/qtbase commit `c07c2d5a527a644d36e7853d55132ae38921682f`의 attribution JSON 48개·58개 entry와 직접 참조된 notice 49개, 총 97개 원본 파일을 hash 검증했다. Qt 6.8 온라인 문서는 현재 patch가 달라질 수 있어 버전 정본으로 사용하지 않는다. 이 묶음은 전체 qtbase의 superset이며 모든 optional component가 실제 DLL에 들어갔다거나 모든 전이 license 의무를 검토했다는 의미가 아니다. 패키지는 이를 generic LICENSES와 함께 보존한다. windeployqt가 포함한 Qt6Network 및 D3Dcompiler도 실제 파일 manifest에서 제외하지 않는다.
