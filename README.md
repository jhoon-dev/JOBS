# JOBS
전문 프로젝트 관리 데스크톱 앱 · C++20 / Qt 6.8 · 개발 중

## 목표와 현재 상태
프로젝트별 독립 JOBS 파일로 새로 만들기·열기·저장·다른 이름 저장과 Windows/macOS 이동 열기를 제공하며, 기본 작업은 계정·서버 없이 수행한다.
사용자 확정 목표는 Microsoft Project와 Oracle Primavera를 대체할 수준의 전문 PM 도구다. 양 제품의 전체 기능을 후보로 유지한다. 개발 단계는 제외 목록이 아니라 개발 순서다.
JOBS는 빈 폴더에서 기획을 시작했다. 2026-10-26 요청으로 C++20/Qt Widgets 앱과 파일 관리, 기본 CPM, WBS·표·간트를 구현했다. 기준선 버전 저장, 상태일별 실적 보고 이력, 잔여 예측 및 기준선 대비 변경 비교, 인력·장비 등록/배정·일별 과부하 및 별도 평준화 시나리오 A/B 비교를 추가했다. 유효일 단가·작업 비용·누적 실제원가와 기준선 EVM 분석, 프로젝트 대시보드·일정/자원/원가 보고서와 XLSX/PDF 내보내기도 구현했다. 호환성 및 제품 동등성은 아직 검증되지 않았다.
별도 체크포인트 자동 저장, 비정상 종료 후 세대 선택/사본 복구, 일반 입력의 실행 취소·다시 실행을 추가했다. 승인·제출 이력은 보존하며 복구본이 명시 저장을 대신하지 않는다.
파일명의 대소문자를 보존하고 같은 파일 판정을 별도로 수행한다. 실행 취소는 변경분을 보존하며 일반 이름 변경의 복사 비용을 줄였다. [편집 성능 계약·측정](docs/editing-performance.md)에 1천/1만 작업의 셀 편집·undo/redo 측정과 범위를 기록한다.
작업 표는 QAbstractTableModel/QTableView를 사용하며 공수 미리보기·역산, 실적 입력과 자원 배정의 계산은 비동기로 수행한다. 툴바 또는 대화상자의 계산 취소를 사용하고, 취소한 대화상자의 초안을 수정해 다시 적용할 수 있다. [모델 전환·비동기 편집 계약과 검증](docs/model-async-editing.md)을 따른다.
대규모 편집에서는 worker 요청의 입력 소유권을 공유하고 dirty와 모델/WBS를 변경 작업 기준으로 갱신한다. [준비·반영 단계 측정과 남은 비용](docs/responsive-editing.md)을 기록하며 복합 변경·최초 계산 실효화의 전체 갱신은 보존한다.

FS·SS·FF·SF 선행관계와 지연/선행 시간, 작업별 근무 달력을 지원한다. 표에서 `1SS+2h,3FF-30m`처럼 입력하고 프로젝트 메뉴에서 달력을 등록·배정한다. [계산·입력·파일 계약](docs/pdm-calendars.md)을 따른다.

프로젝트 메뉴에서 8종 작업 제약·마감일과 일정 충돌을 관리한다. 간트 막대 이동/오른쪽 끝 resize, 자동 재계산과 실행 취소를 지원한다. [제약·마감·간트 편집 사용법](docs/constraints-gantt.md)을 따른다.
공수 기반 일정, 고정 기간·고정 공수·고정 단위, 자원별 달력 교집합과 수동 분할 중단을 지원한다. 프로젝트 메뉴의 공수·유형·분할과 자원 등록의 달력에서 설정한다. [공수·자원 달력·분할 계약](docs/effort-calendars-splits.md)을 따른다.
PRD PERF-01~04에 따라 일정 계산의 단계/진행 표시·취소와 화면 주변 행만 그리는 간트를 추가했다. [통합 검증·성능·계산 취소](docs/performance-integration.md)에 1천/1만 작업 측정·검증 범위와 재현 방법을 기록한다. 앞으로도 PRD에 없는 요구사항은 먼저 반영한 뒤 구현한다.
편집 메뉴에서 여러 작업의 삽입·삭제·복제와 원자적 TSV 붙여넣기를 지원한다. 표 위에서 이름/ID 검색·필터·정렬을 조합하며 선행 번호는 항상 프로젝트 원본 행 번호다. [작업 편집 사용법·보호 규칙](docs/task-editing.md)을 따른다.
PM Canvas는 별도 개인용 프로토타입이다. 그 코드·아키텍처·결정을 JOBS에 가져오지 않는다.

## 문서 읽는 순서
중심 문서는 제품 PRD다. 기능 추적표·계산 규칙·데이터모델·화면·로드맵은 PRD의 요구사항과 완료조건을 구체화하는 보조 문서다. 서로 충돌하면 임의로 구현하지 않고 PRD와 결정기록에 차이를 반영한다.

