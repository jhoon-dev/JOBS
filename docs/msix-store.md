# MSIX·Microsoft Store 배포 준비

2026-10-29 사용자 승인. MSIX-01~06을 구현 전에 확정한다. 사용자는 개인 명의 배포를 선택했고 아직 Partner Center 앱 이름을 예약하지 않았다. 계정 가입/신원 확인·이름 예약·실제 패키지 ID·게시자 표시명·가격/대상 국가/연령 등급·최종 제출은 계정 소유자의 결정이다. 현 단계는 검증 패키지와 제출 자료, 실제 ID를 입력한 후 사용할 제출용 생성기까지다.

Store 배포는 공인 인증서를 별도 구매하지 않는 MSIX 경로를 사용한다. 정식 Store 모드는 Partner Center의 Identity Name/Publisher/Publisher Display Name을 명시적으로 받는다. 임의의 개인명이나 ToolMoa 명칭을 검증된 게시자로 가장하지 않는다. 개발용 패키지는 이름과 publisher를 분리하고 Windows unsigned 개발 OID를 사용한다. 개발용 파일은 일반 배포/Store 업로드 대상이 아니다. 별도 인증서 설치나 OS 보안/개발자 설정 변경을 자동으로 수행하지 않는다.

앱은 기존 Qt Widgets/Win32 full trust 데스크톱 형태로 x64, Windows 10 2004(10.0.19041) 이상을 대상으로 한다. manifest는 packagedClassicApp/mediumIL과 필요한 runFullTrust만 선언한다. 관리자 권한·서비스·드라이버·백그라운드 시작·외부 위치 패키지·파일 연결은 추가하지 않는다. 사용자 선택 경로의 프로젝트/내보내기와 잠금은 기존 기능을 사용한다. MSIX 설치 위치는 읽기 전용이며 실행 디렉터리를 저장 위치로 가정하지 않는다.

MSIX에서 앱 상태는 package family별 LocalState 아래에 둔다. 설정은 전용 INI, 자동 복구는 기존 checkpoint 형식을 사용한다. EXE판의 Qt 기본 설정/복구는 그대로 유지하며 자동 마이그레이션하지 않는다. 같은 원본 문서는 기존 파일 잠금/지문으로 보호한다. Windows 업데이트는 패키지 identity를 유지하고 버전을 올려 상태를 보존하며, 일반 제거는 패키지 상태를 제거한다. 외부 사용자 프로젝트 및 다른 패키지/EXE판의 상태를 지우지 않는다.

공통 Windows payload는 Qt 도구로 런타임/플러그인과 라이선스를 모은다. MSIX에는 NSIS 설치기/제거기나 디버그/테스트/개발용 인증서 파일을 포함하지 않는다. 테스트 probe는 개발 검증 모드에만 포함하며 최종 Store 모드에는 없다. Microsoft SDK의 MakeAppx 검증과 ZIP/XML/파일 SHA 교차 검사를 수행한다. 앱 아이콘은 저장소의 벡터 원본과 재현 가능한 출력 자산을 사용하며, Store 화면은 실제 앱의 테스트 화면으로 준비한다.

로컬 환경은 비관리자이고 Windows SDK가 설치되지 않았다. SDK 패키징 도구는 공식 배포 원본에서 검증해 별도 도구 폴더에 설치한다. 서명 없는 실행 패키지 설치는 관리자 권한이 필요한 Windows 개발 경로이므로 로컬 보안 설정을 바꾸지 않고 관리자 Windows CI에서 수용 검사를 수행한다. 실제 package identity 아래의 probe가 파일/설정/복구를 만들고 다음 버전에서 재확인한 후 제거한다. 실패 시 소유한 테스트 패키지만 정리하며 기존 설치/사용자 데이터를 변경하지 않는다.

제출 체크리스트는 현재 공백인 Partner Center ID, 실제 제출용 package 재생성, WACK 결과, privacy/support URL, 설명/화면·등급·국가·가격·runFullTrust 사유와 최종 심사를 구분한다. WACK와 실제 Store 승인은 실행하지 않은 경우 미완료로 남긴다. 업로드나 공개 게시를 자동으로 수행하지 않는다.

공식 기준: [MSIX Store 패키지 요건](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/app-package-requirements), [MakeAppx](https://learn.microsoft.com/en-us/windows/msix/package/create-app-package-with-makeappx-tool), [unsigned 개발 패키지](https://learn.microsoft.com/en-us/windows/msix/package/unsigned-package), [패키지 앱 상태/제거](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes), [Store 화면](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/screenshots-and-images).
