# 핵심 계산 규칙 초안
상태: PROPOSED / 전문 검토 원고 본문 검토·통합. 구현 계약 확정본이 아니다.

## 입력과 출력
입력: 프로젝트/작업/의존관계/달력/자원/배정/제약/실적/상태일/계산 정책의 고정 버전.
출력: 작업별 시작·완료·조기/늦은 날짜·여유·임계 상태, 자원 수요, 편차, 진단 및 원인 연결.
실패 결과에 정상 완료 상태를 붙이지 않는다. 계산 결과에는 시간대·단위·정밀도·버전이 따라야 한다.

## 시간과 달력
근무구간은 반개구간 [start,end)로 제안한다. 달력 예외의 우선순위, 교대 근무, 자정 통과, DST 중복/누락 시간 처리와 휴일 충돌은 명시적 계약이 필요하다.
작업 달력과 자원 달력의 교집합/우선권, elapsed duration, lag의 기준 달력은 정책 필드로 드러낸다. 기본값은 전문 검토 후 확정한다.
0기간 마일스톤은 경계 시각에 위치한다. 날짜만 입력했을 때의 시작/완료 시각을 명시하고 조용히 자정으로 바꾸지 않는다.

## 네트워크
개념 제약은 FS: S후≥F선+L, SS: S후≥S선+L, FF: F후≥F선+L, SF: F후≥S선+L이다. 여기서 +L은 달력 정책에 따른 시간 연산이며 단순 벽시계 덧셈이 아니다.
순환은 경로를 진단한다. 하드 제약과 의존관계 충돌은 표시한다. deadline과 강제 완료 제약은 구분한다. 음수 lag, 수동 일정, 역방향 계산은 후보로 보존하되 검토된 정책 없이 혼합하지 않는다.
CPM의 forward/backward pass, 프로젝트 완료 기준, 총/자유 여유, 다중 임계경로, 음수 여유와 임계 임계값은 전문 검토에서 확정한다.

## 자원과 작업량
work = duration × units는 일정한 가용량·달력·배정 조건에서만 성립한다. 고정 기간/작업량/단위와 effort-driven 정책을 명시한다.
인력등급·단가·급여는 자동 생산성 배수가 아니다. 생산성을 모델링하려면 별도 측정 변수와 가정·출처·승인이 필요하다.
평준화는 우선순위·분할 허용·제약 준수·동률 해소 규칙을 기록한 별도 계산이다. 원본 일정과 비교한 뒤 반영한다.

## 실적과 예측
실제 시작/완료는 관측 사실이며 자동 일정 계산으로 이동하지 않는다. 실제 완료 전 작업의 잔여량을 0으로 만들 때 일관성을 확인한다.
상태일 이전 미완료 작업, out-of-sequence 진행의 retained logic/progress override 등의 처리 방식은 명명된 정책으로 분리한다.
예상 완료일은 실적+잔여 예측으로 계산한다. 기간/작업량/물리 진척률을 혼합 평균하지 않는다. WBS 집계 가중치와 0분모 정책이 필요하다.

## 원가와 성과
계획 예산·기준선 예산·실제 원가·예측 원가를 분리한다. EVM은 PV/EV/AC 정의와 상태일·기준선·성과 측정법이 고정되어야 한다.
CV=EV-AC, SV=EV-PV, CPI=EV/AC, SPI=EV/PV는 정의 충족 시에만 계산하며 0분모는 미정의로 표시한다. EAC/ETC는 선택 산식과 가정을 함께 제공한다. 일정 금액 편차를 날짜 지연으로 해석하지 않는다.

## 검증 사례 목록
CAL-01: 금요일 8시간 작업+월요일 휴일의 완료 시각.
CAL-02: 네 의존관계와 양/음 lag를 각각 검증.
CAL-03: 다이아몬드 네트워크의 두 임계경로와 여유.
CAL-04: 순환·불가능 제약·근무시간 없는 달력의 오류.
CAL-05: 시간대·DST·야간 교대·마일스톤 경계.
CAL-06: 서로 다른 자원 달력과 기간별 가용량.
CAL-07: 기준선 저장 후 실적 입력 시 기준선 불변.
CAL-08: out-of-sequence 실제와 잔여 일정.
CAL-09: 진행률 유형별 WBS 집계와 0분모.
CAL-10: 다중 통화·단가 변경·EVM 기대값.
각 사례는 확정 입력, 수동 기대값, 계산 정책, 허용 오차, 출처와 검토자 서명이 있어야 한다. 현재 사례는 설계 목록이며 실행·통과 증거가 아니다.

