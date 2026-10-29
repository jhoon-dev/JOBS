# 앱 업데이트 계약

2026-10-29. UPDATE-01~06을 구현 전에 확정한다. 사용자는 시작 시 새 버전 알림과 설정에서 확인/업데이트하는 흐름을 요청했다. 기본 자동 확인은 켜며 설정에서 끌 수 있다. 한 프로세스의 여러 창은 하나의 비동기 서비스를 공유한다. 일반 작업과 복구 검색을 막지 않고 새 버전 배너를 표시한다. 확인 취소·오프라인·시간 초과·미게시·개발 패키지를 정상 최신 상태와 구분한다.

Store판의 업데이트 진실 원본은 Windows.Services.Store.StoreContext다. GetAppAndOptionalStorePackageUpdatesAsync 결과로 판정하고 설치는 사용자가 Microsoft Store 업데이트 화면에서 진행한다. Microsoft Store가 서명/다운로드/설치를 관리한다. 개발용 unsigned identity는 Store 조회 대상이 아니며 아직 등록/심사/게시 전임을 안내한다. GitHub의 최신 버전과 Store 배포 상태를 섞지 않는다. 실제 Store 신규 버전은 계정 연결 및 게시 전이므로 모의 응답과 개발 패키지 검증을 실제 Store E2E로 주장하지 않는다.

일반판은 인증 없이 https://api.github.com/repos/jhoon-dev/JOBS/releases/latest 를 조회한다. 정식 3자리 app version과 Release tag를 비교한다. App version은 CMake의 제품 버전과 일치시키며 Store의 4자리 패키지 버전과는 별개다. 초안/prerelease·잘못된 버전·타 저장소 링크·무제한 응답을 거부한다. 정식 Release가 아직 없으면 미게시로 안내한다. Windows 설치 자산 이름은 JOBS-<version>-Setup-win64-<revision>.exe이며 asset의 GitHub SHA-256 digest와 실제 크기를 확인한다. HTTPS GitHub/CDN으로만 이동하고 자동으로 다운로드/실행하지 않는다. digest가 없으면 자동 설치를 허용하지 않고 릴리스 페이지에서 수동 다운로드를 안내한다. 개발 실행·macOS는 릴리스 페이지를 사용한다.

검증된 설치기를 실행하기 전에 모든 문서의 기존 저장 확인을 거친다. 취소/편집/계산 중에는 설치를 시작하지 않는다. Windows 설치기는 이전 PID 종료 대기를 지원하며 기존 등록 경로에 같은 사용자 설치를 갱신한다. 업데이트는 제거기를 호출하지 않는다. 캐시는 앱 전용 상태 아래에 두고 취소/오류의 부분 파일을 폐기하며 일반 제거 범위에 포함한다. 실행 실패 시 앱을 유지하고 오류를 표시한다.

설정은 배포판별 applicationSettings에 저장한다. 자동 확인을 끄면 시작 시 외부 요청이 발생하지 않는다. 요청에는 프로젝트·자원·비용·파일 경로·계정 토큰을 담지 않는다. GitHub에는 일반 HTTP 연결 정보와 JOBS updater User-Agent가 전달되고 Store는 Microsoft 정책에 따른다. 배포 전 개인정보 처리방침을 갱신한다. 공개 Release·Store 업로드·오픈소스 라이선스 선택은 이 작업에서 수행하지 않는다.

운영자는 향후 더 높은 app version으로 빌드한 설치기를 같은 tag의 정식 GitHub Release에 올린다. GitHub가 제공하는 SHA-256 digest가 있어야 앱 내 검증 설치가 가능하다. 작성 중/프리뷰 Release는 자동 업데이트 대상이 아니다. Store판은 실제 identity를 유지하고 더 높은 패키지 버전으로 Store에 게시한다.

근거: [Store 앱 업데이트](https://learn.microsoft.com/en-us/windows/msix/store-developer-package-update), [Store URI](https://learn.microsoft.com/en-us/windows/apps/develop/launch/launch-store-app), [GitHub 최신 Release/asset](https://docs.github.com/en/rest/releases/releases#get-the-latest-release), [Qt 네트워크](https://doc.qt.io/qt-6.8/qnetworkaccessmanager.html).

앱 버전은 0.2.0, 이번 개발용 Windows 패키지는 1.0.2.0을 사용한다. 일반 Windows 설치판은 인터넷 출처(Zone.Identifier)를 남기고 Windows Shell로 설치기를 실행해 OS 보안 판정을 유지한다. GitHub SHA 검증은 공인 코드 서명 검증과 별개이며, 기존 미서명 preview의 서명 상태를 바꾸지 않는다. 다른 설치본 프로세스가 남아 있으면 파일 교체 전에 거부한다.

운영 절차: CMake의 제품 버전을 올려 커밋한 뒤 `python scripts/package-windows.py`로 설치기를 만든다. GitHub Releases에서 같은 버전의 `v<app-version>` tag를 선택하고 해당 EXE와 checksum을 첨부한 draft를 준비한다. 검증 후 운영자가 정식 Release로 게시하고 Latest로 지정한다. 앱의 확인은 공개된 정식 Release부터 동작하며 draft/prerelease는 대상이 아니다. 이 작업은 공개 게시를 실행하지 않는다.
