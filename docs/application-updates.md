# 스토어 전용 앱 업데이트 계약

2026-11-01 사용자 확정: Windows 배포와 업데이트는 Microsoft Store MSIX만 사용한다. 이전 GitHub Release/EXE 설치기 업데이트 요구는 폐기했다. 앱에는 릴리스 API 조회, 설치기 다운로드·해시 검사·실행, 문서 종료를 동반하는 외부 설치기 handoff가 없다. Qt Network 의존성도 제거했다.

Store판의 진실 원본은 Windows.Services.Store.StoreContext다. GetAppAndOptionalStorePackageUpdatesAsync 결과로 업데이트 유무를 판정한다. 다운로드·서명·설치는 Microsoft Store가 담당하며 사용자는 앱의 Store 열기 버튼으로 스토어 업데이트 화면을 연다. 실제 게시된 패키지가 없으면 오류를 최신 상태로 표시하지 않는다.

Store판에서 시작 시 확인은 기본으로 켜져 있고 설정에서 해제할 수 있다. 같은 프로세스의 여러 창은 서비스를 공유해 시작 확인을 한 번만 요청한다. 수동 확인·취소·시간 초과·재시도를 지원하며 취소·시간 초과 이전의 늦은 결과는 무시한다. 자동 확인 설정과 마지막 확인 시각은 기존 applicationSettings에 보관한다.

일반 개발 실행본과 개발용 unsigned MSIX는 자동 확인을 하지 않는다. 수동 확인 시 업데이트 미지원과 스토어 배포본 사용을 안내한다. 실행 디렉터리의 옛 install-info.ini나 제거 레지스트리로 EXE 업데이트 기능을 다시 활성화할 수 없다. Microsoft Store에 등록된 실제 MSIX만 Store 조회 대상이다.

Mac App Store판은 앱 자체의 자동 확인을 하지 않고 App Store의 이 앱 페이지를 연다. Windows Store API를 호출하거나 별도 HTTP 다운로드를 하지 않는다. 스토어 페이지를 여는 데 실패하면 앱에 오류를 표시한다. 스토어 버튼은 문서를 닫거나 앱을 종료하지 않는다.

업데이트 요청에는 프로젝트 내용·자원·비용·파일 경로·계정 토큰을 넣지 않는다. 스토어 통신·계정·다운로드 처리는 해당 플랫폼 정책에 따른다. 과거 EXE 업데이트 검사 기록은 이력이며 현재 배포 방식의 검증 근거로 사용하지 않는다.

근거: [Store 앱 업데이트](https://learn.microsoft.com/en-us/windows/msix/store-developer-package-update), [Store URI](https://learn.microsoft.com/en-us/windows/apps/develop/launch/launch-store-app).
