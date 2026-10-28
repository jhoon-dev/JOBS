# 외부 파일 파서 검증

2026-10-28 사용자 후속 개발 요청에 따라 PARSER-SAFE-01~04를 구현 전에 확정한다. 파일 파서의 coverage-guided 입력 변이와 ASan/UBSan 런타임 검사가 이번 범위다. 일반 Release 옵션과 Windows/macOS 제품 지원 범위는 유지한다.

`.jobs`의 메모리 decode는 파일 저장·게시·잠금 없이 load와 저장 전 검증의 실제 envelope/해시/schema/도메인 검사를 공유한다. XML/XER harness도 실제 import 함수를 사용한다. 변이 시 checksum 때문에 native 본문 검증이 가려지는 문제는 별도 payload 경로로 보완하고 schema1~9에 동일 본문을 시도한다. envelope 원본 경로의 무결성 검사는 우회하지 않는다.

퍼징은 Clang에서 명시적으로 켜는 도구다. JOBS 코드에 coverage와 ASan/UBSan을 적용하고 사전 빌드 Qt 내부를 검사했다고 주장하지 않는다. 일반 테스트는 정상/손상 seed 재생, Linux CI는 계측된 전체 CTest와 3개 타깃의 유한 실행을 수행한다. 실행 수/seed·크기/시간/RSS 제한·revision·컴파일러·명령/반환 코드·로그·발견 입력을 기록한다. 정상적인 입력 거부만 허용하며 sanitizer/비정상 종료/시간 초과를 성공으로 바꾸지 않는다.

유한 smoke는 보안 보증이나 상용 제품 왕복 호환 판정이 아니다. Linux는 검증 실행기이며 신규 배포 플랫폼이 아니다. 퍼징 자체가 끝난 뒤에도 앱이나 사용자 파일을 지속 감시하지 않는다.

