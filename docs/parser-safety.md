# 외부 파일 파서 검증

2026-10-28 사용자 후속 개발 요청에 따라 PARSER-SAFE-01~04를 구현 전에 확정한다. 파일 파서의 coverage-guided 입력 변이와 ASan/UBSan 런타임 검사가 이번 범위다. 일반 Release 옵션과 Windows/macOS 제품 지원 범위는 유지한다.

`.jobs`의 메모리 decode는 파일 저장·게시·잠금 없이 load와 저장 전 검증의 실제 envelope/해시/schema/도메인 검사를 공유한다. XML/XER harness도 실제 import 함수를 사용한다. 변이 시 checksum 때문에 native 본문 검증이 가려지는 문제는 별도 payload 경로로 보완하고 schema1~9에 동일 본문을 시도한다. envelope 원본 경로의 무결성 검사는 우회하지 않는다.

퍼징은 Clang에서 명시적으로 켜는 도구다. JOBS 코드에 coverage와 ASan/UBSan을 적용하고 사전 빌드 Qt 내부를 검사했다고 주장하지 않는다. 일반 테스트는 정상/손상 seed 재생, Linux CI는 계측된 전체 CTest와 3개 타깃의 유한 실행을 수행한다. 실행 수/seed·크기/시간/RSS 제한·revision·컴파일러·명령/반환 코드·로그·발견 입력을 기록한다. 정상적인 입력 거부만 허용하며 sanitizer/비정상 종료/시간 초과를 성공으로 바꾸지 않는다.

유한 smoke는 보안 보증이나 상용 제품 왕복 호환 판정이 아니다. Linux는 검증 실행기이며 신규 배포 플랫폼이 아니다. 퍼징 자체가 끝난 뒤에도 앱이나 사용자 파일을 지속 감시하지 않는다.

도구 계약은 [LLVM libFuzzer](https://llvm.org/docs/LibFuzzer.html), [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html), [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)를 기준으로 한다. 구현·명령·검증 결과는 후속 절에 기록한다.
