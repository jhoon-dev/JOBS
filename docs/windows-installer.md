# Windows 설치 패키지

2026-10-28 사용자 요청. PACK-01~04를 구현 전에 확정한다. 현재 Windows x64 Release 앱을 사용자별 단일 설치 exe로 제공한다. 기본 설치 경로는 사용자 LocalAppData 아래 Programs/JOBS이며 관리자 권한을 요청하지 않는다. 시작 메뉴 바로가기와 선택적 바탕화면 바로가기, Windows 앱 목록의 제거 항목을 제공한다.

Qt 6.8.3의 windeployqt로 필요한 동적 라이브러리·플러그인·컴파일러 런타임을 모으고 개발 PATH 없는 실행을 확인한다. 설치 프로그램은 고정한 NSIS portable compiler를 사용한다. Qt/런타임 고지와 라이선스 자료, 소스 revision·파일별 SHA manifest를 포함한다. 빌드 도구와 산출물은 각각 `.deps`와 `build`/`dist`에 두며 저장소에는 패키징 소스와 검증 기록만 남긴다.

설치는 관련 없는 비어 있지 않은 폴더를 거부한다. 후속 사용자 요청으로 **완전 제거**를 기본 동작으로 변경했다. 제거는 등록 경로·제품 marker·폴더명이 일치하는 JOBS 설치 폴더 전체, JOBS 전용 Local/Roaming AppData·설정·캐시·자동 복구, 자신의 등록/바로가기를 삭제한다. 설치 폴더에 추가한 파일도 삭제 대상이다. 문서 폴더 등 설치 폴더 밖에 따로 저장한 프로젝트와 다른 앱의 데이터는 보존한다. 삭제 대상은 사용자별 등록과 고정된 JOBS 전용 경로로 제한하며 임의 위치를 탐색해 파일을 삭제하지 않는다. 임시 폴더에서 실제 실행·새로 만들기/저장/열기·완전 제거를 검증하며 기존 사용자 자료를 보호한다.

미서명 preview를 로컬 산출물로 전달한다. 코드 서명·자동 업데이트·GitHub Release 등 공개 배포·기존 파일 연결 및 macOS 패키지는 이번 요청에 포함하지 않는다. 설치 방법·재현 명령과 실제 검증 결과는 후속 절에 기록한다.

## 설치와 완전 제거

산출물은 `dist/JOBS-0.1.0-Setup-win64-2ff28ff8.exe`이며 크기는 12,830,792 bytes다. Windows x64용 미서명 0.1.0 Preview다. 설치 파일 하나만 실행하면 되며 Qt·Python·Office를 추가로 설치할 필요가 없다. 설치 전용 폴더 이름은 `JOBS`로 제한한다. 기본 경로를 쓰거나 `D:\Apps\JOBS`처럼 선택한다. 시작 메뉴의 JOBS로 실행한다.

Windows 설정의 앱 목록 또는 시작 메뉴의 JOBS 제거에서 제거한다. **설치 폴더 안의 모든 파일, JOBS 설정·캐시·자동 복구 자료를 삭제**한다. 제거 전에 앱을 닫아야 한다. 문서 폴더 등 설치 폴더 밖에 별도로 저장한 `.jobs` 프로젝트는 유지한다. 삭제 경로는 등록된 설치 폴더와 현재 사용자의 `LocalAppData/ToolMoa/JOBS`, `Roaming AppData/ToolMoa/JOBS`, `HKCU/Software/ToolMoa/JOBS`로 제한한다. 다른 ToolMoa 앱의 폴더는 삭제하지 않는다.

SHA-256: `6bd51edefd03d881d8c0f2974bb933d015f67adf3500b6602891452258190668`

## 개발자 재현

`packaging/toolchain-lock.json`에 NSIS 3.11의 원본/미러 URL과 독립적으로 게시된 archive checksum, compiler SHA를 고정했다. 다운로드 후 checksum을 확인하고 `.deps/nsis/nsis-3.11`에 압축을 푼다. Qt 6.8.3/MinGW 13.1과 CMake가 준비된 개발 환경에서 실행한다.

