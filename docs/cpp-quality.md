# C++ 포맷·정적 분석·편집 가드

2026-10-28 사용자 요청. MAINT-01~04를 구현 전에 확정한다. 포맷은 clang-format 18.1.8, 정적 분석은 Clang/clang-tidy 18과 Qt 6.8.3을 사용한다. 기본 앱 빌드와 실행에는 이 도구가 필요하지 않다.

포맷 적용 시작점은 Document.h/.cpp·TaskEditing.cpp·PreparedEdit.cpp다. 등록 파일은 이후에도 검사하고 새로 추가/수정한 src/tests C++ 파일도 같은 규칙을 적용한다. 이미 구현된 나머지 코드를 한 번에 재포맷하지 않는다. 읽기 전용 검사와 명시적 fix를 구분하며 포맷 커밋을 의미 변경과 분리한다.

정적 분석은 실제 compile_commands에서 전체 src 번역 단위를 골라 수행한다. 오류 중심 check 집합과 버전을 고정하고 선택 진단·컴파일 실패는 실패로 남긴다. Qt 사전 빌드 내부나 모든 종류의 정적 분석을 완료했다고 주장하지 않는다. 재현 명령·목록·진단·실패 경로 검사와 전체 회귀 결과를 후속 절에 기록한다.

Document의 문서 존재/읽기 전용 조건은 private helper로 공유한다. helper는 ProjectState를 쓰거나 복사하지 않는다. 기존 일반 편집 거부 문구와 특수 흐름의 별도 문구/조건을 보존한다. 읽기 전용 사본 저장과 조회는 편집 권한과 다르므로 유지한다. 실패는 입력·이력·계산·파일·ID·snapshot에 부작용을 남기지 않아야 한다.

