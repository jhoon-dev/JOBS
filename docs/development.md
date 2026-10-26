# C++/Qt 초기 구현

2026-10-26 사용자 요청에 따른 구현 착수. PRD-01(초안 입력 일부), PRD-02(WBS·표·간트 일부), PRD-03(기본 FS CPM), PRD-04/05(기준선·실적·편차 일부), PRD-08,
DESK-01/04/05 및 FILE-01/02/03/05/07/09의 초기 수직 흐름이다.
전체 PRD나 FILE-T01~09의 완료 선언은 아니다.

## 구성

- `jobs_domain`: 표준 C++20 프로젝트·작업·WBS·기준선·실적 원장·자원/배정·불변 시나리오·날짜·revision 검증 및 계층 탐색. Qt 의존 없음.
- `jobs_calculation`: 표준 C++20 FS0 CPM, 달력, retained remaining 예측, 기준선 준비/검증, 계획/예측 비교, 자원 수요/가용량 및 비분할 직렬 평준화. Qt 의존 없음.
- `jobs_application`: 문서 세션, 생성/열기/편집/저장/사본, 추상 저장 포트.
- `jobs_persistence`: Qt Core JSON, checksum, 잠금, 외부 변경 감지, 원자적 교체.
- `jobs_reporting`: Qt Gui 기반 숫자/텍스트 표시·공통 추이, 공개 OOXML XLSX와 페이지를 나누는 PDF 출력. 외부 런타임/Office 라이브러리 없음.
- `jobs_ui`: Qt Widgets WBS·표·간트, 기준선/상태일/실적 대화상자, 자원 등록/배정·비동기 부하/시나리오 A/B 비교, 변경 상세, 미저장 변경 확인.

기간 미정(null)과 마일스톤(0분)을 구분한다. 기간·FS 선행·휴일을 입력하면 F9로
기간 중심 논리 일정을 계산한다. F9는 자원과 독립인 논리 일정이다. 자원 메뉴에서 별도로 배정·일별 부하와 평준화 시나리오를 비교한다.
기본 달력은 월~금 09–12/13–18이며 날짜는 시간대 없는 ISO civil date다.
WBS는 별도 그룹 트리로 편집하며 작업을 그룹에 배치한다. 간트에 최신 계산 결과·마일스톤·FS 연결선을 표시한다.
추적 메뉴에서 기준선을 새 버전으로 저장하고 상태일·실제 시작/완료·실제 소요/잔여 근무분을 입력한다.
기준선은 불변 입력·달력·결과 스냅샷, 실적은 정정 사유를 가진 append-only 보고 원장이다.
현재 계획과 상태일 이후 잔여 예측을 나누어 비교하며, 간트에서 선택 기준선을 회색으로 표시한다.
간트 드래그 편집·FS 외 관계/lag·공수/물리 진척·undo/redo는 후속 범위다.
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
원본 bytes를 보존한다. 일반적인 다단계 이관·자동복구 checkpoint·백업은 아직 없다.
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