```powershell
python scripts/package-windows.py
# 다른 설치 위치의 도구는 --qt-root, --compiler-bin, --nsis-root, --build-dir로 지정한다.
```

읽기 전용 provenance를 위해 소스 변경을 커밋한 상태에서만 패키징한다. 새 stage를 만들어 기존 산출물을 지우지 않는다. Release/x64·고정 compiler hash·필수 DLL을 확인하고 windeployqt로 의존성을 모은다. Qt Concurrent는 앱 PE import에 별도 DLL이 없으며 Qt Core/Gui/Widgets 및 플러그인 의존성이 배포 도구로 수집된다. Qt 번역 catalog가 개발 SDK에 없어 Qt translation 파일은 제외했다. 앱의 기존 한국어 문구와 OS의 네이티브 대화상자를 실제 화면에서 확인했다.

Qt 고지/라이선스 원문·SPDX, GCC/MinGW/winpthreads와 NSIS 고지를 포함한다. 공급자 라이선스 원문은 bytes/hash를 유지한다. 자료 출처는 [Qt deployment](https://doc.qt.io/qt-6.8/windows-deployment.html), [Qt 6.8.3 license 원문](https://github.com/qt/qtbase/tree/v6.8.3/LICENSES), [NSIS](https://nsis.sourceforge.io/Download), toolchain lock을 따른다.

수용 helper는 **격리된/비어 있는 JOBS 데이터**에서만 사용한다. 기존 사용자 데이터나 설정이 있으면 중단한다. Windows Store/MSIX Python alias는 registry/AppData를 가상화할 수 있어 helper가 거부하므로 일반 `python.exe`의 실제 경로를 사용한다. 이 작업에서는 아래 가상환경의 일반 Python 3.12로 검사했다.

```powershell
& ./build/validation-python/Scripts/python.exe scripts/check-windows-installer.py install --installer ./dist/JOBS-0.1.0-Setup-win64-2ff28ff8.exe
& ./build/validation-python/Scripts/python.exe scripts/check-windows-installer.py launch
# 실제 설치 앱 UI에서 프로젝트를 만들고 state JSON의 project_file에 저장/재열기한 뒤 닫는다.
& ./build/validation-python/Scripts/python.exe scripts/check-windows-installer.py uninstall
```

## 검증 결과

[전체 검증 기록](validation/windows-installer-2026-10-28/verification.json)에 설치 exe hash·소스·payload manifest·NSIS/deployment 로그·실제 화면의 접근성 기록·생성 프로젝트를 보존했다.

- 패키징 revision `2ff28ff8775afd14aec45a8f9757f08373a839a5`. 한글/공백/미생성 부모 경로 설치, 관련 없는 비어 있지 않은 폴더 거부를 통과했다. 설치 파일 76개가 manifest SHA와 일치했다.
- 개발 PATH 및 Qt 환경 변수를 제거하고 앱 실행을 확인했다. 실제 설치 앱의 새로 만들기·한국어 이름의 저장·재열기를 검증했고 최종 후보에서도 재열기를 확인했다. 처음 생성/저장한 후보와 최종 후보의 앱/DLL/플러그인 bytes가 모두 일치했다.
- 재설치 전에 기존 바탕화면 바로가기 등록을 합성해 소유 정보가 유지되는지 검사했다. 최종 제거에서 설치 폴더에 추가한 파일까지 모두 사라지고 Local/Roaming AppData·설정/캐시/복구·바탕화면/시작 메뉴 바로가기·제거 등록이 삭제됨을 확인했다. 밖에 저장한 프로젝트 SHA와 다른 앱의 합성 데이터는 보존됐다.
- 기존 실제 사용자 데이터 폴더는 없었고 테스트 전 존재하던 빈 설정 key는 제거 성공을 확인한 후 원상 복구했다. 테스트가 기존 설정을 삭제한 채 끝나지 않도록 했다.
- C++ 제품 코드와 CMake는 기존 `d357128` 검증 소스와 동일하다. 기존 44개 전체 회귀 및 정적/메모리 검사 증거를 유지하며 이번에는 제품 계산을 바꾸지 않고 패키징 수용 검사를 수행했다. GitHub Release 공개 업로드와 서명은 하지 않았다.
