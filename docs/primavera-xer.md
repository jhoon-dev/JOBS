# Primavera XER 교환 계약

2026-10-27 · J-15 / XER-01~04. 이 계약을 개발 전에 확정한다. .jobs schema 9와 기존 계산 의미는 유지한다.

근거: Oracle [TASK](https://docs.oracle.com/cd/G48897_01/English/Mapping_and_Schema/xer_import_export_data_map_project/97906.htm), [CALENDAR](https://docs.oracle.com/cd/F88966_01/English/Mapping_and_Schema/xer_import_export_data_map_project/97869.htm), [TASKPRED](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/p6-xer-proj/taskpred-activityrelationships-97914a.html), [PROJECT](https://docs.oracle.com/en/industries/construction-engineering/primavera-p6-project/26/p6-xer-proj/project-projects-97885a.html)의 공식 필드 매핑을 따른다. 세부 직렬화와 독립 reader는 [MPXJ](https://www.mpxj.org/howto-read-xer/)와 구현 소스 `71d78a9b64f5de3bbeb7b667a480ab0888d011e8`의 StructuredText/Primavera reader/writer를 참고한다. Oracle이 공개한 필드 표를 모든 버전의 완전 문법 명세로 간주하지 않는다. 연구용 원본/외부 코드를 앱에 포함하지 않는다.

## 지원 범위

- 단일 PROJECT가 필수다. PROJWBS의 proj_node_flag=Y 루트 하나는 프로젝트 이름/최상위에 매핑하며 하위 WBS와 leaf TASK를 ID로 연결한다. TASK task_code/PROJECT proj_short_name은 native 이름과 구분하여 변환 보고서에 기록한다. 새 수입은 새 documentId와 `xer-task-<task_id>` 등 구분된 ID를 사용한다.
- TASK의 target_drtn_hr_cnt를 기간 기반 고정 기간으로 투영한다. 공백은 미정, 0은 마일스톤이다. TT_Task/TT_Rsrc/TT_Mile/TT_FinMile을 지원하며 자원 종속·시작/완료 마일스톤 차이와 duration_type 손실을 진단한다. TT_LOE/TT_WBS·미지 유형은 차단한다. target/early/actual 날짜를 고정 실적이나 하드 제약으로 추론하지 않는다.
- PR_FS/PR_SS/PR_FF/PR_SF와 lag_hr_cnt를 지원한다. JOBS는 후행 작업 달력 기준이다. SCHEDOPTIONS의 rcal_Successor 외 정책/누락을 진단하고 원본 설정을 보고한다. 여러 SCHEDOPTIONS/다른 프로젝트 ID는 차단한다. 외부 프로젝트 관계는 임의 제거하지 않는다.
- primary constraint는 공백 ASAP, CS_ALAP, CS_MSOA/MSOB/MEOA/MEOB를 SNET/SNLT/FNET/FNLT, CS_MANDSTART/MANDFIN을 MSO/MFO로 매핑한다. CS_MSO/MEO도 고정 날짜로 투영하되 P6의 Start/Finish On과 native 하드 고정 의미 차이를 진단한다. 보조 제약은 차단한다. native 소프트 마감은 XER expect_end_date로 바꾸지 않고 손실 진단한다.
- CALENDAR clndr_data의 `(0||CalendarData()(…))`에서 DaysOfWeek의 일=1~토=7을 native 월=1~일=7로 매핑한다. 각 근무 요일의 일중 구간이 같아야 한다. 전일 휴일은 Exceptions d의 1899-12-30 기준 일수로 매핑한다. 부모 달력에서는 휴일만 상속하며 주간 시간은 각각 명시한다. 상속 순환/부모 누락, 근무 예외·서로 다른 요일 시간·누락된 주간 패턴·알 수 없는 일정 구조는 차단한다. UI 표시용 VIEW 정보는 진단 후 원본에만 보존한다. 프로젝트 달력은 수입 작업/자원에 명시 배정하고 새 native 작업은 기존 JOBS 기본 달력을 쓰는 차이를 표시한다.
- RT_Labor/RT_Equip 자원과 TASKRSRC target_qty_per_hr를 1% 정밀도로 매핑한다. material 자원·곡선 배정은 차단한다. RSRCRATE는 한 자원에 시작일이 프로젝트 시작 이전/같은 단일 행일 때 max_qty_per_hr만 기본 가용량으로 매핑한다. 없거나 다중/미래 가용량이면 미정과 진단으로 남긴다. 단가/원가·원장·기준선·역할/코드/사용자 필드·분할/공수 규칙은 XER 계산으로 이식하지 않는다.

## 직렬화·한도

- UTF-8(선택 시 strict decode/BOM 허용)과 명시 선택 Windows-1252만 지원한다. 인코딩을 자동 추측하지 않는다. CP949/기타 파일은 원본을 보관하고 사본을 지원 인코딩으로 변환해야 한다. 내보내기는 한국어가 유지되는 UTF-8 BOM/CRLF이며 ERMHDR 버전 20.12의 지원 부분을 기록한다. P6 설치본이 이 파일을 수용한다는 뜻이 아니다.
- 탭 구분이며 마지막 빈 셀도 유지한다. %F는 테이블마다 한 번, 열 이름 중복/행 폭 불일치/%E 누락·후행 데이터/알 수 없는 레코드 마커는 차단한다. 필드 안의 탭/줄바꿈/NUL·제어문자는 수출 때 임의 escape/절삭하지 않고 거절한다. 날짜는 timezone 없는 YYYY-MM-DD HH:mm이다. 프로젝트 시작은 native 날짜의 09:00이므로 원본 시각 차이는 진단한다.
- 숫자는 locale 없는 최대 소수 6자리 십진수다. 시간×60을 정수 분으로 만들 때 6자리 시간 표현의 최대 반올림 오차(0.000030분) 이내만 허용하고 0이 아닌 보정은 진단한다. 수출은 소수 시간 6자리로 기록한다. 그 밖의 분수 분은 차단한다. 배정/가용량은 정확한 1% 단위여야 한다. 지원 통화는 기존 native KRW/JPY/USD/EUR이며 환산하지 않는다.
- 파일 16 MiB·행 300,000개·테이블 128개·열 256개·셀 1 Mi자, 주요 엔터티/관계/배정 각 10,000개다. clndr_data는 깊이 32·총 300,000 노드, 달력 1개 휴일 10,000일·상속 후 총 휴일 100,000일로 제한한다. Calendar parser는 전체 입력 한도와 깊이/노드 한도로 메모리 증폭을 제한한다. 원본 데이터는 코드/쿼리로 실행하지 않는다.

## 저장·화면·판정

파일 → Primavera XER 가져오기에서 파일/인코딩을 고르고 변환 미리보기의 건수/진단과 새 폴더 경로를 확인한다. 승인 후 project.jobs/original.xer/conversion.json을 같은 부모의 임시 폴더에서 모두 기록·재열기 검증하고 새 폴더로 rename한다. 내보내기는 전체 프로젝트를 project.xer/source.jobs/conversion.json으로 같은 방식으로 게시한다. schema 9 입력/승인·실적/원가는 source.jobs에 정확한 현재 스냅샷으로 보존하며 기존 디스크 파일 바이트 사본이라는 뜻은 아니다. .jobs만 따로 저장/이동하면 동반 파일은 자동 복제되지 않는다. 기존 대상은 덮어쓰지 않으며 취소/오류/오래된 미리보기는 현재 문서를 교체하지 않는다.

conversion.json contract는 `jobs-xer-plan-v1`이다. 방향·원본 헤더/버전/선택 인코딩·SHA-256·source/target ID/Activity Code·native profile/09:00 시작/후행 달력 지연 정책·경로별 손실을 기록한다. import/originalPreservation/jobsRecomputation/externalRoundTrip는 독립 판정이며 외부 계산/왕복은 unverified다. 독립 MPXJ 읽힘도 P6 재계산/재저장 동등성으로 확대하지 않는다.

## 검증

수작업 골든 fixture의 주간 달력/상속 휴일, FS/SS/FF/SF/음수 지연, 제약/자원/배정과 독립 날짜 기대값을 확인한다. 지원 부분 JOBS→XER→JOBS의 ID 정규화/일정 의미, 1분/미정/0/정밀도·한국어/Windows-1252, 헤더/원본 바이트/해시, 승인·실적·원가 사본과 실패/기존 폴더 보호를 검사한다. UI는 취소·미저장 경고·스냅샷/session 경계·읽기 전용과 native 화면을 검사한다. Python 표준 TSV reader 및 고정 MPXJ 16.10.0/JPype1 1.6.0(Java 21)로 수출물을 독립 재열기한다. 이들은 검증 의존성이며 앱 실행에는 Python/Java가 필요 없다. 기존 XML·계산·성능·보고서 회귀를 함께 실행한다.
