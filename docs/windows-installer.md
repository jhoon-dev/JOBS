# Windows 설치 파일

Project Planning Manager (PPM)의 Windows x64 일반 배포판은 NSIS 설치 파일 하나로 배포한다. Qt·Python·Office를 별도로 설치할 필요가 없다. 현재 사용자 권한으로 설치하며 기본 위치는 `%LOCALAPPDATA%\Programs\PPM`, 실행 파일은 `PPM.exe`, 시작 메뉴 이름은 PPM이다.

프로젝트는 `.ppm`, 작업공간은 `.ppmworkspace`를 사용한다. 이전 확장자는 지원하지 않는다. 문서 폴더 등 설치 폴더 밖에 파일을 저장한다. 앱 제거는 등록된 설치 폴더 전체와 앱 설정·캐시·복구 자료를 지우며, 외부 프로젝트는 보존한다. 설치 폴더 안에 따로 넣은 파일도 삭제된다.

업그레이드를 위해 기존 `ToolMoa.JOBS` 설치 등록과 데이터 위치는 유지한다. 새 설치 경로의 마지막 폴더 이름은 `PPM`이어야 한다. 기존 등록과 경로가 정확히 일치하는 경우에만 이전 `JOBS` 폴더에서 업그레이드를 허용하고 소유한 이전 실행 파일·바로가기를 정리한다. 다른 파일이 있는 폴더의 새 설치는 거부한다. 제거 시 등록·설치 marker를 검사하고 다른 ToolMoa 앱의 자료는 건드리지 않는다.

## 패키징

Qt 6.8.3/MinGW 13.1과 CMake가 준비된 환경에서 소스 변경을 커밋한 뒤 실행한다.

```powershell
python scripts/package-windows.py
```

산출물 이름은 `dist/PPM-<version>-Setup-win64-<revision>.exe`다. `--qt-root`, `--compiler-bin`, `--nsis-root`, `--build-dir`로 도구 위치를 지정할 수 있다. `packaging/toolchain-lock.json`으로 NSIS 원본·미러·checksum과 compiler SHA를 고정하고, 새 stage에서 Release/x64·필수 DLL·라이선스 원문과 payload SHA를 검증한다. 기존 산출물을 덮어쓰지 않는다. 패키징 manifest는 실제 소스 revision과 산출물 정보를 기록한다.

## 검증

전체 CTest와 Python MSIX 계약 검사를 통과한 뒤 패키징한다. `scripts/check-windows-installer.py`의 설치·실행·제거 검사는 격리된 빈 사용자 환경에서 수행한다. 기존 앱 설정·자료가 있으면 중단한다. 실제 사용자 계정의 자료를 삭제하면서 검증하지 않는다.

과거 JOBS 설치본의 파일명·hash·검증 결과는 당시 버전의 기록이다. 새 PPM 설치본의 검증으로 간주하지 않는다.

- [다중 프로젝트 버전 설치·제거 기록](validation/workspace-2026-10-28/verification.json)
- [이전 설치본 직접 조작 검증](validation/windows-installer-2026-10-28/verification.json)
- [PPM 이름·확장자 계약](ppm-branding.md)
