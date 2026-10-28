# 선행관계·제약·작업 유형의 강한 타입

2026-10-28 사용자 요청. ENUM-01~04를 구현 전에 확정한다. 기존 파일 형식과 일정 의미를 유지하는 내부 enum class 전환이다.

대상은 관계4개·제약8개·작업 유형3개다. 도메인 필드와 편집 API/비동기 명령, 계산 및 간트 모델에 enum을 사용한다. 관계의 시작/완료 endpoint는 문자열 위치 대신 명시적 의미 함수로 구분한다. 잘못된 정수 cast도 도메인 검증에서 거부한다. 자원 종류·정책 ID·현지화는 별도 후속 범위다.

파일은 schema1~9의 기존 canonical token을 유지하며 writer는 계속 schema9다. 기준선·시나리오·복구에 포함된 입력도 같은 변환을 공유한다. Qt 없는 formatting 경계 helper가 enum↔token을 담당하고 지원되지 않는 token에 기본값을 부여하지 않는다. UI의 기존 소문자 선행관계 허용/콤보 token, XML 숫자 코드와 XER PR_/CS_ 코드는 경계에서 명시적으로 매핑한다. enum ordinal을 저장 코드로 사용하지 않는다.

타입 변경 전에 이전 writer/seed 도구로 만든 fixture를 고정하고 SHA와 provenance를 남긴다. 새 reader가 이를 읽어 기본값·타입·전체 payload를 보존하는지 확인하고, 현재 writer만으로 만든 roundtrip을 이전 파일 호환의 유일한 증거로 삼지 않는다. 원본 파일은 수정하지 않으며 새 파일 저장 뒤 재열기·입력 이력·독립 reader·일정 hash·정적/동적 검사를 함께 검증한다.

## 구현과 경계

`domain/PlanTypes.h`에 세 enum과 유효성/관계 endpoint 검사를 두었다. `Project`와 `PlanInput` 안의 Task/Dependency, Document 편집 API, EffortEdit, 계산 및 간트 행이 같은 타입을 사용한다. 이전 문자열 API를 우회 overload로 남기지 않는다. UI에서 문자열을 입력받는 곳과 직렬화/보고서에서만 `formatting/PlanTokens.h`를 사용한다. 이 helper는 Qt와 입출력 의존성이 없는 header다.

| 도메인 | 기존 `.jobs`/보고서 token | 외부 교환 |
|---|---|---|
| DependencyType::FS/SS/FF/SF | FS/SS/FF/SF | XML FF=0, FS=1, SF=2, SS=3; XER PR_FS/PR_SS/PR_FF/PR_SF |
| ConstraintType::ASAP/ALAP/SNET/SNLT/FNET/FNLT/MSO/MFO | 같은 대문자 문자열 | XML ASAP=0, ALAP=1, MSO=2, MFO=3, SNET=4, SNLT=5, FNET=6, FNLT=7; XER 기존 CS_ 매핑 유지 |
| TaskType::FixedDuration/FixedWork/FixedUnits | fixed-duration/fixed-work/fixed-units | 기존 XML/XER 기간 투영과 손실 보고 유지 |

외부 코드의 순서는 enum 선언 순서와 다르다. XML 숫자 변환은 명시 배열/조건으로 수행하며 ordinal을 cast하지 않는다. XER의 지원되는 별칭 및 미지원 값의 거부도 유지한다. UI 선행 입력은 기존 정규식과 대문자 변환 후 엄격한 decoder를 호출한다. 파일 decoder는 대소문자/공백/숫자/미지원 값을 자동 보정하지 않는다. 정책에 따라 원래 존재하지 않던 필드는 이전과 같이 FS/ASAP/고정 기간 기본값을 가진다.

## 호환 회귀

`tests/fixtures/enum-compat/manifest.json`은 이전 구현 f3316f1로 만든 13개 파일의 SHA-256과 출처를 고정한다. schema1~9 및 PDM/제약/공수 profile은 이전 parser-safety CI corpus에서 가져왔고, history.jobs는 enum 모델 수정 전에 이전 라이브러리에 연결한 별도 generator로 작성했다. 과거 generator 소스는 `.cpp.txt`로 보관하며 현재 빌드에 포함하지 않는다. `.jobs` fixture는 Git 줄바꿈 변환에서도 제외한다.

`plan_enum_compatibility`는 이 bytes/SHA를 먼저 확인하고 새 decoder/store로 읽는다. schema9 파일은 이전 writer의 전체 payload와 새 encode 결과를 비교하므로 중첩 기준선/시나리오도 검사한다. 구 schema의 기본값/다른 이름 저장 및 재열기를 확인하고 원본 bytes를 보존한다. history에는 관계4개·제약8개·작업 유형3개, 승인 기준선·시나리오·공수 실적·원가가 모두 있다. 이력 불변성, undo/redo, 체크포인트/사본 복구, 잘못된 문자열/숫자/enum 거부와 실패 편집의 무변경도 검사한다.

재현 명령:

```powershell
cmake --build build --parallel 3
ctest --test-dir build -R plan_enum_compatibility --output-on-failure
ctest --test-dir build --output-on-failure
python scripts/check-cpp-quality.py format --base a4b1fa1 --tool .deps/cpp-quality/Scripts/clang-format.exe
```

수정 파일의 기존 포맷은 2cab025 커밋으로 먼저 분리했다. 타입 전환 후의 의미 변경과 리뷰에서 구분할 수 있다. 최종 실행 결과는 아래 검증 기록에 추가한다.
