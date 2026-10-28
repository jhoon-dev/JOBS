# 계획 enum 전환 검증

PRD `06d4580`의 ENUM-01~04, 포맷 분리 `2cab025`, 최종 코드 `d35712898d432154098d21994d99ce5440fcf657`을 검증했다. `verification.json`이 실행 ID·상태·시간·기존 일정 hash 대조를 묶는다.

- `local/`: 최종 증분 빌드, 전체 CTest/402개 enum 검사, 전체 변경 파일 포맷 검사, 독립 reader, 일정 probe 4개. probe는 hash 목적 1회 실행이다.
- `Windows/`, `macOS/`: 전체 회귀/reader 검증 로그 및 1천·1만 기본/공수 일정 JSON. 8개 hash를 이전 `../cpp-quality-2026-10-28/Windows/` 증거와 대조했다.
- `quality/`: 실제 compile database와 CMake 설정, format JSON, 59개 src의 clang-tidy 명령·반환·원시 로그.
- `sanitized/`: Linux ASan/UBSan 전체 CTest, 실제 의존 라이브러리 계측 확인, 도구 버전, seed 재생과 각 parser 5천 회의 명령·로그·corpus. 많은 작은 corpus 파일은 zip으로 보존했다.
- `*-ci.json`: 같은 최종 코드의 세 workflow 전체 성공 및 단계 정보.

이전 writer bytes와 provenance는 `tests/fixtures/enum-compat/manifest.json` 및 그 디렉터리에 별도로 고정되어 있다. native schema9의 전체 payload를 이전 writer와 비교하고, schema1~9 파일을 읽기 전용으로 연 뒤 독립 사본 저장/재열기와 원본 보존을 검사했다. 재현 명령과 타입/교환 변환표는 `docs/plan-enums.md`를 따른다.
