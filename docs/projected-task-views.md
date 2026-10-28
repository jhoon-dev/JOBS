# 필터·정렬·WBS 구조 편집의 준비된 보기

2026-10-28 사용자 요청. VIEWBULK-01~04를 `a5fa558`·`a128aef`에서 구현 전에 확정했다. 전후 측정 도구는 제품 변경 전 `ae8d9ed`에 추가했고 구현은 `cea0bc4`, 중복 사본/기본 보기 준비 제거는 `85159e9`, 불필요한 Qt 선택 인덱스 확장 제거는 `696ffa9`다.

GUI는 보기 조건과 현재 표시 원본 행을 값으로 캡처한다. worker는 live 위젯/Document를 읽지 않고, 기존 bulk 준비 결과에 검색/필터/정렬/WBS 투영과 ID 기준 행 변경을 추가한다. 공통 함수는 일반 전체 갱신과 worker 양쪽에서 사용한다. 변경 후 일정은 오래됨이므로 유효한 날짜/임계 결과를 재사용하지 않는다. WBS 합계는 검색 결과가 아닌 하위 WBS 전체 범위다.

모델은 삭제 구간, 생존 ID의 layout 변경, 삽입 구간을 적용하며 각 단계에서 rowCount/data/persistent index를 일관되게 유지한다. 합계 128개를 넘는 구간은 준비된 입력/행 매핑으로 한 번 reset한다. 이 경우도 GUI가 큰 PlanInput을 복사하거나 검색/정렬을 반복하지 않는다. 기본 보기는 기존 경로를 유지한다.

보기 조건과 원본 revision/session이 맞지 않거나 새 계산이 적용되면 기존 전체 갱신으로 복귀한다. 캡처 시 아직 검색 debounce가 대기 중이어도 실제 표시 행과 목표 검색 조건으로 준비한 결과는 적용할 수 있다. 결과의 token과 이전 모델 행을 검사하며, 적용 뒤 렌더링 조건을 기록해 늦게 도착한 검색 타이머도 중복 reset을 하지 않는다(`a0de474`, 타이머를 기다리지 않는 회귀 검사 포함). 취소·실패·stale 결과는 투영/선택/입력 이력을 바꾸지 않는다. 준비 중 시작한 새 셀 편집은 결과 반영 직전에 먼저 확정해, 이전 결과/정렬 이동이 초안을 덮어쓰지 않도록 revision 경합을 검사한다. 선택은 살아 있는 표시 ID만 유지하고 새 ID가 필터에 숨으면 선택하지 않는다. 간트의 행 순서는 표와 일치하며 오래된 막대는 표시하지 않는다.

시간은 단일 전후 관측으로 기록한다. 모달 입력/확인과 준비 대기, 명령 반환과 완료/GUI 갱신의 범위를 구분하고 고정 FPS/벽시계 상한을 주장하지 않는다. 전체 회귀/독립 reader 및 네이티브 검사 결과와 남은 비용을 아래에 기록한다.


## 구현과 검사 연결

`TaskViewProjection`은 QWidget/Document를 참조하지 않는 UI 계층의 값 계산 함수다. 기존 full refresh와 worker에서 같은 Qt Unicode 검색·안정 정렬·미정 마지막 규칙을 사용한다. WBS 하위 집계는 검색/필터 이전 범위이며, 선행 번호는 전체 원본 작업 순서다. 변경 뒤에는 날짜가 실효화되므로 임계 필터 표시는 비어 있고 시작일 정렬은 미정의 원본 순서로 돌아간다.

`EditRequest`의 선택적 `ViewPreparation`은 보기 조건, compact 원본 행, 계산 generation을 소유한다. worker가 BulkView의 행/위치·변경 구간·생존 ID 순서와 WBS 합계를 준비한다. 적용 시 revision/session뿐 아니라 계산 generation과 보기 token/이전 모델 행도 검사한다. 이미 바뀐 보기나 새 계산에 이전 투영을 적용하지 않는다. 새 계산/단일 셀 편집 등은 기존 안전 경로를 사용한다.

