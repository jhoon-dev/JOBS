# Microsoft Project XML 교환 계약

2026-10-30 응답성 보완: 읽기/변환과 내보내기 준비, 교환 사본 저장·재열기·기준선 검증은 공통 worker에서 수행한다. 진행 창의 취소와 창 닫기는 취소 요청이며 worker의 임시 파일 정리가 끝난 후 창을 닫는다. 최종 새 폴더 rename 직전에 취소를 검사한다. rename이 이미 끝난 후에는 완성된 산출물을 보존하고 완료로 처리한다. 처리 중 문서 revision/session 변경은 오래된 요청을 취소하며, 가져오기 완료 후에도 문서 출처를 다시 확인해 최신 문서를 교체하지 않는다. 일반 저장은 같은 입력이면 허용한다. 명시 저장 확인, 변환 미리보기 및 새 사본 열기 정책은 유지한다.

`exchange_async_recovery_diagnostics`는 실제 XML/XER 메뉴의 worker 실행과 GUI 계산 checkpoint 0개, 1만 작업/기준선의 heartbeat, 게시 직전/완료 후 취소, 임시 폴더 정리·기존 목적지/원본 보존, 편집·같은 ID/revision 재열기·일반 저장 경합을 검사한다. 전체 파일 사본과 기준선 재검산을 생략하지 않는다.

2026-10-27 · J-15 / XML-01~04 · 첫 구현 범위. PRD 확정 후 개발한다.

