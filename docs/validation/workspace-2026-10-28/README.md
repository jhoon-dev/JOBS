# 구조화 진단·다중 프로젝트 작업공간 검증

제품 소스 `a2039fed0140c02bc8051d7cb3c67a3655056815`, 제품 소스 SHA-256 `8e04b4757d8b9463daf29d0e58bb99b3a24e36f4f82948d7a189312a3cd3a846`의 기록이다. PRD `b872750` 및 메모리 디코더 퍼징 수용 `68bfe59`를 먼저 커밋하고 개발했다. 기존 파일의 서식 변경은 `d86c90d`·`4b5cf7b`로 분리했다. 기능 구현 후 주기 갱신과 새 계산 간 취소 경합, 공유 부하 안내 분류를 회귀 테스트와 함께 수정했다.

| 검증 | 결과 | 증거 |
|---|---|---|
| 로컬 MinGW Release | 48/48, 135.06초 | [CTest](local/ctest.log), [상세 출력](local/LastTest.log) |
| macOS Release | 48/48, 64.73초; 전체 workflow 성공 | [작업 로그](macOS/job.log), [화면·조작](macOS/workspace/) |
| Windows MSVC Release | 48/48, 85.67초; 전체 workflow 성공 | [CI](https://github.com/jhoon-dev/JOBS/actions/runs/40698951285) |
| Linux ASan/UBSan | 48/48, 180.45초 | [CTest](sanitized/ctest.log), [계측 확인](sanitized/instrumentation.json) |
| 정적 분석 | 실제 제품 TU 67개 모두 returncode 0 | [clang-tidy 명령·결과](quality/cpp-quality/tidy.json) |
| 서식 | 기준 revision부터 바뀐 C++ 포함, 변경 필요 0건 | [로컬 전체 검사](local/format.json) |
| 파서 | `.jobs`·XML·XER·작업공간 각 5,000회, 총 20,000회 | [원시 명령·종료 코드](sanitized/parser-fuzz/run.json) |
| 기존 파일 | 이전 writer SHA 고정 파일 13개, schema1~9, 402개 계약 검사 | [호환성 출력](local/enum-compatibility.log) |
| 외부 reader | XLSX 5개/PDF 6개, XML, XER 독립 TSV 및 MPXJ 16.10.0 통과 | [로컬 로그](local/readers.log), 플랫폼 CI 로그 |
| Windows 설치본 | 파일 76개 SHA, 재설치·개발 PATH 없는 실행·전체 제거 통과 | [설치·제거 기록](local/installer-smoke.json), [payload](local/package-manifest.json) |

[기계 판독 요약](verification.json)은 source revision·설치 hash·CI run·일정 hash를 연결한다. 취소/입력 수정은 부분 결과를 적용하지 않으며, 새 계산 시작 직후 주기 갱신을 강제로 발생시키는 회귀를 포함한다. 공유 자원은 수동 기대값과 24개 분 단위 독립 oracle, 다른 프로젝트의 동일 로컬 ID·시작일·달력·분할·잔여 공수를 확인했다.

로컬 1천/1만 작업의 legacy/effort 일정 hash 4개는 기존 검증과 일치한다. macOS·Windows 동일 4개씩도 일치해 총 12개 관찰값을 확인했다. 편집 측정은 baseline 0/3개 각각에서 100단계 undo를 확인했고 변경 이력 추정량은 20,570 bytes다. 개별 샘플과 중앙값을 [로컬](local/), [macOS](macOS/performance/), [Windows](Windows/performance/)에 보존한다. 시간은 해당 실행 환경의 관찰값이며 일반적인 응답 시간 보증이 아니다.

최종 Windows 설치 파일은 `dist/JOBS-0.1.0-Setup-win64-a2039fed.exe`, 12,992,450 bytes, SHA-256 `c3a74e1c96c325475759e2c8bd6fdb6aa3fc1115f4c747c5db6c01895bb899de`다. 미서명 preview이며 GitHub Release로 공개하지 않았다. 설치 폴더와 앱 설정/캐시/복구/바로가기는 전체 제거됐고 외부 고정 `.jobs` 파일과 다른 앱 데이터는 보존됐다.

설치본 직접 클릭 검증은 Windows 화면 자동화 접근 거부로 수행하지 못했다. 로컬 네이티브 Qt 작업공간 테스트와 플랫폼 CI의 화면/조작 검증을 설치본 직접 클릭 결과와 구분한다. 이 세션은 기존 실제 사용자 데이터가 없는 상태에서만 설치 helper를 실행했고 테스트 전의 빈 설정 key를 복원했다. 상세 범위는 [설치 안내](../../windows-installer.md)를 따른다.

영문은 진단 기반이며 일부 코드에는 공통 안내를 사용한다. 전체 UI 번역, 교차 프로젝트 선행관계, 전역 평준화와 서버 협업은 포함하지 않는다. 퍼징은 위 입력 크기/횟수로 제한된 smoke 검사이며 완전한 호환성·보안 보증이 아니다. 원시 corpus는 해당 CI artifact에 보존하며 저장소에는 재현 명령·로그를 둔다.
