# Windows 배포: Microsoft Store MSIX 전용

2026-11-01 사용자 요청으로 EXE/NSIS 설치기 배포를 폐기했다. 설치기 생성·도구 설치·설치 handoff 검사 스크립트와 NSIS 설정을 삭제했고 앱의 EXE 다운로드·실행 업데이트 경로도 제거했다.

현재 빌드·패키징·설치·업데이트는 [MSIX·Microsoft Store 배포 준비](msix-store.md)와 [스토어 전용 업데이트 계약](application-updates.md)을 따른다. 개발용 unsigned MSIX는 CI 내부 설치/업그레이드/제거 검사용이며 사용자 배포 파일이 아니다. 사용자는 Store가 서명·제공하는 패키지를 설치한다.

과거 EXE 설치/제거 검증 기록은 `docs/validation`에 이력으로 보존한다. 로컬 `dist`의 무서명 설치기·개발 MSIX·메타데이터는 `build/retired-unsigned-windows`로 옮겨 배포 경로에서 제외했다. 기존 사용자 프로젝트나 설치된 앱을 자동으로 제거하지 않는다.
