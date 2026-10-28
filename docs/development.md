# C++/Qt 초기 구현

2026-10-26 사용자 요청에 따른 구현 착수. PRD-01(초안 입력 일부), PRD-02(WBS·표·간트 일부), PRD-03(기본 FS CPM), PRD-04/05(기준선·실적·편차 일부), PRD-08,
DESK-01/03/04/05 및 FILE-01/02/03/05/06/07/09의 초기 수직 흐름이다.
전체 PRD나 FILE-T01~09의 완료 선언은 아니다.

## 구성

- `jobs_domain`: 표준 C++20 프로젝트·작업·WBS·기준선·실적 원장·자원/배정·불변 시나리오·날짜·revision 검증 및 계층 탐색. Qt 의존 없음.
- `jobs_calculation`: 표준 C++20 FS0 CPM, 달력, retained remaining 예측, 기준선 준비/검증, 계획/예측 비교, 자원 수요/가용량 및 비분할 직렬 평준화. Qt 의존 없음.
- `jobs_application`: 문서 세션, 생성/열기/편집/저장/사본, 추상 저장 포트.
- `jobs_persistence`: Qt Core JSON, checksum, 잠금, 외부 변경 감지, 원자적 교체.
- `JsonRecoveryStore`: 사용자 로컬 복구 세션/세대, 완료·해시 검증, 프로세스 잠금, 원본 상태/변경 미리보기. `AutoSaveController`가 입력 스냅샷을 백그라운드에서 기록한다.
- `jobs_reporting`: Qt Gui 기반 숫자/텍스트 표시·공통 추이, 공개 OOXML XLSX와 페이지를 나누는 PDF 출력. 외부 런타임/Office 라이브러리 없음.
- `jobs_ui`: Qt Widgets WBS·표·간트, 기준선/상태일/실적 대화상자, 자원 등록/배정·비동기 부하/시나리오 A/B 비교, 변경 상세, 미저장 변경 확인.

기간 미정(null)과 마일스톤(0분)을 구분한다. 기간·FS 선행·휴일을 입력하면 F9로
기간 중심 논리 일정을 계산한다. F9는 자원과 독립인 논리 일정이다. 자원 메뉴에서 별도로 배정·일별 부하와 평준화 시나리오를 비교한다.
기본 달력은 월~금 09–12/13–18이며 날짜는 시간대 없는 ISO civil date다.
WBS는 별도 그룹 트리로 편집하며 작업을 그룹에 배치한다. 간트에 최신 계산 결과·마일스톤·FS 연결선을 표시한다.
추적 메뉴에서 기준선을 새 버전으로 저장하고 상태일·실제 시작/완료·실제 소요/잔여 근무분을 입력한다.
기준선은 불변 입력·달력·결과 스냅샷, 실적은 정정 사유를 가진 append-only 보고 원장이다.
현재 계획과 상태일 이후 잔여 예측을 나누어 비교하며, 간트에서 선택 기준선을 회색으로 표시한다.
공수 중심 일정·왼쪽 끝 resize/관계 생성 드래그는 후속 범위다. 간트 날짜/기간 드래그, 작업 제약·마감, FS·SS·FF·SF/lag와 작업별 달력, 일반 입력 실행 취소/다시 실행과 EVM용 물리 진척은 별도 구현 계약을 따른다.
[현재 계산 정책·수용시험·입력 예제](calculation-native-v1.md)를 참고한다.
[WBS·표·간트 사용법과 검증 계약](wbs-gantt.md)을 함께 따른다.
[기준선·실적·변경 비교 계약](baseline-progress.md)에 재현 예제와 예측 범위를 기록한다.
[자원·평준화 계약](resources-leveling.md)은 정률 배정·일별 최대 부하·별도 시나리오·재계산/출처와 후속 범위를 정의한다.

## 빌드와 실행