## 전문 검토 핵심 통합 — 설계 제안
전문 원고 libfile_97c3a46627348191802a0197a3722842의 457줄 전체를 공식 Library read로 검토했다. 원본 bytes 다운로드는 실패했으며 아래는 본문에서 통합한 설계 제안이다. 사용자 확정 정책이나 실행 검증 결과가 아니다.
WBS는 보고·집계 트리, 실행 의존관계는 별도 그래프다. 계층에서 실행 링크를 자동 생성하거나 요약 객체를 실행 작업처럼 계산하지 않는다.
duration/work/capacity/progress는 독립 변수다. 단가·인력등급·가용량을 자동 생산성으로 해석하지 않는다.
FS/SS/FF/SF는 endpoint 부등식이며 lag 달력·기간 달력·정밀도·시각 경계를 명시한다.
actual/baseline은 불변 버전이다. 실적 정정은 사유를 가진 새 버전으로 남긴다. remaining을 재계산하고 과거 actual을 이동하지 않는다.
필수 입력이 부족하면 확정적인 전체 forecast를 만들지 않는다. 계산 가능한 부분·누락·가정·결과 사용 가능 여부를 분리한다.

## 수동 검산 fixture 초안
가정: 연속 근무시간, 동일 달력, FS 0 lag, 시작 t=0h, 비분할·비자원제약 CPM. 벽시계 날짜 사례를 대신하지 않는다.

| 작업 | 기간 | 선행 | ES | EF | LS(완료56h) | LF | 총여유 |
|---|---|---|---|---|---|---|---|
| A | 16h | 없음 | 0h | 16h | 0h | 16h | 0h |
| B | 24h | A | 16h | 40h | 16h | 40h | 0h |
| C | 8h | A | 16h | 24h | 32h | 40h | 16h |
| D | 16h | B,C | 40h | 56h | 40h | 56h | 0h |

완료는 56h, C 여유는 16h다. 요구 완료 48h를 backward pass 기준으로 적용하면 A/B/D 총여유는 -8h, C는 8h다. 요구 완료를 경고용 deadline으로만 해석하는 profile과 구분한다.
고정 작업량 가중치가 40h/10h이고 같은 유형의 진척이 50%/100%이면 (40×0.5+10×1)/50=60%다. 물리·기간·원가 진척으로 일반화하지 않는다.
위 사례는 수동 기대값 제안이며 엔진 실행·제품 비교·테스트 통과가 아니다.

## Semantic profile과 호환성
공급자·제품군·버전·빌드·스케줄 옵션·달력·정밀도·실적·제약·진척·원가 정책을 profile로 보존한다.
import/원본 보존/JOBS 재계산/원제품 왕복을 각각 판정한다. 불일치는 입력 누락·의미 차이·미지원·계산 결함으로 분류하고 근거를 기록한다.

## 원고의 세부 계약 반영
- C01~03: 계산 입력은 불변 revision, null은 0과 다름, stable ID는 이름/행번호/WBS 코드와 분리. 외부 ID namespace와 원본 텍스트 보존.
- T01~09: UTC instant·원래 현지시각·IANA 시간대·tzdb 버전 보존. 1초 정밀도는 제안. 날짜만 입력한 값은 변환 규칙 없으면 DATE_TIME_RESOLUTION_REQUIRED. 입력의 1d 환산과 실제 근무달력 분리. 달력 예외 우선순위는 날짜 예외→주간 패턴→명시 상위 버전. horizon 내 근무가 없으면 NO_AVAILABILITY_IN_HORIZON. AddWork는 휴일 경계에서 단순 역함수가 아니므로 FF/SF endpoint 부등식 재확인. DST 누락 시각은 오류, 중복 시각은 fold/offset 선택.
- G01~04: WBS summary/LOE와 summary dependency는 원본 보존하되 profile 검증 전 재계산 미지원. 보고 집계는 고유 leaf 집합으로 중복 방지.
- D01~04: 기본 FS0/project lag calendar는 제안. 비율 lag는 분모 필드·대상·revision·해석 초를 보존. native DAG는 SS0 포함 모든 방향 순환 차단 제안.
- CPM01~07/CON01~03: 초기 native는 시작 anchor ASAP 제안. 자연 완료와 required finish 구분. backward pass로 음수 여유가 나와도 불가능한 음수 시작을 실행안으로 표시하지 않음. 일반 관계의 자유여유에 FS 전용 산식을 쓰지 않음. 다중 달력 start/finish float와 참조 달력 보존. critical-by-float와 driving path 분리. deadline은 native에서 경고·별도 deadline float이며 MSO/MFO 등 endpoint 제약과 다름. 외부 anchor 누락/만료는 INCOMPLETE/STALE.
- R01~06: CPM은 capacity 보장 아님. 기간별 demand/capacity 비교. DURATION_DRIVEN과 WORK_DRIVEN 구분, 후자는 분할/대체 가능한 공수 전제가 필요. Fixed Units/Work/Duration 편집은 유지할 두 값을 명시. 배정자 추가/삭제는 단일 units 편집과 다름. 평준화는 독립 scenario와 안정된 tie-break, 미해소 과부하를 반환.
- P01~05: work/기간/물리/물량/milestone 진척 분리. 가중치는 같은 차원의 양수 값이며 baseline/as-of/leaf 집합/coverage 보존. 미보고가 있으면 전체 확정 진척 null. known-only를 전체값으로 표시하지 않음.
- A01~05/B01~06/AUD01~02: 상태일 instant·실제 상태 불변조건·실적 원장 단일 원천·정정 이벤트. retain-remaining-logic은 native 제안. 기준선은 달력·관계·설정까지 버전 묶음. 담당자 기대완료/결정론 결과/확률 quantile 구분. 과거 actual cost는 새 단가로 덮어쓰지 않음. CPI는 합계EV/합계AC, EAC 공식 ID·전제 보존. 감사·원인 추적은 설계이며 규제 인증 보장 아님.
- Q01~03: 영역별 VALID/INCOMPLETE/CONFLICT/UNSUPPORTED/STALE와 진단 반환. 부분 일정의 최대 완료를 전체 완료로 표시하지 않음. 기간 중심 논리 일정은 가능해도 자원 미확인이면 resource-feasible로 부르지 않음. 3점 추정만으로 P50/P80 생성 금지.