공식 근거: Microsoft의 [Project XML schema](https://github.com/MicrosoftDocs/office-developer-msproject-xml-docs/blob/main/project-xml-data-interchange/xml-schema-for-the-project-element.md), [작업 schema](https://github.com/MicrosoftDocs/office-developer-msproject-xml-docs/blob/main/project-xml-data-interchange/xml-schema-for-the-tasks-element.md), [달력 schema](https://github.com/MicrosoftDocs/office-developer-msproject-xml-docs/blob/main/project-xml-data-interchange/xml-schema-for-the-calendars-element.md), [LinkLag](https://github.com/MicrosoftDocs/office-developer-msproject-xml-docs/blob/main/project-xml-data-interchange/linklag-element.md). LinkLag는 분의 1/10이며 Type은 0=FF, 1=FS, 2=SF, 3=SS다. 수입 대상 namespace는 `http://schemas.microsoft.com/project`, 내보내기는 SaveVersion 12의 MSPDI 지원 부분이다. 형식이 같아도 제품/빌드별 계산 동등성은 미검증이다.

## 지원과 경계

- XML 최대 16 MiB, 10,000 task/resource/calendar/assignment 요소, 깊이 32·총 300,000 요소. XML 요소 이름은 128자, 경로는 4,096자, 모든 저장 경로의 합은 UTF-16 코드 단위 16 Mi자로 제한하여 중첩 경로의 메모리 증폭을 차단한다. 상속을 펼친 모든 달력의 휴일 합은 100,000일로 제한한다. DTD/entity와 외부 namespace 요소는 차단한다. 알 수 없는 일반 필드는 경로별 진단하고 원본에 보존한다. XML을 실행하거나 외부 참조를 가져오지 않는다.
- UID를 식별자로 사용하고 ID는 표시 순서 정보다. Task UID 0은 프로젝트 요약, Summary=1은 WBS 그룹이며 OutlineLevel 계층을 사용한다. 요약 작업 선행/배정은 leaf로 임의 확장하지 않고 차단한다. OutlineNumber/WBS 사용자 코드는 원본에만 남는다. 내보내기는 WBS 순회 순서와 신규 정수 UID를 사용하며 JOBS stable ID와의 매핑을 보고한다.
- Duration은 PT의 시간/분/초 표기에서 정수 근무분만 허용한다. 누락은 미정, 0은 마일스톤이다. 경과시간/백분율 기간·지연과 소수 분 지연은 차단한다. 추정 표시형식은 근무분 값만 매핑하며 JOBS에 없는 추정 표시의 손실을 진단한다. 변환 보고서는 대상 계산 정책·후행 작업 달력 기준 지연·09:00 시작 기준을 명시한다. 제약은 0 ASAP, 1 ALAP, 2 MSO, 3 MFO, 4 SNET, 5 SNLT, 6 FNET, 7 FNLT를 매핑한다. civil minute 날짜만 허용한다. timezone/초 정밀도를 임의 절삭하지 않는다.
- 달력은 모든 근무 요일에 같은 일중 구간을 갖는 주간 달력과 비반복 전일 휴일만 허용한다. 상속을 펼치고 누락/순환을 차단한다. 요일별 상이한 시간·근무 예외·반복 예외·WorkWeeks는 차단한다. 프로젝트 달력은 모든 수입 작업/자원에 명시 배정한다. JOBS의 프로젝트 시작은 해당 날짜 09:00이므로 원본 시작 시각 차이와 향후 새 작업의 기본 달력 차이는 진단한다.
- work 자원(Type=1)의 이름/MaxUnits와 양수 정률 Assignment Units를 1% 정밀도로 매핑한다. 이름 없는 UID 0 자원은 미배정 placeholder로 보존 원본에만 남기며 이를 참조하는 배정은 차단한다. material/cost 자원·외부 프로젝트·수동 일정은 차단한다. 자원 달력은 보존하지만 첫 수입 작업은 기간 기반 고정 기간으로 변환한다. 작업 유형/공수·자원 달력에 따른 외부 일정 차이는 진단한다. 자원 단가/가용량 곡선/시간별 배정과 고정비·기준선/실적/EVM·사용자 필드는 매핑하지 않고 원본에 보존한다.
- 내보내기도 기간 기반 계획 투영이다. 공수 기반 계획은 명목 기간을 사용하고 작업 유형/공수/수동 분할의 손실을 작업별 진단한다. 계산된 날짜는 내보내지 않는다. 미정 값을 0으로 바꾸지 않으며 미정 기간/가용량 필드는 생략하고 외부 앱의 기본값 해석 가능성을 진단한다. 자원 종류 equipment는 MSPDI work 자원으로 투영하며 진단한다. 자원/작업 달력, 네 관계, 제약/마감일과 정률 배정은 유지한다. 비용/승인 이력은 source.jobs에만 보존한다.

## 파일과 UI

파일 메뉴의 XML 가져오기/내보내기는 건수·손실 진단·대상 새 폴더를 미리 보여 준다. 진단을 확인하고 승인한 뒤 같은 부모의 임시 폴더에서 모든 파일을 기록/재열기 검증하고 새 폴더로 rename한다. 기존 목적지는 거절한다. 오류/취소 때 현재 문서는 그대로다. 가져오기는 미저장 변경 처리 후 project.jobs를 새 세션으로 연다. 내보내기는 읽기 전용 문서에서도 가능하며 전체 프로젝트를 사용한다.

conversion.json은 contract `jobs-mspdi-plan-v1`, 방향, 원본 SHA-256/이름/SaveVersion/BuildNumber, 문서 ID/revision, UID 매핑과 경로/설명별 진단을 담는다. `import`, `originalPreservation`, `jobsRecomputation`, `externalRoundTrip` 네 판정은 독립이다. 수입 original.xml은 원본 바이트 exact이고 수출 source.jobs는 현재 입력/이력의 정확한 새 직렬화 스냅샷이다(기존 디스크 파일의 바이트 사본이라는 뜻이 아니다). 외부 재계산/왕복은 unverified다. .jobs schema 9는 그대로 유지한다. 동반 원본/진단 파일은 해당 교환 폴더에 보관되며 .jobs 파일만 별도 저장/이동하면 동반 이동하지 않는다.

## 완료 검사

도메인 골든 fixture와 XML 오류/한도/DTD/누락·중복 UID/고아·요약 참조/순환, 달력 상속·휴일/불가 패턴, 네 관계·음수 지연·정밀도·제약, 자원/배정, 미정 기간, 한국어/escape, 원본 바이트/hash 보존, 실패 시 기존 폴더 보호를 검사한다. 지원 부분의 내보내기→재수입은 ID 정규화 후 입력과 일정 의미를 검사하고 Python 표준 XML reader로 namespace/UID/참조/값을 독립 검사한다. 네이티브 앱에서 미리보기 취소·가져오기/내보내기·읽기 전용 내보내기와 기존 회귀를 실행한다. Microsoft Project 설치본이 없는 CI에서는 실제 제품 호환을 완료로 표시하지 않는다.

## 사용 순서와 검사 도구

1. 파일 → Project XML 가져오기에서 .xml을 선택한다. 작업/자원 수·매핑하지 않는 경로와 미검증 판정을 확인한다.
2. 아직 없는 새 교환 폴더의 전체 경로를 지정하고 `변환 확인 · 새 사본 열기`를 누른다. 기존 문서가 미저장이면 저장/버림/취소를 선택한다. 취소는 교환 폴더를 만들지 않는다.
3. 해당 폴더의 `project.jobs`가 새 문서로 열린다. F9로 JOBS 일정을 계산한다. `original.xml`/`conversion.json`은 원본과 판단 근거로 함께 보관한다.
4. 내보내기는 파일 → Project XML 내보내기에서 같은 미리보기 흐름을 사용한다. `project.xml`/`source.jobs`/`conversion.json`이 생성된다. 읽기 전용 문서와 미저장 입력도 내보낼 수 있으며 열린 원본의 편집 이력/저장 지점을 바꾸지 않는다.

`project_xml_contract`/`project_xml_ui`를 추가해 전체 CTest는 30개다. 고정 fixture는 실제 제품에서 저장한 파일로 위장하지 않은 수작업 골든 입력이다. `scripts/verify-project-xml.py`는 Python 표준 XML reader로 고정 기대값을 독립 검사한다. `scripts/capture-project-xml-ui.py`는 네이티브 대화상자의 취소/가져오기/내보내기/오래된 입력/읽기 전용 동작과 `project-xml-import-native.png`/`project-xml-export-native.png`를 남긴다. CI artifact `project-xml-Windows`/`project-xml-macOS`에서 XML/진단/화면을 확인한다. 공식 근거 조회 시 MicrosoftDocs 소스 revision은 `c77602db4d0733ce57cb0e5514c6fc898c7fe2c2`였다. 내보내기는 schema의 프로젝트 이름 255자, 작업/WBS/자원/달력 이름 512자, 일중 구간 5개 한도도 검사한다.
