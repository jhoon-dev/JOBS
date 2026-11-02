# 화면 배치·잔여 선 점검 · 2026-11-02

Windows 11 / Qt 6.8.3 / MinGW 13.1.0 Release에서 확인했다. 앞선 우하단 상태 표시줄 이중 선 수정에 이어 주요 편집기, 분석·보고서, 작업공간, 복구, 업데이트, XML/XER 변환 창을 점검했다.

## 확인한 문제와 수정

- 빈 계산·편집 진행 라벨의 상태 표시줄 프레임이 밝은 세로선으로 남았다. 유휴 라벨을 숨기고 항목 프레임을 제거했다.
- 긴 기준선·프로젝트·시나리오 이름이 콤보박스의 최소 너비를 늘려 오른쪽 내용을 밀어냈다. 내용 길이와 컨트롤 너비를 분리하고 전체 선택값은 툴팁으로 확인하도록 했다.
- 보고서 지표 카드가 항상 4열이라 좁은 창에서 내용이 밀렸다. 창 너비에 맞게 1~4열로 재배치하고 마지막 카드·차트까지 내부 스크롤로 도달하도록 했다. 대시보드의 중복 프레임도 제거했다.
- 대화상자 전체가 스크롤되어 작은 창에서 닫기·취소 버튼이 사라졌다. 마지막 동작 행을 스크롤 본문 밖에 고정했다. 보고서 출력 설정과 내보내기 동작은 두 행으로 나누었다.
- 보고서·복구·작업공간·업데이트·XML/XER·작업 삽입·새 프로젝트·WBS 이동 등에 빠져 있던 공통 컨트롤 높이, 설명 줄바꿈, 작은 화면 스크롤 처리를 적용했다.
- 원가 차트의 고정 제목 높이와 금액 축 너비가 긴 텍스트를 자를 수 있었다. 제목을 줄바꿈하고 실제 금액 글자 폭에 맞게 축 여백을 계산한다.
- 보고서 미정 값의 밝은 강조 배경에 어두운 테마 글자색이 그대로 적용됐다. 강조 셀의 글자색도 함께 지정했다.

계산·저장·복구·변환의 입력/출력 계약은 변경하지 않았다. 넓은 자료 표는 자체 가로 스크롤로 읽으며, 작은 창의 본문 스크롤과 구분한다.

## 검증

`DialogLayoutTests.cpp`는 보고서, 기준선 비교, EVM, 자원 비교, 복구, 작업공간, 업데이트의 7개 창을 640×480 / 1024×650 / 1440×900과 밝은/어두운 QPalette로 조합한 **42개 배치**를 검사한다. 긴 선택값, 창 크기 유지, 버튼 높이와 고정 동작 행, 카드 재배치 후 보존, 대시보드 가로 넘침, 마지막 차트 접근, 미정 셀 대비를 확인한다. 밝은/어두운 팔레트 검사는 Windows 네이티브 플랫폼에서 QPalette를 바꾼 시험이며 OS 테마 설정 자체를 변경하지 않았다.

- Windows 네이티브 배치 검사: 42개 조합과 긴 이름 회귀 통과(QtTest 45 passed, 초기화/정리 포함).
- 150% 배율: 주요 7개 창의 640×480 배치 통과(QtTest 9 passed, 초기화/정리 포함).
- 기존 Windows 네이티브 화면 회귀 11개 실행 파일 모두 통과.
- 전체 CTest **63/63 통과**, 211.90초. 화면 외 계산·저장·파서·복구·보고서 출력 회귀를 포함한다.
- clang-format 18.1.8 검사와 `git diff --check` 통과.

로컬 산출물은 `build/ui-layout-final/`에, 전체 회귀 로그는 `build/ui-layout-final-ctest.log`에 있다. 네이티브 화면 회귀는 편집·보고서·자원·원가·기준선/실적·공수·달력·제약·XML·XER·작업공간을 포함한다. macOS 실기기와 실제 Store 배포본은 이번 로컬 검증 범위에 포함하지 않는다.

```powershell
$env:QT_PLUGIN_PATH = "$PWD/.deps/Qt/6.8.3/mingw_64/plugins"
ctest --test-dir build/ui-lines-fix --output-on-failure
$env:JOBS_LAYOUT_AUDIT_DIR = "$PWD/build/ui-layout-final/matrix"
./build/ui-lines-fix/jobs_dialog_layout_tests.exe -platform windows -o build/ui-layout-final-matrix.log,txt
$env:JOBS_EDITOR_CAPTURE_DIR = "$PWD/build/ui-layout-final/native/editors"
./build/ui-lines-fix/jobs_editing_polish_tests.exe -platform windows
```

Qt/MinGW의 `bin`과 CMake/CTest가 PATH에 있는 개발 환경에서 실행한다. 위 캡처 경로 환경변수는 선택 사항이다.

## 대표 화면

| 화면 | 캡처 |
|---|---|
| 작은 보고서 · 2열 카드와 고정 내보내기 | [640 / dark](report-640-dark.png) |
| 일반 보고서 · 4열 카드 | [1440 / light](report-1440-light.png) |
| 작은 자원 비교 · 컨트롤과 고정 닫기 | [640 / dark](resource-640-dark.png) |
| 작은 복구 선택 · 하단 동작 유지 | [640 / dark](recovery-640-dark.png) |
| 작은 작업공간 | [640 / light](workspace-640-light.png) |
| 작은 업데이트 창 | [640 / dark](update-640-dark.png) |
