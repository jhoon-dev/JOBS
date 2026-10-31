# 최초 스토어 출시 진행 기록

사용자 확정: 두 개발자 계정 등록 완료, JOBS는 신규 앱, 무료·지원 가능한 모든 국가 출시.

`JOBS`는 Microsoft Store에서 사용 불가였고 `JOBS Project Planner`는 사용 가능한 것을 확인한 뒤 신규 앱으로 예약했다. 별도 이름 선호 답변이 오면 제출 전에 반영한다.

| 항목 | 실제 상태 |
|---|---|
| Microsoft 앱 | JOBS Project Planner, 초안 |
| Microsoft Store ID | `9NDHXXVR8F2P` |
| Microsoft 패키지 이름 | `jhoon-dev.JOBSProjectPlanner` |
| Microsoft Publisher | `CN=C488F7ED-2611-417D-B3ED-C39E81B7590E` |
| Apple 전용 Bundle ID | `com.toolmoa.jobs`, 등록 완료 |
| Microsoft 가격·국가 | 무료, 전체 240개 시장 및 향후 시장 |
| Microsoft 제출 1 | `1152921505702283208`, 패키지/속성/가격/한국어·영어 소개/제출 옵션 완료, 연령 등급 약관 동의 대기 |
| Apple 앱 레코드 | `6829146371`, SKU `JOBS-MAC`, macOS `0.2.0` 제출 준비 중 |
| Apple 가격·국가 | 무료, 전체 175개 국가/지역 및 향후 국가/지역 |
| Apple 소개·분류·등급 | 한국어 설명·지원 URL·생산성·4+ (한국 전체) 저장, 개인정보 URL·수집하지 않음 응답 초안 저장 |
| Apple 서명·업로드 | 미완료. 실제 sandbox 파일 접근/복구·서명/프로비저닝 검증 전 |
| 공개 출시·스토어 승인 | 미완료 |

계정 표시명, 심사 연락처와 인증서/비밀키는 이 기록에 복사하지 않는다. 실제 identity JSON과 최종 패키지는 `build/`와 `dist/`에 보관한다.

macOS CI에서 `std::size_t`와 `std::uint64_t`를 하나의 `auto` 선언으로 묶은 테스트가 실패했다. 선언을 분리하고 관련 Windows 회귀 2개와 포맷 검사를 통과한 `703ea60`을 원격 CI에 반영했다. 네이티브 macOS 결과는 별도로 확인한다.

`616e584f`의 Desktop workflow [41176684462](https://github.com/jhoon-dev/JOBS/actions/runs/41176684462)에서 Windows/macOS 전체 빌드·회귀가 통과했다. MSIX workflow [41176684460](https://github.com/jhoon-dev/JOBS/actions/runs/41176684460)은 실제 개발용 패키지 설치·실행·저장/열기·업그레이드·복구·제거를 통과했다. 이 증거는 최종 Store identity 패키지의 WACK 결과가 아니며 WACK은 미실행이다.

업로드한 최종 Windows 파일은 `JOBS-store-1.0.2.0-x64-616e584f.msix`, SHA-256 `e1adf1c729c726060f9b144bc9b69ef48dd5eccb2ffd62c6e971b9835d4e2527`, 18,175,683바이트다. 실제 Store identity·probe 제외·MakeAppx 검증을 완료했으며 Partner Center에서 **Validated**를 확인했다. 승인이나 공개 게시를 뜻하지 않는다.

최신 Windows 실제 패키지의 1440×900 일정 화면을 업로드하고 한국어·영어 소개의 완료 상태를 확인했다. 영어 소개에도 앱 UI는 한국어임을 명시했다. macOS 일반 CI의 `workspace-1440.png`는 파일 이름과 달리 실제 PNG가 1024×653이므로 Apple 제출에 사용하지 않았다. 임의 확대 대신 허용 해상도에서 새 네이티브 화면이 필요하다. 전용 크기 캡처 모드와 PNG 실제 치수 검사를 추가했고 Windows 네이티브 캡처·기존 `workspace_presentation` 회귀를 통과했다. macOS 결과는 새 CI에서 별도로 확인한다.

Mac 전용 Qt 준비 workflow [41177090201](https://github.com/jhoon-dev/JOBS/actions/runs/41177090201)은 별도 universal Qt를 빌드 중이다. 이 실행의 출력은 서명·sandbox 미검증 개발용 번들이며 제출 가능한 `.pkg`가 아니다. 후속 Mac Store 업데이트 채널 검사는 로컬 `application_updates`와 clang-format 18.1.8 검사를 통과했다.

Microsoft 제출은 실제 identity의 probe 없는 MSIX, 심사/설치 검증과 소개·실제 화면·등급·국가·가격 설정을 완료한 뒤 진행한다. Mac 제출은 샌드박스 파일 접근·서명/프로비저닝·Qt App Store 준수 빌드·업데이트 경로까지 검증한 후 진행한다. 개발용 패키지나 미서명 Mac 앱을 심사용 산출물로 표시하지 않는다.
