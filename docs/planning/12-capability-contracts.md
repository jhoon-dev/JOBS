# 기능군별 상세 계약 초안
전부 PROPOSED. 337행 개별 필드/기본값 검토를 보완하는 공통 계약이며 전체 요구 확정본이 아니다.

| 기능군·단계 | 실제 입력 후보 | 출력 | 예외·검증 가능한 수용조건 |
|---|---|---|---|
| J-01 WBS·작업 B | stable ID, parent ID, 순서, 코드마스크, 유형, 기간/단위, 달력 | 계층·작업 revision·코드·집계 | 부모/자기순환·중복코드 진단. 이동 후 ID·참조·총공수 보존(F11) |
| J-02 뷰 B/C | 프로젝트 revision, 열 정의, 필터식, 정렬·그룹, 선택ID | 표·간트·네트워크의 같은 객체와 범위 | 없는 필드·잘못된 식 오류. 뷰 전환 후 같은 ID/날짜, 필터의 숨김을 삭제로 해석하지 않음 |
| J-03 일정 A/B | duration/elapsed 구분, 달력버전, endpoint 관계/lag달력, anchor, 제약, profile | ES/EF/LS/LF, float/달력/완료기준, 임계·driving경로, 진단 | F01~04/F12. 모든 관계 부등식 확인, 불가능 제약과 누락 입력은 false success 금지 |
| J-04 다중 프로젝트 C/F | 프로젝트 snapshot 집합, 외부 endpoint·유효시각, 시간대/profile | 공동/외부 경계별 결과·stale 목록 | 외부 미확인을 연결 없음으로 바꾸지 않음. profile 불일치·오래된 anchor를 표시 |
| J-05 자원 C | resource ID·종류·단위·기간별max units·달력, assignment곡선·work·drive방식 | 시간대별 demand/capacity, 과부하 구간, 잔여 예측 | F05/F12. capacity 미정과0 구별, 시간/물량 혼합 합산 금지, 단가로 생산성 추정 금지 |
| J-06 평준화 D | 기준run, 대상범위, priority/tie-break, split/slack/horizon | 변경scenario, 지연·분할·미해소 과부하, 원인 | 동일입력 결정론, actual 보호, work 보존, 원본 자동 덮어쓰기 금지(F05) |
| J-07 기준선 B/C | 승인revision, 범위·달력·설정·가중치 snapshot, 승인자·사유 | 불변baseline ID, 현재/이전 비교 | 재기준선은 새버전. 비교baseline 없음이면 편차null, F09 |
| J-08 실적·진척 B/C | as-of instant, actual 원장이벤트, remaining, 측정법·가중치·승인 | 상태·현재예측, 진척/coverage·편차, historical위반 | F06~09/F12. 실제일자 보호, 미보고≠0, actual/remaining 단일원천, 원장 정정 이력 |
| J-09 원가 C/D | 통화·유효일단가·단위·기간별배정·고정비·원장·환율시점 | 계획/기준/실제/잔여비용과 기간별합계 | 환율미정시 통화혼합합계 금지, 과거actual 소급단가변경 금지, 단위/반올림 보존 |
| J-10 EVM D | BAC, timephased PV, credit method, EV근거, AC, as-of, EAC공식 | CV/SV/CPI/SPI, 조건부EAC/ETC, 근거 | F10. 0분모null, 집계비율은 합계분자/합계분모, 일정금액편차≠날짜지연 |
| J-11 위험 D/E | 위험ID, 확률·영향·분포·상관·달력·seed·반복·모델버전 | 시나리오와 quantile·수렴/민감도·가정 | 모델미정이면P50/P80 금지. 고정seed재현·반복수 수렴평가, 정답오라클은 별도 확보 |
| J-12 포트폴리오 E | 고유프로젝트집합, 전략가중치·예산·편익·용량·제약·as-of | 중복없는집계·시나리오·선정근거 | 누락지표0처리 금지, leaf중복방지, 통화·기간정합성, 규칙변경의 순위영향 설명 |
| J-13 협업 C/E | 로컬revision, 선택확장 연결, 담당자·권한, 제출·승인이벤트 | 검토상태·충돌·감사·알림후보 | 오프라인local기능 보존. 중복제출 멱등, 동시수정 검토, 승인없는actual승격 금지 |
| J-14 보고 B/E | report정의·필터·범위·as-of·baseline·profile·용지/배율 | 인쇄/출력과 원자료 연결·계산상태 | 화면과출력 합계일치, 미계산/stale표시, 페이지잘림/키보드/색외표시검증 |
| J-15 교환 B~F | 원본bytes참조·제품/빌드·형식·schema·옵션·IDnamespace | mapping·보존payload·진단·네독립호환판정 | 원본손실보고·고아참조·재수입중복 검증. 읽힘만으로재계산호환선언 금지 |
| J-16 운영 A~F | local파일revision·잠금·autosave·이관정책·권한·감사·백업 | 정상저장/복구본·충돌·이관결과·감사 | DESK-01~09. 저장중단/디스크부족/신버전파일/동시열기/OS별복구 시나리오, 기존정상본보존 |

## 행별 상세화를 위한 완료 게이트
10-detailed-requirements의 각 행에서 위 공통계약을 필요한 필드 목록·허용값·기본값·명령별 변경규칙으로 구체화한다. 정상/경계/실패 fixture에 입력값·기대값·출처·판정방법을 부여해야 '상세설계 승인'으로 전환한다. 현재 일괄 생성한 문장이나 feature 존재 자체는 이 게이트의 통과 증거가 아니다.
