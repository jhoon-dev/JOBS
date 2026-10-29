# Microsoft Store 한국어 소개 초안

2026-10-29. 앱 이름 예약·계정·가격·등급·국가·최종 문구는 제출자가 Partner Center에서 확정한다. 현재 앱은 초기 공개 준비 버전이다.

**제품명:** JOBS (이름 예약 전)

**짧은 설명:** 프로젝트 일정과 간트 차트, 자원 배정, 원가와 실적을 한곳에서 관리하세요.

**설명:**

JOBS는 프로젝트 계획과 진행 상황을 관리하는 Windows 데스크톱 앱입니다. 작업과 선행관계를 작성하고 일정을 계산해 간트 차트에서 확인할 수 있습니다. 작업별 달력과 제약조건, 자원 배정과 과부하, 기준선과 실적, 원가와 획득가치 분석을 함께 다룹니다.

프로젝트별 창과 작업 영역을 사용해 여러 프로젝트를 열고 공유 자원 부하를 비교할 수 있습니다. 일정·자원·원가 보고서를 Excel과 PDF로 내보내고, Primavera XER 및 Microsoft Project XML 파일을 교환할 수 있습니다.

프로젝트는 사용자가 선택한 위치에 로컬 파일로 저장합니다. 자동 복구와 실행 취소·다시 실행을 지원합니다. 계정 가입이나 앱 자체의 클라우드 동기화는 필요하지 않습니다.

초기 버전이므로 조직에서 사용하는 실제 파일의 사본으로 가져오기·계산·내보내기 결과를 확인한 후 활용해 주세요. 외부 제품의 모든 필드를 보존하는 형식 변환을 보장하지 않으며, 변환 과정의 경고와 지원 범위를 앱 및 문서에서 확인할 수 있습니다.

**주요 기능:**

- 작업·WBS·FS/SS/FF/SF 관계·지연/선행 시간·작업 달력
- 제약조건·마감일·일정 충돌 진단·간트 편집
- 공수 기반 일정·작업 유형·분할 작업·자원 달력
- 기준선·실적·변경 비교·자원 평준화 시나리오
- 계획/실제 원가·EVM·대시보드·Excel/PDF 보고서
- 여러 프로젝트 창·작업 영역 저장·공유 자원 부하
- 로컬 저장·자동 복구·실행 취소·다시 실행

**분류 제안:** 생산성. 선택은 제출자가 확정한다.

**지원 언어:** 한국어. **장치:** Windows PC x64. **최소 OS:** Windows 10 2004, 빌드 19041. ARM64/x86 전용 패키지는 제공하지 않는다.

**제출 화면 설명:** `01-schedule.png`는 샘플 프로젝트의 계산된 작업 표와 간트 차트다. 실제 패키지의 Qt UI를 1440×900으로 캡처하며 샘플 데이터에는 개인 정보가 없다. 공유 자원 대화상자 화면은 별도 검증 자료로 보관한다. 데스크톱 최소 크기에 못 미치는 보조 화면은 제출 묶음에 넣거나 인위적으로 확대하지 않는다.

**지원 URL:** https://github.com/jhoon-dev/JOBS/issues

**개인정보 처리방침 URL:** https://github.com/jhoon-dev/JOBS/blob/main/docs/store/privacy-policy.ko.md

**심사 메모 / runFullTrust 요청 사유 (영문):**

JOBS is a C++/Qt Widgets desktop project scheduling application. The runFullTrust capability is required for its existing Win32 desktop process, user-selected local project files, file locks, recovery checkpoints, and Excel/PDF/XER/XML import and export. It runs at medium integrity, does not request elevation, and installs no services, drivers, startup tasks, or background agents. It uses no account, advertisements, telemetry, or application-operated cloud service. Settings and recovery files are stored in the package LocalState. No test probe is included in the Store payload. Please launch JOBS from Start, create a project, calculate its schedule, save and reopen the project; no sign-in or purchase is needed to test the current build.

상표 표기는 외부 형식 호환성을 설명하기 위한 것이다. Microsoft·Oracle과의 제휴나 해당 제품과 동등한 전체 호환성을 주장하지 않는다.
