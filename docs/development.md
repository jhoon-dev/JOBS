# C++/Qt 초기 구현

2026-10-26 사용자 요청에 따른 구현 착수. PRD-01(초안 입력 일부), PRD-08,
DESK-01/04/05 및 FILE-01/02/03/05/07/09의 초기 수직 흐름이다.
전체 PRD나 FILE-T01~09의 완료 선언은 아니다.

## 구성

- `jobs_domain`: 표준 C++20 프로젝트·작업·날짜·revision 검증. Qt 의존 없음.
- `jobs_application`: 문서 세션, 생성/열기/편집/저장/사본, 추상 저장 포트.
- `jobs_persistence`: Qt Core JSON, checksum, 잠금, 외부 변경 감지, 원자적 교체.
- `jobs_ui`: Qt Widgets 창, 메뉴/단축키, 초안 생성, 작업 표, 미저장 변경 확인.

계산 엔진은 아직 구현하지 않았다. 기간은 미산정, 선행/자원은 미설정으로 표시한다.
기본 달력은 월~금 하루 8시간이며 날짜는 시간대 없는 ISO civil date다.
WBS·간트·의존관계·휴일 편집·기준선·실적·undo/redo는 후속 범위다.

## 빌드와 실행

초기 개발 기준: CMake 3.24+, C++20 컴파일러, Qt **6.8.3 Core/Widgets**.
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
GitHub Actions는 Windows/macOS 빌드와 파일 테스트를 수행한다.

## 초기 `.jobs` 형식 (schema 1)

UTF-8 JSON 한 파일에 `format: JOBS`, `schemaVersion: 1`, `payload`, `sha256`을 보존한다.
payload에는 documentId, derivedFrom, revision, name, startDate, calendar, tasks가 들어간다.
각 작업은 UUID와 이름을 갖는다. checksum은 Qt의 compact JSON payload에 대한 SHA-256이며
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
자동복구 checkpoint·백업·이관은 아직 없다. 미지원 버전은 원본을 수정하지 않는다.

## 검증 범위

`jobs_file_tests`는 실제 임시 파일로 생성/편집/revision/재열기, Unicode 경로·데이터,
사본의 ID/계보·원본 bytes 보존, 중복 writer 잠금, 읽기 전용 사본, 외부 변경 거절,
JSON 손상/checksum/신버전/알 수 없는 필드/중복 ID, 열기·저장 실패 시 세션 보존,
단일 파일 복사를 검사한다. 이는 FILE-T01의 계산을 제외한 부분과 T02/T06/T08/T09 일부에 해당한다.

GUI 수동 점검: 새로 만들기→작업 목록 입력→저장→작업명 편집→열기/종료 시
저장·버리기·취소→다른 이름 저장→원본 재열기. 두 앱으로 같은 파일 열기와 읽기 전용 사본도 확인한다.
실제 OS 간 전달, 강제종료·디스크 부족·장치 제거, 접근성/IME/대용량 성능 시험은 미완료다.

`jobs_ui_tests`는 offscreen Qt Widgets 환경에서 새 프로젝트 대화상자·작업 목록 입력·표 편집·
저장 대화상자·종료 취소·종료 시 저장을 자동 검사한다. 네이티브 파일 대화상자와 실제 데스크톱
화면의 수동 검증을 대신하지 않는다.

## 실행 증거 · 2026-10-26

Windows x64, Qt 6.8.3, MinGW GCC 13.1.0, CMake 4.4.4, Ninja에서
`scripts/dev.ps1 -TestOnly`로 Release 빌드 성공. CTest `file_lifecycle`와
`ui_lifecycle` 모두 통과(2/2). macOS 및 MSVC 검증은 GitHub Actions에서 별도 추적한다.
