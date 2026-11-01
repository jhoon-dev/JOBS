# Mac App Store 배포 구성

전용 앱은 **Project Planning Manager**, Apple 앱 ID `6829146371`, Bundle ID `com.toolmoa.jobs`, 팀 `V6445K474Q`다. 무료·지원 가능한 모든 국가에 출시한다. 다른 앱의 기록·서명 자료를 임의로 가져오지 않는다.

## 준비와 실제 제출

`Mac App Store preparation`은 Qt 6.8.3 공식 qtbase archive와 고정 SHA-256을 확인하고 `-feature-appstore-compliant` universal arm64/x86_64 Qt를 사용한다. 네이티브 회귀·실제 Mac 화면·별도 서명된 sandbox 검증 앱을 실행한 다음 미서명 `PPM.app` 개발 번들을 만든다. build_number 입력은 Apple CFBundleVersion이며 업로드마다 증가시킨다. 앱 버전은 0.2.0이다.

`Mac App Store signing and upload`는 **같은 커밋의 성공한 preparation run ID**를 받는다. 임시 키체인에서 앱·설치기 배포 인증서를 선택하고, 정확한 팀·Bundle ID·유효기간·인증서가 일치하는 App Store 프로파일을 검사한다. 내장 Mach-O/framework부터 앱 순서로 서명하고 최종 entitlement·pkg 서명·Apple altool validation을 확인한다. output은 서명된 앱 zip, pkg와 비밀 값을 제외한 상태·검증 결과다. 빌드 업로드는 App Store 심사 제출과 별개다.

기본값은 업로드하지 않는다. 외부 폴더 승인·취소·재실행 bookmark 접근을 실제 Mac에서 확인한 경우에만 `external_folders_verified`를 선택한다. 이 확인 없이 upload를 요청하면 실패한다. 컨테이너 자동 검증 성공을 외부 폴더 검증으로 취급하지 않는다.

## 앱의 sandbox 파일 접근

Mac Store 구성에만 `NSOpenPanel` 폴더 선택과 app-scoped bookmark 저장·시작 시 복원·stale 갱신을 포함한다. 실제 sandbox entitlement가 활성인 프로세스에서만 동작한다. 프로젝트 저장/열기, workspace와 참조 프로젝트, XML/XER 교환, 보고서 출력의 부모 폴더를 먼저 승인받고 worker에서도 GUI 스레드로 승인 요청을 전달한다. 폴더를 취소하면 해당 작업을 중단한다. scope는 비동기 파일 작업을 위해 앱 종료까지 유지하고 종료 시 반환한다.

앱 컨테이너와 앱 전용 임시 위치에는 별도 폴더 승인을 요구하지 않는다. 북마크는 컨테이너의 `mac-sandbox-folders.ini`에 저장한다. Windows 및 일반 Mac 배포판은 이 권한 흐름을 사용하지 않는다. Mac Store는 App Store 업데이트를 사용하고 자체 HTTP 버전 확인·Windows 설치기 다운로드를 실행하지 않는다.

`check-mac-sandbox.py`는 별도 ad-hoc 서명 검증 앱으로 실제 sandbox 활성 여부, 미선택 외부 파일 쓰기 거부, 프로세스 재시작 후 프로젝트·잠금·workspace·복구·설정·PDF/XLSX·XML/XER를 검사한다. 이 검증 앱은 제품 번들에 포함하지 않는다.

## 이 저장소에 설정할 GitHub Actions Secrets

[Repository Secrets 설정](https://github.com/jhoon-dev/JOBS/settings/secrets/actions)에서 다음 여섯 항목을 만든다. 비밀번호·인증서 private key·API private key는 채팅·소스·artifact에 넣지 않는다.

| 이름 | 값 |
|---|---|
| `MAC_APP_STORE_CERTIFICATES_P12_BASE64` | Mac App Store 앱/설치기 배포 인증서와 각각의 private key를 함께 내보낸 P12의 base64. 앱은 Apple Distribution 또는 3rd Party Mac Developer Application, 설치기는 3rd Party Mac Developer Installer identity가 필요하다. Developer ID 배포 인증서를 대신 쓰지 않는다. |
| `MAC_APP_STORE_CERTIFICATES_PASSWORD` | 해당 P12의 내보내기 비밀번호 |
| `MAC_APP_STORE_PROVISION_PROFILE_BASE64` | 정확히 com.toolmoa.jobs용 Mac App Store 배포 프로파일의 base64. development/ad-hoc/와일드카드 프로파일은 거부한다. |
| `APP_STORE_CONNECT_API_KEY_ID` | 업로드 권한이 있는 App Store Connect 팀 API key의 10자리 ID |
| `APP_STORE_CONNECT_API_ISSUER_ID` | 해당 팀 API key의 issuer UUID |
| `APP_STORE_CONNECT_API_PRIVATE_KEY` | AuthKey_…p8의 PEM 원문(줄바꿈 포함) |

Actions는 비밀 값을 환경 변수로 전달한다. P12·프로파일·p8은 runner 임시 폴더에 제한된 권한으로 만들며, 키체인과 임시 private key는 작업 후 제거한다. 인증서 생성이나 기존 다른 앱의 private key 접근은 자동으로 수행하지 않는다. 서명 자료가 없으면 계정 소유자가 먼저 Apple 인증서·프로파일과 API key를 준비해야 한다.

## 외부 폴더 수동 검증

서명된 빌드를 테스트 가능한 Mac에서 실행하여 컨테이너 밖의 한국어/공백 경로에 새 프로젝트 저장·종료·재실행·재열기·수정 저장을 확인한다. 다른 폴더의 프로젝트를 참조하는 workspace 열기, PDF/XLSX 출력과 XML/XER 교환도 확인한다. 폴더 선택을 취소한 경우 기존 파일과 편집 내용이 유지되어야 한다. 폴더 이동·북마크 복원 실패 시 재승인으로 정상 동작하는지도 확인한다. 실제 결과를 기록하기 전에는 upload를 실행하지 않는다.

## 현재 스토어 초안

2026-11-01: 앱 이름·설명·심사 메모·실제 1440×900 Mac 스크린샷과 사용자 입력 심사 연락처를 저장했다. 사용자 동의 후 데이터 수집 없음 답변을 게시했다. 가격·국가·카테고리·연령 등급을 설정했다. 실제 서명 빌드 업로드·선택·심사 제출은 후속 단계다.

공식 참고: [Apple 파일 접근](https://developer.apple.com/documentation/security/accessing-files-from-the-macos-app-sandbox), [배포 서명](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac/), [앱 빌드 업로드](https://developer.apple.com/help/app-store-connect/manage-builds/upload-builds), [altool 옵션](https://help.apple.com/asc/appsaltool/en.lproj/static.html), [Qt 배포 도구](https://doc.qt.io/qt-6.8/macos-deployment.html).
