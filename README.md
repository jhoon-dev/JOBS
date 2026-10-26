# JOBS
전문 프로젝트 관리 도구 기획 · 2026-10-25 · v0.1 제안

## 목표와 현재 상태
프로젝트별 독립 JOBS 파일로 새로 만들기·열기·저장·다른 이름 저장과 Windows/macOS 이동 열기를 제공하며, 기본 작업은 계정·서버 없이 수행한다.
사용자 확정 목표는 Microsoft Project와 Oracle Primavera를 대체할 수준의 전문 PM 도구다. 양 제품의 전체 기능을 후보로 유지한다. 개발 단계는 제외 목록이 아니라 개발 순서다.
JOBS는 빈 폴더에서 기획을 시작했다. 2026-10-26 요청으로 C++20/Qt Widgets 앱과 파일 관리, 기본 CPM, WBS·표·간트를 구현했다. 기준선 버전 저장, 상태일별 실적 보고 이력, 잔여 예측 및 기준선 대비 변경 비교, 인력·장비 등록/배정·일별 과부하 및 별도 평준화 시나리오 A/B 비교를 추가했다. 유효일 단가·작업 비용·누적 실제원가와 기준선 EVM 분석, 프로젝트 대시보드·일정/자원/원가 보고서와 XLSX/PDF 내보내기도 구현했다. 호환성 및 제품 동등성은 아직 검증되지 않았다.
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

초기 개발 기준은 C++20, CMake 3.24+, Qt 6.8.3 Core/Widgets/Concurrent다. 서버·계정 없이 WBS·기간·FS 선행·휴일을 입력하고 F9로 일정 계산한다. 추적 메뉴에서 기준선을 저장하고 상태일·실적을 보고한 뒤 계획 대비 비교를 연다. 자원 메뉴에서 가용량·배정을 입력하고 부하와 평준화 시나리오를 비교한다. `.jobs` 단일 파일에 계획·자원·기준선·실적·시나리오 입력을 저장한다. 원가 메뉴에서 유효일 단가·고정비·누적 실제원가·물리 진척을 입력하고 기준선 EVM을 분석한다. 자동복구·고급 평준화·회계/환율 연계는 후속 구현이다.

[빌드·실행·파일 형식·검증 범위](docs/development.md)를 참고한다. GitHub 저장소는 [jhoon-dev/JOBS](https://github.com/jhoon-dev/JOBS)다.
[기준선·실적·비교 사용 예제](docs/baseline-progress.md)에서 첫 추적 흐름을 재현할 수 있다.

[자원·배정·부하·평준화 사용법과 계산 계약](docs/resources-leveling.md)에서 F05와 우선순위 대안을 재현할 수 있다.

[단가·작업 비용·실제원가·EVM 사용법](docs/costs-evm.md)에서 F10·EAC 가정과 집계 범위를 확인할 수 있다.

[대시보드·보고서·엑셀/PDF 내보내기](docs/reports-export.md)에서 필터·집계·출처와 출력 방법을 확인할 수 있다. 보고서 메뉴에서 열며 Excel·Python 설치 없이 출력한다.
