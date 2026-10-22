# 소스 접근 통제

현재 단계: Phase 1. Explorer++ 원본 소스는 다운로드하거나 열지 않았다.

허용 입력은 공식 release API의 버전/asset 메타데이터, 배포 ZIP 중 Explorer++.exe 및 License.txt, 실행 관찰, REA/Ghidra 결과, 공식 OS/Qt 문서다. GitHub repository tree, 원본 코드 검색, 소스 archive/clone은 Phase 6 freeze gate 이전 금지한다. 배포 Help/History/Readme도 현재 분석 입력에 사용하지 않았다.

- 역공학/재구현: 프로젝트 루트, docs/analysis/, src/, experiments/B/.
- 대조 실험 A: experiments/A/. 원본과 동일 요구/fixture를 제공하되 REA 결과를 제공하지 않는 독립 세션이 필요하다. 현재 루트 세션은 REA를 이미 보았으므로 A로 재사용할 수 없다.
- 사후 검증: verification/source/. 현재 비어 있고 원본 소스가 없다. 초기 구현/분석 확정 시 Git commit과 SHA-256 manifest를 먼저 작성한다.
- 접근 권한 분리: 사용자 승인 후 `verification/source`만 현재 사용자 ReadData/ListDirectory를 거부했다. 원래 ACL 백업과 실제 파일 읽기·목록 접근 거부는 [source-gate-status.json](source-gate-status.json)에 남겼다. 이 디렉터리에는 소스가 없으며 Phase 6 freeze 확인 전 복원하지 않는다. 소유자가 ACL을 복원할 수 있으므로 적대적 사용자 격리가 아니다. A/B는 별도 fresh-context와 허용 입력 지침으로 분리하며, 같은 사용자이므로 A/B 간 강제 ACL 격리·도구별 차단은 확보하지 못했다.

도구가 반환하는 소스 경로 문자열/PDB 참조는 바이너리 메타데이터이며 원본 파일 접근 권한으로 해석하지 않는다. 디컴파일 결과는 원본 소스와 구별한다.