초기 개발 기준: CMake 3.24+, C++20 컴파일러, Qt **6.8.3 Core/Widgets/Concurrent**.
Qt 패키지와 컴파일러 ABI를 맞춘다. Qt 6.8 이상을 CMake에서 허용하되 CI 기준 버전은 고정한다.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
& ./build/Release/JOBS.exe
```

MinGW는 해당 Qt의 `bin`과 MinGW의 `bin`을 PATH에 추가한 뒤 Ninja를 사용한다.
이 작업 폴더에는 `.deps/Qt`에 개발 도구를 설치했으며 `./scripts/dev.ps1`로 빌드·검사·실행한다.
`./scripts/dev.ps1 -TestOnly`는 실행 창을 열지 않고 검사한다. 이 로컬 스크립트는 Python 3.14의
사용자 Scripts 경로를 사용한다. 다른 환경에서는 아래 일반 명령을 사용한다.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64
cmake --build build --parallel
ctest --test-dir build --output-on-failure
& ./build/JOBS.exe
```

macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.3/macos"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
open build/JOBS.app
```

개발 실행에는 Qt DLL/라이브러리 경로가 필요하다. 설치 패키지·서명·배포·라이선스 검토는 별도다.
GitHub Actions는 Windows/macOS 빌드와 계산·파일·화면 테스트를 수행한다.
macOS는 `macos-15`와 Xcode 16.4를 고정한다. 최신 SDK에서 Qt 6.8.3의 AGL 링크 실패를 확인했다.

## 현재 `.jobs` 형식 (schema 6)

UTF-8 JSON 한 파일에 `format: JOBS`, `schemaVersion: 6`, `payload`, `sha256`을 보존한다.
payload에는 documentId, derivedFrom, revision, name, startDate, calendar, tasks, holidays,
calculationProfile, wbsNodes, resources(rates 포함), assignments, currency, costProfile, taskCosts, statusAt, baselines, progress, scenarios, costEntries가 들어간다. 각 작업은 UUID·이름·정수 근무분 또는 null·FS 선행 ID 목록·wbsId를 갖는다.
WBS 노드는 ID·이름·parentId를 가지며 빈 부모/소속 ID는 프로젝트 루트다. 순환·누락 부모·누락 소속을 거절한다.
현재 계산 결과는 세션 cache이며 재열기 후 F9로 재계산한다. 기준선의 동결 결과는 파일에 보존한다.
기준선 입력과 결과는 열기/저장 시 고정 엔진으로 재검산한다. 실적은 source document/revision·상태일·UTC 기록시각·보고자·사유·이전 보고 ID를 보존한다.
checksum은 Qt의 compact JSON payload에 대한 SHA-256이며
보안 서명이 아닌 우발적 손상 검사다. 현재 최대 64 MiB, 첨부/외부 링크/교환 형식 없음.
checksum과 구조·필드·날짜·ID 중복·revision을 검사한다. 미지원 schema/필드는 무음 삭제하지 않고 거절한다.

첫 저장은 새 프로젝트 ID를 유지한다. 저장된 문서의 다른 이름 저장은 새 documentId와
derivedFrom을 만들고 작업 ID를 유지한다. 기존 대상 파일 덮어쓰기는 초기 버전에서 거절한다.
현재 파일에 저장할 때는 SHA-256 fingerprint가 열린 시점과 같은지 쓰기 전·교체 직전에 확인한다.
실패하면 dirty 상태와 이전 문서 세션을 유지한다.

QSaveFile로 임시 파일을 쓰고 flush 후 commit한다. 직접 덮어쓰기 fallback은 비활성화한다.
QLockFile sidecar는 전달 파일이 아니며 단일 writer를 위한 협조적 잠금이다.
잠금 실패 시 읽기 전용 열기와 다른 이름 저장을 제공한다.
외부 비협조 writer의 검사 직후 변경, 네트워크/동기화 폴더 원자성, 정전 내구성은 보장하지 않는다.
schema 1/2/3/4/5는 ID·revision·기존 입력을 유지하여 읽고 새 필드에 null/빈 목록/미분류/빈 이력을 적용한다.
이전 형식의 원본 덮어쓰기는 거절한다. 편집 후 저장하면 다른 이름 저장으로 schema 6 사본을 만들며
원본 bytes를 보존한다. 일반적인 다단계 이관·별도 백업 순환 정책은 아직 없다. 자동복구 체크포인트는 원본과 분리한 별도 레코드로 제공한다.
미지원 버전은 원본을 수정하지 않는다.

## 검증 범위

`jobs_file_tests`는 실제 임시 파일로 생성/편집/revision/재열기, Unicode 경로·데이터,
사본의 ID/계보·원본 bytes 보존, 중복 writer 잠금, 읽기 전용 사본, 외부 변경 거절,
JSON 손상/checksum/신버전/알 수 없는 필드/중복 ID, 열기·저장 실패 시 세션 보존,
단일 파일 복사, 계산 입력 왕복, revision/문서 ID가 다른 결과의 STALE 처리,
schema 1/2→5 사본 이관·WBS 계층/작업 소속 왕복·손상 계층 거절을 검사한다. 이는 FILE-T01/T02/T06/T08/T09 일부에 해당한다.

GUI 수동 점검: 새로 만들기→작업 목록 입력→저장→작업명 편집→열기/종료 시
저장·버리기·취소→다른 이름 저장→원본 재열기. 두 앱으로 같은 파일 열기와 읽기 전용 사본도 확인한다.
실제 OS 간 전달, 강제종료·디스크 부족·장치 제거, 접근성/IME/대용량 성능 시험은 미완료다.

`jobs_ui_tests`는 offscreen Qt Widgets 환경에서 새 프로젝트 대화상자·작업 목록 입력·표 편집·
저장 대화상자·종료 취소·종료 시 저장, 기간·선행·휴일 입력, 비동기 계산·계산 중 편집,
누락/순환/오래된 계산의 날짜 숨김과 재계산을 자동 검사한다. 네이티브 파일 대화상자와 실제 데스크톱
화면의 수동 검증을 대신하지 않는다.

`jobs_wbs_tests`는 F11 합계 변경·ID/그래프/날짜 보존, 중첩 그룹·이동·빈 삭제,
미정 기간·오래된 집계·부모 순환/누락·원자적 편집 거절·읽기 전용·깊이 2천 계층을 검사한다.
`jobs_wbs_gantt_ui_tests`는 필터와 프로젝트 행 번호, 필터된 편집, 작업/하위 WBS 이동,
추가/이름 변경/삭제, 간트 막대·마일스톤·FS 링크·상호 선택·배율·스크롤·오래된 막대 숨김을 검사한다.

`jobs_tracking_tests`는 F09의 +8h·기준선 불변/버전·계획/예측 분리, F08 잔여 논리, 완료 실제 고정,
상태일 조회·과거일 정정·잘못된 보고 거절·미보고/미정·읽기 전용을 검사한다.
`jobs_tracking_file_tests`는 기준선/실적 원장 왕복·사본의 원본 계보·달력 불변·기준선 재계산 무결성·잘못된 원장·schema 3→5 사본 저장을 검사한다.
`jobs_tracking_ui_tests`는 기준선/상태일/실적/비교 대화상자, +8h 표시·미보고 coverage·회색 기준선 선택,
모순 보고 오류·정정 이력·새 기준선 버전·저장/재열기를 검사한다.

## 실행 증거 · 2026-10-26

Windows x64, Qt 6.8.3, MinGW GCC 13.1.0, CMake 4.4.4, Ninja에서
첫 파일 구현은 `scripts/dev.ps1 -TestOnly`로 Release 빌드 및 2/2 테스트에 통과했다.
기본 계산 추가 후 `schedule_contract`, `file_lifecycle`, `ui_lifecycle` 3/3에 통과했다.
WBS·간트 추가 후 `wbs_contract`, `schedule_contract`, `file_lifecycle`, `ui_lifecycle`, `wbs_gantt_ui` 5/5에 통과했다.
WBS·간트 화면 테스트도 Windows 네이티브 플랫폼에서 통과했고 생성한 화면에서 트리·표·연결선·마일스톤을 확인했다.
기준선·실적·비교 추가 후 `tracking_contract`, `tracking_file`, `wbs_contract`, `schedule_contract`,
`file_lifecycle`, `ui_lifecycle`, `wbs_gantt_ui`, `tracking_ui` 8/8에 통과했다.
추적 화면 테스트를 Windows 네이티브 플랫폼에서도 실행해 통과했으며, 생성한 비교 화면에서
기준선/현재 계획/예측 +8h·보고 범위·기간 진척·변경 상세가 읽히는지 확인했다.
추가로 같은 화면 테스트를 Windows 네이티브 플랫폼(`-platform windows`)에서 실행하여 통과했고,
앱이 생성한 화면 이미지에서 한글·날짜·진단 표시를 확인했다. offscreen 테스트 이미지는
Windows의 폰트 로딩 차이로 글리프가 사각형으로 보이므로 시각 검토에 사용하지 않았다.
macOS 및 MSVC 검증은 GitHub Actions에서 별도 추적한다.

자원 증분은 `resource_contract_file`, `resource_ui`를 포함한 10개 테스트로 확장했다.
자원/배정·날짜 예외·불변 시나리오/기준선·schema 4→5 이관 및 분 단위 독립 수요 검산을 포함한다.

이번 증분에서 로컬 Release 전체 10/10 및 Windows 네이티브 자원 화면 테스트를 통과했다.
등록·A/B 일정 비교·원본 과부하·주 화면 이미지를 확인하고 다크 테마의 경고 행 글자 대비를 보정했다.

원가/EVM 증분은 유효일 단가·작업 고정비/성과 측정법·원가 보고 원장 및 비동기 분석/추이를 추가했다.
정의·통화/금액 한도·반올림·기준선 코호트·수용시험은 [원가·EVM 계약](costs-evm.md)을 따른다.
`cost_contract_file`, `cost_ui`를 포함한 전체 12개 테스트로 확장하며 F10·CPI 합계 비율·AC 불변·schema 5→6 이관을 검사한다.

원가/EVM 증분의 로컬 Release 전체 12/12와 Windows 네이티브 원가 화면 테스트를 통과했다.
단가·F10 EVM 표·PV/EV/AC 추이의 한글/금액 표시와 기존 자원 화면의 단가 보존을 확인했다.

보고서 증분은 `reports_export`, `reports_ui`를 더해 CTest 14개로 확장했다. `jobs_calculation`의 표준 C++ 보고서 스냅샷을 대시보드/표·두 출력기가 공유하며 설정은 `.jobs` 입력을 바꾸지 않는다. Qt Gui는 Widgets 의존 모듈이므로 별도 Qt 패키지 설치가 필요하지 않다.
[대시보드·보고서·출력 계약](reports-export.md)에 필터와 집계 범위·용지/글자 크기·한도·후속 범위를 기록한다.

로컬 Release 전체 14/14, Windows 네이티브 보고서 UI와 출력 시험을 통과했다. openpyxl/pypdf로 ZIP/숫자/미정/출처, PDF 크기·본문·긴 행 700구간과 마지막 표식을 독립 대조했다. 엑셀 14개 시트와 PDF 페이지를 렌더링해 한글/줄바꿈/정렬을 확인했다. 독립 재열기 검사 스크립트는 `scripts/verify-report-fixtures.py build`이며 Python 의존성은 개발/CI에만 사용한다. Windows/macOS CI에서도 CTest 뒤 같은 파일 검증을 실행한다.

자동 저장/복구·실행 취소 증분은 `undo_contract`, `recovery_crash_file`, `recovery_ui`를 더해 전체 17개로 확장했다. 실제 별도 프로세스 강제 종료와 flush 이후 commit 직전 강제 종료를 포함한다. 일반 입력은 새로운 revision으로 취소/재실행하고, 승인·제출 사실은 불변 이력 경계로 유지한다. 저장 입력과의 동일성(dirty)과 실제 저장된 revision을 구분해 보고서 출처도 보정했다.
[자동 저장·복구·편집 이력 계약](recovery-undo.md)에 저장 위치/간격·사본 복구·잠금/세대·중단·한도·후속 검증을 기록한다. `.jobs`는 schema 6이며 복구 envelope는 별도 schema 1이다. 회귀 시험용 저장 단계 observer는 실제 flush/commit 사이에서 중단·경쟁을 재현하며 제품 UI에서는 설정하지 않는다.

후속 PDM/작업 달력 증분은 `.jobs` schema 7을 사용하며 schema 1~6 원본과 기존 기준선을 보존한다. 복구 envelope schema 1의 내부 projectSchema 6/7을 지원한다. `native-pdm-taskcal-v1`은 civil 분 좌표와 작업별 근무분을 구분하며 `jobs-pdm-1.0`으로 계산한다. 자원/원가는 같은 근무 구간을 사용한다. 기존 `native-fs0-workminutes-v1` / `jobs-cpm-0.2` 결과는 유지한다. [PDM·작업 달력 계약](pdm-calendars.md), `pdm_calendar_contract`/`pdm_calendar_ui`와 독립 reader를 추가해 CTest 19개로 확장했다.

제약·마감·간트 증분은 schema 8 / native-pdm-constraints-v1 / jobs-pdm-1.1을 사용하며 구 입력/기준선 정책을 보존한다. Constraints는 하드 충돌과 소프트 마감 경고를 구분하고, GanttEditing은 UI 미리보기 후 Document의 ID/revision guard와 단일 edit를 사용한다. [상세 계약](constraints-gantt.md), constraint_contract/constraint_gantt_ui로 CTest 21개와 독립 XLSX/PDF 검증을 제공한다.

공수·작업 유형·자원 달력·분할 증분은 schema 9 / native-effort-split-v1 / jobs-effort-1.0이다. TaskExecution의 달력 교집합/공수 적분·마지막 분 배분을 일정·부하·자원비·출력이 공유한다. [계산/편집/파일 계약](effort-calendars-splits.md), effort_work_calendar_split/effort_ui를 포함해 CTest 23개로 확장하고 새 공수 보고서의 19개 XLSX 시트와 PDF를 독립 reader로 검증한다. schema 1~8의 원본 보존 SaveAs와 이전 계산 엔진, projectSchema 6~9 복구를 유지한다.


성능·통합 검증 증분은 PRD PERF-01~04를 구현 전에 커밋하고 계산 진행/협력 취소, 요청/session 보호, 동일 스냅샷의 달력/공수 격자 공유와 화면 주변 간트를 추가했다. `calculation_cancellation`, `integrated_project_workflow`, `performance_cancellation_ui`로 CTest 26개를 구성한다. 로컬 26/26·Windows 네이티브 새 UI·독립 XLSX 5/PDF 6 검사를 통과했다. [측정 계약·원시 결과·한계](performance-integration.md)를 따르며 CI는 1천/1만 기본·공수 사례를 별도 프로세스에서 실행해 JSON/log artifact를 남긴다. 파일 schema/기존 계산 정책은 유지한다.


작업 편집 증분은 PRD EDIT-01~05를 먼저 정의한 뒤 `TaskEditing`의 Qt 독립 원자적 명령과 표 입력/TSV 공통 파서, ID 기반 검색/필터/정렬/선택을 구현했다. `task_editing_contract`, `task_editing_ui`로 CTest 28개이며 로컬 전체 28/28·네이티브 Windows UI·독립 XLSX/PDF 검사를 통과했다. [작업 편집 계약](task-editing.md)에 삭제 보호·복제 범위·붙여넣기 한도·완료 증거와 후속 범위를 기록한다.

Project XML 증분은 PRD XML-01~04를 먼저 커밋하고 `persistence/ProjectXml`의 제한 파서·계획 투영·원본 보존 교환 폴더와 `MainWindowXml` 미리보기를 추가한다. 기존 .jobs schema 9와 계산 코어는 유지한다. `project_xml_contract`, `project_xml_ui`를 포함해 CTest 30개다. 도메인 기대값/지원 부분 왕복/원본 바이트 및 승인·실적·원가 사본, XML 한도/오류, UI 취소/오래된 미리보기/읽기 전용 내보내기를 검사한다. `python scripts/verify-project-xml.py build`는 표준 XML reader의 독립 값 검사이며 실제 Microsoft Project 검증을 대체하지 않는다. `python scripts/capture-project-xml-ui.py build`는 네이티브 UI 동작과 미리보기 PNG를 기록한다. CI에서 기존 XLSX/PDF·1천/1만 성능 검증과 함께 실행한다. [지원 범위·손실·사용법](project-xml.md)을 따른다.

Primavera XER 증분은 PRD XER-01~04를 먼저 정의하고 `persistence/PrimaveraXer`의 제한된 탭/structured calendar 파서·기간 기반 계획 투영과 `MainWindowXer`의 인코딩/변환 미리보기를 추가했다. XML과 XER의 원자적 교환 폴더 게시를 `ExchangePackage`에서 공유한다. 파일 schema/계산 코어는 유지하며 `primavera_xer_contract`/`primavera_xer_ui`로 CTest 32개다. 소수 시간의 정수 분 변환/1분 오차·지연/24시간 달력·상속 휴일·가용량 미정/0·UTF-8/Windows-1252·승인/실적/원가 사본·오류/한도/취소/session 경계를 검사한다. 독립 검사는 Python 3.12+, `mpxj==16.10.0`, `JPype1==1.6.0`, Java 21을 설치하고 `python scripts/verify-primavera-xer.py build`로 실행한다. MPXJ의 공식 UniversalProjectReader proxy를 통해 BOM 인식과 strict read를 함께 적용하며 실제 파일을 수정하지 않는다. `python scripts/capture-primavera-xer-ui.py build`는 네이티브 UI 시험/미리보기 PNG를 기록한다. 앱 실행에는 Python/Java가 필요 없다. [형식·원본 보존·제한](primavera-xer.md)을 따른다.

작업 표 모델/비동기 편집 증분은 PRD MODEL-01·ASYNC-EDIT-01~03·MODEL-PERF-01을 구현 전에 커밋했다. TaskTableModel/TaskTableView는 14열과 ID 기반 보기/선택을 유지하며, AsyncEditController와 Document의 봉인된 준비/반영 API가 공수·실적·배정 계산을 격리한다. 새 prepared_edit_contract/task_table_model_contract/async_editing_ui로 CTest 37개다. 1만 공수 작업에서 실행 중/완료 직전 취소, 최신 요청·문서 재열기·저장/F9 순서, 대화상자 초안/재시도와 창 파괴를 검사한다. 네이티브 검사는 `python scripts/capture-model-async-ui.py build`이며 PNG와 관측 JSON을 남긴다. [모델·비동기 편집 계약과 검증](model-async-editing.md)을 따른다.

대규모 편집 응답성 증분은 PRD RESPONSIVE-01~04를 먼저 확정하고 내부 ProjectState의 owning read-only snapshot, worker의 정확한 dirty 준비와 일반 기간 단일 작업 delta, 연속 변경의 모델/WBS 증분 갱신을 추가했다. F9/공수 미리보기/기준선 준비/자동복구도 공유 입력을 캡처한다. responsive_document_contract/responsive_editing_ui를 포함해 CTest 39개를 구성하며, 상세 범위와 전후 원시 관측은 [대규모 편집 준비·dirty·화면 갱신](responsive-editing.md)을 따른다.


구조·복합 편집 증분은 PRD BULK-01~04를 먼저 확정하고 작업 추가/삽입·삭제·복제·모든 TSV 붙여넣기와 해당 복합 undo/redo의 해석/검증/표시 준비를 worker로 연결했다. 현재 계획만 검증하는 내부 경계와 준비된 BulkView, 기본 보기의 행 추가/삭제 및 변경 범위 신호, 공유 delta·선택 범위·기준선 캐시를 사용한다. `bulk_editing_contract_ui`를 포함해 CTest 40개다. [구조·복합 편집 계약과 측정](bulk-editing.md)을 따른다.


필터·정렬·WBS 대량 편집 증분은 PRD VIEWBULK-01~04를 선반영한 뒤 공통 TaskViewProjection/worker 준비와 ID 기반 모델 행·layout 갱신을 추가했다. 새 셀 초안은 반영 전에 확정하여 stale 결과가 덮지 않게 한다. projected_views_ui로 CTest 41개이며 [준비된 보기 계약·전후 관측](projected-task-views.md)을 따른다.

외부 파일 검증 증분은 PRD PARSER-SAFE-01~04를 먼저 확정하고 실제 native 메모리 decoder와 XML/XER import에 공유 harness를 추가했다. `parser_seed_replay`를 포함해 CTest 42개이며, 기본 빌드에서는 56개 정상/손상 seed를 재생한다. 별도 Linux/Clang CI는 선택적 ASan/UBSan 전체 회귀와 libFuzzer 세 타깃의 유한 실행을 수행한다. [명령·경계·검증 결과](parser-safety.md)를 따른다.
