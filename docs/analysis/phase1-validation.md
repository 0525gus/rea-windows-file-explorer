# Phase 1 — 실제 PE 분석 검증

판정: **기술 게이트 통과**. 전체 프로젝트 완료나 Explorer++ 기능 복원 성공 판정이 아니다.

## 실행 근거

- REA Skill v42를 읽고 실제 MCP 도구 계약을 조회했다. `binary_session`은 rea-agents 6.7.0, Ghidra 12.1.4, JDK 21.0.12.1(64-bit full JDK), 207 MCP 도구를 보고했다. [정본](evidence/phase1-session.json).
- 기존 Windows 시스템 파일을 별도 분석 입력으로 복사했다. `artifacts/smoke/whoami.exe`, 98,304 bytes, PE x86_64, SHA-256 `23240ef9f8b0a9a324110b1c2331de31dc1b0e08f5359cb707e51a939af56cd3`. 대상 프로그램은 실행하지 않았다.
- `open_binary(path, provider_id: ghidra)` → `binary_overview({})` → `list_procedures({})` → `analyze_function(procedure: 0x140001cb0)`를 실제 호출했다.
- overview Evidence `ev_7022afc10a784047e42b6da4b4baed8391033ca4994c9598cf6b6a88305952da`: 함수 341개, 정의된 문자열 308개, 메모리 블록 9개. 미발견 함수/미정의 문자열까지 완전함을 뜻하지 않는다.
- 함수 Evidence `ev_4d7010cb610e29c55af0471266e20547686c9f7d0f83d2b1cd7bdf5667825a8d`: entry `0x140001cb0`, 실제 의사코드 81문자, 직접 호출 대상 `0x1400021b4`, `0x140001af0`. callers는 빈 배열이며 진입 함수에는 정상적일 수 있다. 모든 호출의 의미를 규명한 결과가 아니다.
- `export_evidence_bundle`: [4-record bundle](evidence/phase1-smoke-bundle.json), 129,899 bytes. 파일을 다시 JSON으로 파싱하고 정확한 함수 ID, 대상 digest, 비어 있지 않은 의사코드 및 두 callee를 검증했다. 복사된 PE의 SHA-256이 분석 전후 동일했다.
- `close_binary(snapshot_path)`: [snapshot](evidence/phase1-smoke-snapshot.json), 169,973 bytes, primitive bindings 2, workflow bindings 1, retained records 4. Ghidra 임시 프로젝트를 종료했다.

저장된 의사코드는 `entry → FUN_1400021b4; FUN_140001af0` 흐름이다. 원본 소스가 아니며, Ghidra의 terminal-call/fallthrough 제한과 PDB load unknown이 Evidence에 그대로 보존돼 있다.

## 다음 단계 입력

[대상 manifest](target-manifest.json): 공식 Explorer++ 1.4.0 x64 ZIP에서 EXE와 License만 확보했다. EXE FileVersion 1.4.0.2293, SHA-256 `22ac20341c5525658ebe5e9c4b9ac9f7c152b18080ff347861fac3f04d56a2d6`. 배포 메타데이터에는 publisher digest가 없어서 로컬 hash만으로 배포자 서명을 입증하지 않는다. 원본 소스·PDB·Help·History·Readme는 분석 입력으로 열지 않았다.

[환경 감사](../evaluation/environment-audit.md): Windows MSVC/CMake/Ninja 기존 설치 확인. Windows Qt 미발견(검색 범위 한정), 프로젝트 내부 Qt SDK 추가 승인 대기. WSL Ubuntu에는 Qt6 6.4.2가 존재한다. Git 저장소를 초기화했다.

후속 진행: GUI 실행·프로젝트 내부 Qt 설치·검증 폴더 권한 분리는 사용자 승인 후 수행했다. Qt 환경 smoke 및 제품 Windows 첫 빌드가 실제 성공했다. 제품 테스트는 별도 [P0 검증표](../evaluation/p0-validation.md)에서 추적한다. A는 fresh-context 독립 세션으로 시작했으나 정확한 모델 배포 식별자·과금과 A/B 간 강제 권한 격리는 검증하지 못했다. Phase 1 통과와 전체 제품 완성을 구분한다.

## 측정 한계

이 초기 환경/분석 호출들의 총 작업 시간, CPU 시간, process-family peak memory, 정확 입력·출력 토큰/API 과금은 계측하지 못했다. 도구 호출 로그가 제공하는 단일 wall time을 전체 개발시간으로 대체하지 않는다. 모델 내부 배포 식별자는 이 세션에서 검증하지 못했다. 향후 사전 고정한 측정 프로토콜로 cold/warm을 분리하며 미측정 값은 0으로 쓰지 않는다.