도구 계약은 [LLVM libFuzzer](https://llvm.org/docs/LibFuzzer.html), [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html), [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)를 기준으로 한다. 구현·명령·검증 결과는 후속 절에 기록한다.

## 실행과 경계

기본 CTest의 `parser_seed_replay`는 정상/잘린 입력·빈 입력/이진 입력, schema1~9 envelope와 raw payload, PDM/제약/공수, XML, XER UTF-8/Windows-1252를 포함한 56개 seed를 검사한다. 무결성 오류 envelope가 파일 경로에서 거절되더라도 별도 body 경로를 실행할 수 있음을 구분한다. 알 수 없는 필드, 64MiB 초과 native 입력, 메모리 decoder와 실제 load/save 일치, harness의 logic/cancellation 예외 전파도 확인한다.

`ProjectFile.h`의 `jobs::decodeProjectFile`은 실제 load와 저장 전 검증에서 호출한다. `ParserHarness`는 성공/일반 입력 거부를 세고 `std::runtime_error`만 처리한다. `bad_alloc`, `logic_error`/`out_of_range`, 별도 `CalculationCancelled` 및 프로세스 sanitizer 실패는 거부 결과로 바꾸지 않는다. 라이브러리 파서는 파일을 쓰지 않으며 seed 도구만 명시한 테스트 corpus/임시 파일을 쓴다.

Clang·Qt 6.8.3 개발 환경에서 실행한다(Windows/MinGW opt-in은 명시 실패한다).

```sh
cmake -S . -B build-sanitized -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DJOBS_ENABLE_SANITIZERS=ON -DJOBS_BUILD_FUZZERS=ON
python3 scripts/verify-parser-instrumentation.py
cmake --build build-sanitized --parallel 2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-sanitized --output-on-failure
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  python3 scripts/run-parser-fuzz-smoke.py --runs 5000
```

계측 옵션은 모든 JOBS 번역 단위에 ASan/UBSan·비복구 오류·프레임 포인터를 적용한다. domain/application/calculation/persistence/harness 다섯 정적 라이브러리에 `fuzzer-no-link`, 세 진입점에 fuzzer를 적용하고 compile_commands에서 누락을 검사한다. CMake는 두 옵션 모두 기본 OFF이며 fuzz만 켜고 sanitizer를 끈 구성은 거절한다. 계측되지 않은 사전 빌드 Qt 내부의 coverage를 주장하지 않는다.

`run-parser-fuzz-smoke.py`는 seed 재생 후 각 타깃에 `-runs=5000 -seed=20261028`, 64KiB 입력·입력당10초·RSS2048MiB·단일 할당1024MiB 제한을 적용한다. 타깃별 프로세스 한도는 180초이며 시간 초과/비정상 반환은 CI 실패다. native는 각 입력의 파일 envelope와 raw payload를 구분하고 9개 schema 본문 검증을 시도하며, XER는 두 인코딩을 시도한다. 요청 실행 수에는 libFuzzer의 corpus 처리도 포함되고, 고정 PRNG seed만으로 모든 머신의 실행 순서/coverage가 같다고 주장하지 않는다. 실제 corpus·명령·반환 코드·환경·revision과 libFuzzer 통계를 `build-sanitized/parser-fuzz`에 보존한다. 발견 입력이 있으면 `artifacts/<parser>/`의 파일을 해당 binary에 인수로 주어 재현한다.

```sh
build-sanitized/jobs_fuzz_jobs build-sanitized/parser-fuzz/artifacts/jobs/crash-SHA1
```

64KiB는 유한 smoke의 자원 예산이며 제품 파일 한도를 낮추지 않는다. 더 큰 입력·장기 퍼징·저장/게시/복구 I/O·ThreadSanitizer/동시성 검사는 이번 퍼징 경계 밖에 있다. 실제 사용자 프로젝트를 CI corpus에 넣지 않는다.


## 최종 검증

PRD/계약은 `4434568`에서 먼저 커밋하고 실제 decoder·harness·seed/CMake·runner는 `ea97d44`에서 구현했다. Linux CI 설치의 Qt 아키텍처 이름을 배포 목록 `linux_gcc_64`로 수정한 workflow는 `ec9de2e`다. 두 revision의 제품/CMake module/테스트/스크립트 소스가 동일함을 diff로 확인했다.

[제품 CI 40637538292](https://github.com/jhoon-dev/JOBS/actions/runs/40637538292)는 Windows/MSVC·macOS 각각 **42/42** 전체 회귀, native projected/bulk/model/async/responsive/XML/XER 검사, 독립 XLSX 5·PDF 6·Project XML·XER TSV/MPXJ 판독을 통과했다. 로컬 MinGW Release도 **42/42, 119.90초**와 독립 reader·미지원 sanitizer 옵션의 명시 실패를 확인했다. 두 OS의 1천/1만 기본·공수 일정 SHA-256 8개는 기존 golden과 일치했다.

[Linux CI 40637696271](https://github.com/jhoon-dev/JOBS/actions/runs/40637696271)는 Ubuntu 24.04·Clang 18.1.3·Qt 6.8.3·RelWithDebInfo에서 ASan/UBSan 전체 회귀 **42/42, 195.14초**를 통과했다. 컴파일 명령에서 확인한 번역 단위는 63개이며 다섯 의존 라이브러리의 coverage와 세 진입점을 검사했다. `detect_leaks=1`/비복구 오류 설정에서 56개 초기 seed와 각 타깃 **5,000회**, 총 **15,000회** 실행이 모두 0으로 종료했다. 이 유한 실행에서 crash/sanitizer/timeout/OOM 발견 입력은 없었다.

| 타깃 | 실행 수 | libFuzzer cov / ft | new_units_added | peak RSS(MiB) |
|---|---:|---:|---:|---:|
| .jobs | 5,000 | 1,078 / 3,947 | 79 | 266 |
| Project XML | 5,000 | 1,805 / 2,915 | 151 | 415 |
| Primavera XER | 5,000 | 2,215 / 4,634 | 243 | 423 |

cov/ft는 libFuzzer의 계측 카운터이며 형식/분기 검증률(%)이 아니다. new_units_added는 실행 중 추가 이벤트 수이며 최종 파일 수와 같지 않다. [검증 묶음](validation/parser-safety-2026-10-28/verification.json)에 revision·OS별 시간·일정 해시를 기록했다. Linux 하위 폴더에 실행 명령/반환 코드·환경·컴파일러·계측 목록·CTest와 seed/fuzz 원시 로그, 실제 corpus 319개 압축본과 파일별 SHA-256 manifest를 보존했다. 초기 재생 seed 56개가 모두 압축본에 포함됐음을 확인했다. 원시 입력/명령/도구 버전으로 후속 발견을 조사하며, 이번 통과를 취약점 부재나 외부 제품 전체 호환으로 확대하지 않는다.