모델은 old cache로 삭제 알림을 마친 뒤 준비된 입력을 이동한다. 생존 작업의 새 값을 먼저 dataChanged로 알리고 layout/persistent index를 재매핑하므로 layout 알림 사이의 항목 값이 바뀌지 않는다. 이후 삽입 구간을 적용한다. GUI가 선택 ID를 따로 소유하므로 모델 갱신 전에 Qt 선택을 신호 차단 상태에서 비우고, 마지막에 살아 있는 ID만 범위로 복원한다. layout 시 대량 선택 셀을 임시 persistent index로 확장한 뒤 다시 버리던 중복 비용을 피하며 외부 persistent index와 현재 작업은 모델의 ID 재매핑으로 보존한다. 합계 128개 초과 구간은 준비된 입력을 한 번 reset하며 GUI에서 PlanInput 복사/검색/정렬을 반복하지 않는다. 이전 입력·캐시의 큰 해제는 worker에 넘긴다.

반영 직전의 새 미확정 셀 입력은 GUI에서 확정한다. 이 확정으로 문서가 바뀌면 기존 worker 결과는 stale로 거절한다. 이 검사도 파일/undo/취소 경계를 통과한 뒤의 단일 반영 정책을 유지한다.

| 요구사항 | 검사 |
|---|---|
| VIEWBULK-01 | `projected_views_ui`: 검색·이름/기간 정렬·WBS 하위/결합·마일스톤·미배정, full model의 14열/원본 헤더 oracle, 필터와 별개인 WBS 집계 |
| VIEWBULK-02 | Qt 모델 검사기·persistent ID, 표시 출입/정렬 이동/다중 삽입/undo, 129개 분산 삽입의 bounded reset; 기존 `bulk_editing_contract_ui`·`task_editing_ui` |
| VIEWBULK-03 | 준비 중 WBS 변경의 캡처 대상 보존, gate로 고정한 새 셀 초안 경합, valid→stale 임계 필터/간트, 기존 async/취소/session/저장/F9 회귀 |
| VIEWBULK-04 | 전체 CTest 41개, 네이티브 `check-projected-views-native.py`, 동일 도구 전후 16개 보기 사례와 원시 JSON/log, 기존 독립 XLSX/PDF/XML/XER·계산 해시 |

## 측정 범위와 남은 비용

`run-projected-view-probes.py`는 검색, 이름 정렬, 하위 WBS, WBS+검색+정렬의 네 보기에서 1천/1만 작업·기준선0/3의 16개 사례를 실행한다. WBS A는 전체 작업 절반을 가진 자식 A1을 포함한다. 각 사례는 전체의 10%(100/1,000개)를 삽입·복제·붙여넣기·삭제하고 각각 undo한다. 검색 사례에서 삽입/붙여넣기의 이름은 query를 벗어나므로 숨은 작업/표시 출입도 포함한다. FS 기간 계획 fixture이며 공수 역산 타이밍을 일반화하지 않는다.

동일한 probe 소스를 참조 `ae8d9edaa360388012fdb98cd79b633192919971`와 최종 제품에 적용한다. test source만 overlay하며 참조 제품 소스/CMake는 바꾸지 않는다. Git revision의 dirty 여부와 제품 source/CMake manifest를 함께 기록한다. 동일 CI 실행기의 Qt/컴파일러를 사용한다. setup_ready는 활성화/클립보드 준비 대기로 측정에서 제외한다. 명령 반환에는 작은 계측 오버헤드, 모달 삽입/삭제에는 자동 입력/확인이 포함된다. 완료는 편집 worker/Qt 완료 이벤트/GUI 갱신을 포함하고 별도 해제 worker 완료와 OS 화면 표시/FPS를 포함하지 않는다.

