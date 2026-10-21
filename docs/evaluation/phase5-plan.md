# Phase 5 실행 범위와 측정 기준

2026-10-22, P0 검증 뒤 수행할 계획. 원본 소스는 Phase 6 freeze 전 금지한다. 기능을 추가한 뒤 전체 FR01–20·14시나리오를 다시 감사한다. 이 문서는 실행 결과가 아니다.

P1 필수 범위는 단일·복수 복사/이동과 명시적 충돌 처리, 복구 가능한 삭제, 파일/폴더 rename, mkdir, Ctrl/Shift 선택과 일괄 작업, 내부 및 OS와의 드래그 앤 드롭, 아이콘/목록/자세히 보기, 파일 관리 단축키, 개수/선택/작업 진행 상태다. 파일 수정은 소유한 disposable fixture에서만 검증하고 기준 fixture를 보존한다. 충돌·권한·삭제/교체 identity·혼합 결과·취소·최종 정리·정리 재시도를 별도로 확인한다. Windows와 POSIX 어댑터는 기능 지원과 명시적 실패를 같은 계약으로 반환한다.

이식성은 기존 Ubuntu/Qt6.4.2에서 실제 B 빌드·공통/어댑터/UI 테스트를 수행하고 Windows와 Qt 버전·파일시스템 차이를 표시한다. macOS는 코드 호환성 검토와 미실행 한계를 기록한다. Windows 독립 패키지에서 개발 PATH를 제외한 실행·의존성·notices·재실행을 검증한다. formatter와 실제 정적 분석 결과도 남긴다.

## 성능 사전 기준

고정 입력은 SHA256 `278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1`의 fixture manifest와 정확히 1,200개 파일의 read-only/bulk다. 동일 호스트에서 원본/A/B에 같은 폴더·정렬/보기 조건을 사용한다. 서로 다른 계측 방식은 같은 수치로 직접 우열 비교하지 않고 한계를 기록한다.

첫 앱 실행과 같은 프로세스의 5회 새로고침을 구분한다. OS cache를 비우지 않으며 cold OS cache라고 부르지 않는다. 초기 Windows 도구 smoke와 Unicode render는 bulk 성능 기준 결과가 아니다. 이후 B의 목표는 창 준비 후 완전 목록이 첫 탐색 2초 이내·warm refresh 1초 이내, 5ms UI heartbeat의 최대 간격 100ms 이하, 해당 앱 process-family peak committed memory 256MiB 이하다. 수치는 PoC 초기 목표이며 원본 수치나 보장값이 아니다. 결과를 보고 임계값을 소급 변경하지 않는다.

CPU는 전체 소유 process family의 user+kernel time, 메모리는 Windows Job Object PeakJobMemoryUsed로 기록한다. 이는 resident working set이 아니다. UI heartbeat는 초기 프로세스 시작/Qt 초기화와 별도이며 실제 계측 구간을 명시한다. 분석 엔진·에이전트·IDE 및 연결 프로그램은 앱 계측에서 제외하고 포함/제외를 기록한다. 원본을 동일 조건으로 계측하지 못한 지표는 미측정으로 남기며 B 수치를 원본 추정치와 비교하지 않는다.

정확 토큰/API 과금·모델 배포 ID는 현재 인터페이스로 접근할 수 없다. 전역 goal 사용량은 A/B별 입력·출력 과금이 아니며, 파일 크기를 토큰으로 대체하지 않는다. 실제 제공된 단계 wall time·CPU·메모리 원자료와 미계측 비용을 함께 보고한다.