1. [제품기획과 PRD](docs/planning/01-product-prd.md)
2. [기능범위와 추적표](docs/planning/02-scope-traceability.md)
3. [개념 데이터모델](docs/planning/03-domain-model.md)
4. [핵심 계산 규칙 초안](docs/planning/04-calculation-contract.md)
5. [화면과 사용자 흐름](docs/planning/05-experience.md)
6. [단계별 로드맵](docs/planning/06-roadmap.md)
7. [결정기록과 질문](docs/planning/07-decisions.md)
8. [자료와 검증 기록](docs/planning/08-evidence-validation.md)
9. [원본 337행과 추적 데이터](docs/planning/09-source-traceability.json)
10. [세부 요구사항 337개 초안](docs/planning/10-detailed-requirements.md)
11. [원본 확인과제 39개](docs/planning/11-open-source-questions.md)
12. [기능군별 상세 계약](docs/planning/12-capability-contracts.md)
13. [데스크톱 기술 대안과 결정 게이트](docs/planning/13-desktop-options.md)
14. [C++ + Qt 기본 아키텍처와 계약](docs/planning/14-cpp-qt-architecture.md)
15. [프로젝트 파일과 전달·복구 계약](docs/planning/15-project-file-contract.md)

## 문서 운영
사용자 확정 사항은 CONFIRMED, 설계 제안은 PROPOSED, 미확인 근거는 UNVERIFIED로 구분한다. 이 문서 묶음의 상세 설계는 전부 PROPOSED이며 구현 승인이 아니다.
기능 상태는 후보→요구사항 확정→설계→구현→검증→호환성 검증으로 관리한다. 문서 존재만으로 기능 완료를 표시하지 않는다.
플랫폼은 Windows/macOS 설치형 데스크톱으로 사용자 확정됐다. 최종 목표는 기존 상용 PM 도구를 능가하는 도구이며, Parity와 Superiority를 별도로 검증한다. 사용자 결정 위임에 따라 C++ + Qt, UI 독립 계산 코어, 로컬 저장·자동 복구를 기본 방향으로 선택했다. Qt 세부 모듈/버전·DB·라이선스·사업모델은 미결정이다.

## 개발 시작

초기 개발 기준은 C++20, CMake 3.24+, Qt 6.8.3 Core/Widgets/Concurrent다. 서버·계정 없이 WBS·기간·FS 선행·휴일을 입력하고 F9로 일정 계산한다. 추적 메뉴에서 기준선을 저장하고 상태일·실적을 보고한 뒤 계획 대비 비교를 연다. 자원 메뉴에서 가용량·배정을 입력하고 부하와 평준화 시나리오를 비교한다. `.jobs` 단일 파일에 계획·자원·기준선·실적·시나리오 입력을 저장한다. 원가 메뉴에서 유효일 단가·고정비·누적 실제원가·물리 진척을 입력하고 기준선 EVM을 분석한다. 보고서 메뉴에서 대시보드/XLSX/PDF, 자동 저장·복구 메뉴에서 체크포인트/사본 복구, 편집 메뉴에서 실행 취소/다시 실행을 사용한다. 고급 평준화·회계/환율 연계는 후속 구현이다.

[빌드·실행·파일 형식·검증 범위](docs/development.md)를 참고한다. GitHub 저장소는 [jhoon-dev/JOBS](https://github.com/jhoon-dev/JOBS)다.
[기준선·실적·비교 사용 예제](docs/baseline-progress.md)에서 첫 추적 흐름을 재현할 수 있다.

[자원·배정·부하·평준화 사용법과 계산 계약](docs/resources-leveling.md)에서 F05와 우선순위 대안을 재현할 수 있다.

[단가·작업 비용·실제원가·EVM 사용법](docs/costs-evm.md)에서 F10·EAC 가정과 집계 범위를 확인할 수 있다.

[대시보드·보고서·엑셀/PDF 내보내기](docs/reports-export.md)에서 필터·집계·출처와 출력 방법을 확인할 수 있다. 보고서 메뉴에서 열며 Excel·Python 설치 없이 출력한다.

[자동 저장·복구·실행 취소 사용법](docs/recovery-undo.md)에서 저장 간격·복구본 선택·사본 저장과 편집 이력의 범위를 확인할 수 있다.

[Project XML 가져오기·내보내기](docs/project-xml.md)는 파일 메뉴에서 실행한다. 작업/WBS·관계·달력·자원/배정의 기간 기반 계획을 변환하고 새 교환 폴더에 원본/현재 JOBS 사본과 손실 진단을 함께 보관한다. 실제 Microsoft Project 재계산/재저장 호환과 MPP는 별도 후속 범위다.

[Primavera XER 가져오기·내보내기](docs/primavera-xer.md)도 파일 메뉴에서 실행한다. 단일 프로젝트의 기간 기반 작업/WBS·네 관계/지연·지원 제약·달력/휴일·자원/정률 배정을 변환한다. UTF-8/명시 선택 Windows-1252를 지원하며 새 교환 폴더에 원본 또는 현재 JOBS 사본과 진단을 보존한다. 실제 P6 계산/재저장 동등성·다중 프로젝트·실적/원가/공수의 완전 교환은 후속 범위다.

[구조·대량 편집 응답성](docs/bulk-editing.md): 작업 추가·삽입·삭제·복제·표 붙여넣기와 해당 undo/redo를 비동기로 준비하고 도구 모음에서 취소할 수 있다. 기본 보기의 연속 구조 변경은 행 단위로 반영하며 정렬/필터 등은 안전한 전체 갱신을 사용한다.
