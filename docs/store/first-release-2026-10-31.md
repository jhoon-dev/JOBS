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
| Apple 앱 레코드·서명·업로드 | 준비 중 |
| 공개 출시·스토어 승인 | 미완료 |

계정 표시명, 심사 연락처와 인증서/비밀키는 이 기록에 복사하지 않는다. 실제 identity JSON과 최종 패키지는 `build/`와 `dist/`에 보관한다.

macOS CI에서 `std::size_t`와 `std::uint64_t`를 하나의 `auto` 선언으로 묶은 테스트가 실패했다. 선언을 분리하고 관련 Windows 회귀 2개와 포맷 검사를 통과한 `703ea60`을 원격 CI에 반영했다. 네이티브 macOS 결과는 별도로 확인한다.

Microsoft 제출은 실제 identity의 probe 없는 MSIX, 심사/설치 검증과 소개·실제 화면·등급·국가·가격 설정을 완료한 뒤 진행한다. Mac 제출은 샌드박스 파일 접근·서명/프로비저닝·Qt App Store 준수 빌드·업데이트 경로까지 검증한 후 진행한다. 개발용 패키지나 미서명 Mac 앱을 심사용 산출물로 표시하지 않는다.
