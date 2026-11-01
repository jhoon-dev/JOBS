# Project Planning Manager (PPM)

2026-11-01 사용자 확정. 공식 제품명은 **Project Planning Manager**, 짧은 이름은 **PPM**이다. 전체 표시가 필요한 곳은 **Project Planning Manager (PPM)**를 사용한다.

프로젝트 파일은 `.ppm`, 다중 프로젝트 작업공간은 `.ppmworkspace`다. 이전 확장자는 열기·저장 대상에서 제외한다. XML/XER 교환 폴더에는 `project.ppm` 또는 `source.ppm`을 보관한다. 같은 `.ppm` 확장자를 쓰는 이미지 파일은 JSON 형식·checksum 검증에서 거부한다. OS의 기존 이미지 파일 기본 연결을 변경하지 않는다.

내부 C++ namespace/빌드 target, GitHub 주소, 등록된 Microsoft identity/Apple Bundle ID, 업그레이드 등록 키와 기존 설정·복구 위치는 제품 표시 이름과 독립인 안정된 식별자다. JSON envelope의 형식 식별자와 schema도 바꾸지 않는다. 원본 fixture bytes와 checksum을 유지하면서 fixture 경로를 새 확장자로 옮긴다. 이전 확장자를 자동 탐색하거나 별도의 호환 열기 경로를 제공하지 않는다.

검증은 전체 CTest, Python MSIX 계약, 새 확장자 실패 시 상태 보존·읽기/쓰기·교환/복구, 실제 PPM 창과 패키지 실행 파일을 대상으로 한다. 과거 검증 문서와 배포 파일명은 당시 기록으로 유지한다.

## 2026-11-01 Windows 검증

- Release 전체 CTest 62/62 통과. 이전 확장자 거부 시 원본·세션·이력 보존, 대문자 `.PPM`, 이미지 PPM 거부, workspace·교환·복구·설정 경로를 포함한다.
- Python MSIX 계약 7/7, clang-format 18.1.8 검사 87개 C++ 파일 통과. 파서 시드 62개와 고정 fixture 13개의 staged bytes/SHA-256 일치를 확인했다.
- 독립 리더로 XLSX 5개·PDF 6개의 구조·내용·PPM 표기와 Project XML을 검증했다.
- 네이티브 Qt 화면 1440×900 및 1024 크기 캡처를 확인했다. `PPM.exe`의 ProductName/FileDescription은 `Project Planning Manager (PPM)`, OriginalFilename은 `PPM.exe`, 제품 버전은 `0.2.0`이다.
- Microsoft Partner Center에서 Project Planning Manager 이름을 예약하고 대시보드 이름을 변경했다. Apple 앱 초안의 같은 이름 저장도 완료했다. 이름 예약은 스토어 최종 제출·승인을 뜻하지 않는다.

로컬 증거는 `build/ppm-validation.log`, `build/ppm-format-check.json`, `build/workspace-ui/`, `build/store-proof/`에 생성한다. macOS 및 실제 설치/제거 검증 결과는 각 CI 실행에 별도로 보존하며 Windows 로컬 검증으로 대신하지 않는다.