`gui_model_task_visits`는 GUI의 Task 값 비교/복사 방문 수이며 모든 GUI 작업 수가 아니다. 준비된 reset도 이 방문이 0일 수 있다. GUI의 compact 행 복사/구간 이동, selection 복원/스크롤 clamp, 자원 표시 QString 변환, Qt 레이아웃/유효 일정 실효화 비용은 남는다. 닫힌 bulk 명령과 해당 이력은 owning read-only 원본을 공유하고 명령 자체가 첫 변경 사본을 만든다. 이 경계에서만 중복 초기 Project 사본을 생략하며 일반 편집은 기존 사본 경로를 유지한다. 투영 worker가 덮어쓸 기본 루트 행/위치/집계 준비도 생략한다. worker는 여전히 큰 프로젝트/표시 입력 사본, dirty·delta 비교와 전체 보기 계산을 수행한다. source/view가 바뀌는 fallback, 단일 셀 편집·새 계산의 전체 갱신은 남는다. 모든 변경의 reset 제거·상수 시간·고정 FPS를 보장하지 않는다.

## 최종 관측

최종 `a0de474`의 [CI 실행 40632368294](https://github.com/jhoon-dev/JOBS/actions/runs/40632368294)은 Windows/MSVC·macOS 각각 **41/41** 테스트와 projected/bulk/model/async/responsive 네이티브 검사, XLSX 5·PDF 6·Project XML·XER TSV/MPXJ 독립 판독을 통과했다. CTest는 Windows 103.46초, macOS 67.65초였다. 로컬 MinGW Release도 41/41(128.44초), projected 네이티브 및 16개 관측/독립 판독을 통과했다.

[검증 JSON](validation/projected-views-2026-10-28/verification.json)에 동일 도구 전후 16개 보기 사례(OS별 128개 작업/undo), 전체 원시 JSON/log·회귀 시간·CI 정보·PNG·로컬 기록을 보존한다. 1천/1만 기본·공수 일정의 전체 결과 SHA-256은 두 OS 모두 기존 golden과 일치했다(8건).

- Windows: reset 120→8회, layout 8회, GUI Task 값 비교/복사 방문 합계 0회.
- macOS: reset 120→8회, layout 8회, GUI Task 값 비교/복사 방문 합계 0회.

많이 흩어진 정렬 복제/undo의 reset은 구간 예산을 적용한 결과다. reset 0을 모든 보기·변경에 보장하지 않는다. 다음은 기본 FS 기간 계획 1만 작업·기준선 3개에서 1,000행 붙여넣기의 **단일 관측(ms)**이다. 심박 최대 간격은 OS/스케줄러 영향도 포함하므로 순수 GUI CPU 시간이나 FPS가 아니다.

| OS | 보기 | 완료까지 전 → 후(ms) | 심박 최대 간격 전 → 후(ms) | GUI Task 방문 전 → 후 | reset 전 → 후 / layout 후 |
|---|---|---:|---:|---:|---:|
| Windows | 검색 | 101.35 → 75.31 | 16 → 15 | 10000 → 0 | 1 → 0 / 0 |
| Windows | 이름 정렬 | 96.71 → 84.34 | 15 → 16 | 10000 → 0 | 1 → 0 / 1 |
| Windows | 하위 WBS | 91.29 → 65.80 | 12 → 11 | 10000 → 0 | 0 → 0 / 0 |
| Windows | WBS+검색+정렬 | 95.32 → 70.68 | 16 → 15 | 10000 → 0 | 1 → 0 / 0 |
| macOS | 검색 | 51.94 → 70.04 | 14 → 19 | 10000 → 0 | 1 → 0 / 0 |
| macOS | 이름 정렬 | 49.30 → 90.61 | 25 → 27 | 10000 → 0 | 1 → 0 / 1 |
| macOS | 하위 WBS | 96.37 → 51.03 | 26 → 22 | 10000 → 0 | 0 → 0 / 0 |
| macOS | WBS+검색+정렬 | 68.57 → 55.51 | 25 → 18 | 10000 → 0 | 1 → 0 / 0 |

GUI의 큰 Task 비교/복사와 검색/정렬은 제거했지만 worker의 투영/ID diff와 Qt 이벤트/레이아웃 비용은 남는다. 일부 단일 완료 표본은 증가했으며 전후 JSON의 모든 작업/undo와 `slower_settled_observations`에 보존했다. 완료 시간의 보편적인 개선이나 벽시계/FPS 상한을 주장하지 않는다.

네이티브 화면: [Windows](validation/projected-views-2026-10-28/Windows/projected-views-native.png), [macOS](validation/projected-views-2026-10-28/macOS/projected-views-native.png).
