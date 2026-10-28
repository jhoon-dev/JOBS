# C++ 포맷·정적 분석·편집 가드

2026-10-28 사용자 요청. MAINT-01~04를 구현 전에 확정한다. 포맷은 clang-format 18.1.8, 정적 분석은 Clang/clang-tidy 18과 Qt 6.8.3을 사용한다. 기본 앱 빌드와 실행에는 이 도구가 필요하지 않다.

포맷 적용 시작점은 Document.h/.cpp·TaskEditing.cpp·PreparedEdit.cpp다. 등록 파일은 이후에도 검사하고 새로 추가/수정한 src/tests C++ 파일도 같은 규칙을 적용한다. 이미 구현된 나머지 코드를 한 번에 재포맷하지 않는다. 읽기 전용 검사와 명시적 fix를 구분하며 포맷 커밋을 의미 변경과 분리한다.

정적 분석은 실제 compile_commands에서 전체 src 번역 단위를 골라 수행한다. 오류 중심 check 집합과 버전을 고정하고 선택 진단·컴파일 실패는 실패로 남긴다. Qt 사전 빌드 내부나 모든 종류의 정적 분석을 완료했다고 주장하지 않는다. 재현 명령·목록·진단·실패 경로 검사와 전체 회귀 결과를 후속 절에 기록한다.

Document의 문서 존재/읽기 전용 조건은 private helper로 공유한다. helper는 ProjectState를 쓰거나 복사하지 않는다. 기존 일반 편집 거부 문구와 특수 흐름의 별도 문구/조건을 보존한다. 읽기 전용 사본 저장과 조회는 편집 권한과 다르므로 유지한다. 실패는 입력·이력·계산·파일·ID·snapshot에 부작용을 남기지 않아야 한다.

기준 도구 문서: [clang-format 스타일](https://clang.llvm.org/docs/ClangFormatStyleOptions.html), [clang-tidy 설정·컴파일 데이터베이스](https://clang.llvm.org/extra/clang-tidy/).