기준 도구 문서: [clang-format 스타일](https://clang.llvm.org/docs/ClangFormatStyleOptions.html), [clang-tidy 설정·컴파일 데이터베이스](https://clang.llvm.org/extra/clang-tidy/).

## 로컬 실행

포맷터는 `python -m pip install -r scripts/cpp-quality-requirements.txt`로 설치한다. 프로젝트 내부 venv를 써도 된다. 실행 파일이 PATH에 없으면 `--tool`로 절대/상대 경로를 지정한다.

```sh
python scripts/check-cpp-quality.py format
python scripts/check-cpp-quality.py format --base origin/main
python scripts/check-cpp-quality.py format --base origin/main --fix
```

기본 검사는 `.clang-format-files`의 등록 파일과 현재 staged/unstaged/untracked src/tests C++ 파일을 검사한다. `--base`는 지정 commit부터 HEAD까지의 추가/수정/이름 변경도 포함한다. CI는 push 이전 SHA 또는 PR base SHA를 사용한다. 삭제 파일은 diff 대상에서 제외하지만 등록 목록의 잘못된/사라진 파일은 실패다. 경로는 src/tests 아래 실제 파일로 제한하며 저장소 밖 경로는 거절한다. 기본 검사는 파일을 쓰지 않고 명시적 `--fix`만 바꾼다.

`.clang-format`은 LLVM 기반·4칸·120열·LF를 사용하고 짧은 if/loop를 한 줄로 합치지 않는다. include 순서는 바꾸지 않는다. formatter 버전은 정확히 18.1.8을 검사한다. 아직 등록/수정하지 않은 레거시 파일의 전면 포맷은 후속 수정 때 수행한다.

정적 분석은 Linux의 Clang 18/Qt 6.8.3 개발 환경을 기준으로 한다. 일반 Release 빌드와 별도 디렉터리를 사용한다.

```sh
CC=clang-18 CXX=clang++-18 cmake -S . -B build-quality \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_NO_SYSTEM_FROM_IMPORTED=ON
python scripts/check-cpp-quality.py tidy --build-dir build-quality --jobs 2
python scripts/test-cpp-quality.py --tidy-tool clang-tidy-18
```

clang-tidy는 major 18을 확인하고 실제 세부 버전과 명령을 기록한다. `.clang-tidy`의 clang-analyzer 전체, infinite-loop/suspicious-semicolon/use-after-move 및 예외 전달 check만 활성화한다. 선택 진단은 오류로 취급하고 일반 tidy의 header filter는 src를 대상으로 한다. analyzer 진단은 이 filter를 우회할 수 있으므로 헤더라는 이유만으로 사후 제거하지 않는다. src의 모든 cpp가 실제 compile database에 존재해야 하며 누락은 실패다. 각 번역 단위는 최대180초, 동시 작업 기본2개/최대4개다. `build-quality/cpp-quality`에 파일별 진단·명령/반환 코드·상태 JSON을 보존한다. 분석은 `--fix`를 호출하지 않는다.

분석용 CMake의 `NO_SYSTEM_FROM_IMPORTED`는 Qt include 경로를 `-isystem` 대신 `-I`로 전달한다. Clang 18이 system-header의 custom delete를 두 번 해제로 모델링하는 [LLVM #62985](https://github.com/llvm/llvm-project/issues/62985)의 공식 우회 방법이다. 경로/타입은 실제 Qt 구성 그대로이며 검사 집합이나 오류 정책을 끄지 않는다. 일반 제품 빌드의 include 설정은 유지한다.

도구 검사는 임시 Git 저장소에서 format 실패 시 바이트 보존, 명시 수정/재검사, commit/staged/untracked 대상 선택, 저장소 밖/사라진 등록 경로 거절을 확인한다. Linux에서는 실제 analyzer가 null 역참조를 거부하고 수정 후 통과하는지, compile database 누락도 거부하는지 검사한다. 이 임시 결함은 제품에 링크하거나 실행하지 않는다.

## 공통 가드

`Document::isEditable() const`는 문서 존재와 readOnly만 읽는다. `requireEditable() const`는 기존 일반 메시지를 그대로 던지며 표/구조 변경·준비 요청의 30개 동일 가드를 공유한다. 실적/원가의 상태일, 간트의 유효 일정, 저장/자동복구와 prepared 결과의 stale 조건은 `isEditable()`와 별도 조건을 조합해 기존 문구·반환 정책을 유지한다. 조회·read-only 독립 사본 저장은 일반 편집 가드로 차단하지 않는다.

`document_edit_guards`는 35개 진입점 각각을 문서 없음/읽기 전용에서 호출한다. 잘못된 row/이름/입력을 함께 줘 권한 거부 우선순위를 검증하고 Project 입력/revision·session·dirty·undo/redo·계산·공유 root 포인터·이벤트·파일/ID 접근이 변하지 않는지 검사한다. 같은 문서 ID/revision의 준비 결과도 읽기 전용 대상에 적용되지 않아야 한다. 정상 편집/undo/redo·상태일 거부·읽기 전용 preview/독립 사본 저장도 확인한다. 새 검사 포함 CTest는 43개다.


## 최종 검증

PRD MAINT-01~04는 `a999d59`에서 먼저 커밋했다. 제품 C++의 포맷만 바꾼 커밋은 `c231170`(편집 핵심4파일)과 `e2106f2`(분석 중 확인한 UI2파일)이며 의미 변경은 `746a448`·`3250ad7`에 분리했다. 새 guard 검사까지 이 증분에서 포맷한 C++ 파일은 7개이고, 기존 파일 등록 목록은 6개다.

첫 전체 분석 `40643042856`의 59개 번역 단위 중 9개에서 선택 진단 12개가 발생했다. Qt QWeakPointer/custom-delete·QObject parent 소유권 모델 진단과 콜백 진입/이동 후 cache 상태를 검토했다. 이동한 assignments cache를 명시적으로 clear하고 완료 콜백 진입에 생존 검사를 추가했다. Qt system-header 모델은 위 LLVM 권고에 따른 분석용 include 분류로 처리했다. `.clang-tidy` 검사/오류 정책 SHA-256은 전후 동일하고 새 NOLINT나 진단 후처리 허용 목록은 없다. [첫 진단과 처리 기록](validation/cpp-quality-2026-10-28/first-analysis/review.json)을 보존한다.

최종 소스 `f3316f1`의 [품질 CI 40644974152](https://github.com/jhoon-dev/JOBS/actions/runs/40644974152)는 clang-format 18.1.8, 도구 경계 검사4개와 Clang/clang-tidy 18.1.3의 **전체 src 59개 번역 단위**를 통과했다. 실제 compile_commands의 Qt include 345개가 `-I`임을 확인했고 각 분석 반환 코드는 모두 0이다. 이 결과는 선택 검사 집합의 통과이며 모든 clang-tidy rule이나 모든 플랫폼 조건부 코드를 검사했다는 뜻은 아니다.

[제품 CI 40644964939](https://github.com/jhoon-dev/JOBS/actions/runs/40644964939)는 Windows/MSVC·macOS 각각 **43/43**, 기존 native projected/bulk/model/async/responsive/XML/XER와 독립 XLSX 5·PDF 6·Project XML·XER TSV/MPXJ 판독을 통과했다. 1천/1만 기본·공수 일정 해시 8개가 기존 golden과 일치했다. [sanitizer CI 40644969279](https://github.com/jhoon-dev/JOBS/actions/runs/40644969279)도 **43/43, 137.41초**, 기존 seed56개와 세 parser 총15,000회 유한 퍼징을 통과했다. 로컬 MinGW Release는 동일 제품/테스트 소스로 **43/43, 144.48초**·7파일 읽기 전용 format·도구 검사3개(실제 tidy canary는 Linux에서 검사)·독립 reader를 통과했다.

[검증 묶음](validation/cpp-quality-2026-10-28/verification.json)에 commit/CI·OS별 시간·계산 해시를 기록했다. quality 하위 폴더는 모든 번역 단위의 진단 원문/명령/반환 코드·CMake cache와 compile database를, first-analysis는 초기 실패를, local/Windows/macOS/sanitized는 각 회귀·parser 명령/로그·실제 corpus 압축본과 SHA-256을 보존한다. 최종 회귀 결과가 초기 실패와 섞이지 않게 폴더를 구분했다.