## 전문 원고 fixture 12개 연결
CAL-01~10은 주제 분류이며 아래 F01~F12는 구체 기대값을 가진 별도 인수시험 제안이다. 원고의 독립 산술 검토를 인용하며 실제 엔진·제품 실행 통과를 의미하지 않는다.

| 원고 ID | 입력·기대값 요약 | 연결 |
|---|---|---|
| F01 | A16→B24/C8→D16 =56h, C TF/FF16h. required finish48이면 A/B/D TF−8h와 CONFLICT | CAL-03, J-03 |
| F02 | A S8/F24, B8: FS+4→28/36, SS+4→12/20, FF+4→20/28, SF+4→4/12, FS−8→16/24, SS−12→0/8 | CAL-02, J-03 |
| F03 | Seoul 월~금09~17, 10/30금13시+16working h→11/3화13시. 11/2휴일이면11/4수13시. 16elapsed h→10/31토05시. FS 양의기간 월09시, milestone 금17시 | CAL-01/05, J-03 |
| F04 | A/B8의 FS 순환과 SS0 순환 차단. A0~16→B8인데 B MSO8이면 CONFLICT | CAL-04, J-03 |
| F05 | 자원1, 독립 A/B8h를 각각1 배정: work16/capacity8, peak200%. A우선 평준화 B화요일, work/기간 보존 | CAL-06, J-05/06 |
| F06 | 40h×50%+10h×100%=60%. B미보고면 전체null, coverage80%, contribution40%, 경계40~60%, known-only50% | CAL-09, J-08 |
| F07 | original16, actual duration8/remaining12, actual/remaining work10/10, physical35%: native40%, P6식25%, work50%, physical35% | CAL-09, J-08 |
| F08 | data12, A actual0~8/rem8→A종료20. B actual4~8/rem4: retained→20~24, override→12~16. actual 유지·역사 위반 표시 | CAL-08, J-08 |
| F09 | baseline16h/16work, data8/actual8/rem16→현재24h/24work, 편차+8, 기준선 유지. 기준선 없으면 편차null | CAL-07, J-07 |
| F10 | BAC1000/PV500/EV400/AC600: CV−200, SV−100, CPI2/3, SPI0.8, 선택EAC1500. 전제 누락이면null | CAL-10, J-10 |
| F11 | A8/B16 WBS 이동 후 총24와 stable ID 보존, WBS1 8→24/WBS2 16→0, 그래프 불변 | J-01 |
| F12 | A16/B기간미정이면 전체finish null. B8 알려져도 capacity미정이면 논리일정과 자원 검증 상태 구분 | PRD-03, J-03/05 |

전문 원고 엔진 단계 E0~E5는 로드맵 A~F와 별도 축이다: E0→A, E1→A/B, E2→B/C/D, E3→C/D, E4→C/F, E5→D/E/F. 기능 전체를 한 단계에서 완료했다는 의미가 아니다.
