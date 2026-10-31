# Mac App Store 배포 구성

2026-10-31 사용자 요청으로 최초 배포를 진행한다. 앱은 `JOBS Project Planner`, 전용 Bundle ID는 `com.toolmoa.jobs`이다. 기존 PM Canvas 앱과 식별자·제출 기록을 공유하지 않는다.

## 현재 준비와 실제 제출의 구분

`Mac App Store preparation` 수동 workflow는 Qt 6.8.3의 qtbase·qttools 공식 source archive와 고정 SHA-256을 검증한다. qtbase는 `-feature-appstore-compliant`를 사용해 arm64/x86_64 동적 프레임워크로 빌드하고 qttools의 macdeployqt만 같은 Qt에 연결해 빌드한다. 일반 macOS용 Qt 바이너리를 그대로 스토어용으로 간주하지 않는다. 앱의 네이티브 회귀와 실제 화면 캡처를 거쳐 `macdeployqt -appstore-compliant`로 별도 개발용 번들을 만든다.

이 출력은 **미서명 개발용 앱**이며, App Sandbox·서명/프로비저닝·설치/저장/복구 검증이 끝난 제출용 `.pkg`가 아니다. 출력 status JSON에도 서명·sandbox 검증·제출 가능 여부를 각각 기록한다.

`-DJOBS_MAC_APP_STORE=ON`은 Mac 전용 업데이트 경로를 선택한다. 앱 자체의 시작/수동 HTTP 버전 확인과 Windows Store API/설치기 다운로드를 사용하지 않고 전용 Apple 앱 레코드를 App Store에서 연다. 이 옵션만으로 sandbox나 서명을 구성하지 않는다. 회귀 검사는 네트워크·Windows Store 호출이 발생하지 않는지와 실제 UI 버튼의 URL을 확인한다.

`JOBS_MAC_BUILD_NUMBER`는 기본값 `1`인 양의 정수이며 재업로드 때 증가시킨다. 표시 버전은 `0.2.0`이다. `capture-workspace-ui.py build --store`는 최초 표시 후 1440×900 네이티브 창 배치를 렌더링하고 실제 PNG 크기를 검증한다. 일반 화면 크기·배율 검사를 대신하지 않는다.

## 최종 제출에 필요한 구성

- 앱 버전 `0.2.0`과 별도 증가하는 build 번호, 전용 Bundle ID·실제 Apple 팀의 프로파일을 일치시킨다.
- App Sandbox와 사용자 선택 파일 읽기/쓰기, 필요한 영구 접근 bookmark를 구성한다. 프로젝트 `.lock`과 원자 저장의 임시 파일, 작업 영역의 참조 프로젝트·보고서/교환 폴더 접근을 샌드박스 상태에서 검증한다. 일반 앱의 파일 접근 성공으로 대체하지 않는다.
- Mac App Store판은 스토어가 업데이트를 관리한다. Windows 설치기 다운로드와 GitHub 자체 업데이트 설치를 사용하지 않도록 배포 채널을 구분한다.
- 앱과 내장 Qt frameworks/plugins를 유효한 Mac App Store 배포 identity로 서명하고 프로비저닝을 포함한다. 별도 installer 배포 identity로 `.pkg`를 만든 뒤 검증·업로드한다. 인증서와 private key는 저장소·artifact에 포함하지 않는다.
- Apple 제출용 실제 Mac 화면, 개인정보/지원 URL, 무료 가격·지원 국가, 분류·연령 등급·암호화·심사 연락처·샌드박스 사용 설명을 완성한다. 접근성이나 언어 지원을 실제 검증보다 넓게 선언하지 않는다.
- Qt의 LGPL 고지·소스 및 재링크/실행 권한을 배포 조건에 맞게 보존한다. 서명이나 스토어 승인이 라이선스 의무를 대체하지 않는다.

공식 근거: [Qt 6.8 macOS 배포](https://doc.qt.io/qt-6.8/macos.html), [Qt 배포 도구](https://doc.qt.io/qt-6.8/macos-deployment.html), [Apple App Sandbox 파일 접근](https://developer.apple.com/documentation/security/accessing-files-from-the-macos-app-sandbox), [Qt LGPL 배포 의무](https://www.qt.io/development/open-source-lgpl-obligations).
