Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "FileFunc.nsh"

Name "JOBS ${AppVersion} Preview"
OutFile "${InstallerPath}"
InstallDir "$LOCALAPPDATA\Programs\JOBS"
RequestExecutionLevel user
SetCompressor /SOLID lzma
VIProductVersion "${AppVersion}.0"
VIAddVersionKey "ProductName" "JOBS"
VIAddVersionKey "FileDescription" "JOBS Windows x64 Preview Setup"
VIAddVersionKey "FileVersion" "${AppVersion}"
VIAddVersionKey "ProductVersion" "${AppVersion}"
VIAddVersionKey "LegalCopyright" "ToolMoa"

!define RegistryKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\ToolMoa.JOBS"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\JOBS.exe"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!define MUI_UNCONFIRMPAGE_TEXT_TOP "JOBS 설치 폴더의 모든 파일과 설정·캐시·자동 복구 데이터를 삭제합니다. 설치 폴더 밖에 따로 저장한 프로젝트 파일은 유지합니다. 제거 전에 JOBS 창을 닫아 주세요."
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Korean"

Function .onInit
  SetShellVarContext current
  SetRegView 64
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "Windows 64비트가 필요합니다." /SD IDOK
    SetErrorLevel 2
    Abort
  ${EndIf}
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_ICONSTOP "Windows 10 이상이 필요합니다." /SD IDOK
    SetErrorLevel 2
    Abort
  ${EndIf}
FunctionEnd

; A silent /D= path is subject to the same check as the directory page.
Function VerifyInstallDirectory
  ; Unlike the NSIS instruction, this Win32 API accepts not-yet-created parents.
  System::Call 'kernel32::GetFullPathNameW(w "$INSTDIR", i ${NSIS_MAX_STRLEN}, w .r2, p 0) i .r3'
  ${If} $3 == 0
  ${OrIf} $3 >= ${NSIS_MAX_STRLEN}
    SetErrorLevel 3
    Quit
  ${EndIf}
  StrCpy $INSTDIR $2
  ${GetFileName} "$INSTDIR" $2
  StrCmp $2 "JOBS" valid_name
    MessageBox MB_ICONSTOP "설치 전용 JOBS 폴더를 선택해 주세요. 예: D:\Apps\JOBS" /SD IDOK
    SetErrorLevel 3
    Quit
  valid_name:
  ReadRegStr $2 HKCU "${RegistryKey}" "InstallLocation"
  StrCmp $2 "" check_contents
  StrCmp $2 "$INSTDIR" check_contents
    MessageBox MB_ICONSTOP "기존 JOBS를 먼저 제거한 뒤 다른 위치에 설치해 주세요." /SD IDOK
    SetErrorLevel 3
    Quit
  check_contents:
  ReadINIStr $0 "$INSTDIR\install-info.ini" "JOBS" "ProductId"
  StrCmp $0 "ToolMoa.JOBS" verified
  FindFirst $0 $1 "$INSTDIR\*"
  loop:
    StrCmp $1 "" empty
    StrCmp $1 "." next
    StrCmp $1 ".." next
    FindClose $0
    MessageBox MB_ICONSTOP "빈 폴더 또는 기존 JOBS 설치 폴더를 선택해 주세요." /SD IDOK
    SetErrorLevel 3
    Quit
  next:
    FindNext $0 $1
    Goto loop
  empty:
    FindClose $0
  verified:
FunctionEnd

Section "JOBS (필수)" MainSection
  SectionIn RO
  Call VerifyInstallDirectory
  SetOutPath "$INSTDIR"
  File /r "${StageDir}\*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\JOBS"
  CreateShortcut "$SMPROGRAMS\JOBS\JOBS.lnk" "$INSTDIR\JOBS.exe"
  CreateShortcut "$SMPROGRAMS\JOBS\JOBS 제거.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "${RegistryKey}" "DisplayName" "JOBS ${AppVersion} Preview"
  WriteRegStr HKCU "${RegistryKey}" "DisplayVersion" "${AppVersion}"
  WriteRegStr HKCU "${RegistryKey}" "Publisher" "ToolMoa"
  WriteRegStr HKCU "${RegistryKey}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${RegistryKey}" "DisplayIcon" "$INSTDIR\JOBS.exe"
  WriteRegStr HKCU "${RegistryKey}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegStr HKCU "${RegistryKey}" "QuietUninstallString" '$\"$INSTDIR\Uninstall.exe$\" /S'
  WriteRegDWORD HKCU "${RegistryKey}" "NoModify" 1
  WriteRegDWORD HKCU "${RegistryKey}" "NoRepair" 1
  WriteRegDWORD HKCU "${RegistryKey}" "EstimatedSize" ${InstalledSizeKiB}
  ReadRegDWORD $0 HKCU "${RegistryKey}" "DesktopShortcut"
  ${If} $0 != 1
    WriteRegDWORD HKCU "${RegistryKey}" "DesktopShortcut" 0
  ${EndIf}
SectionEnd

Section /o "바탕화면 바로가기" DesktopSection
  CreateShortcut "$DESKTOP\JOBS.lnk" "$INSTDIR\JOBS.exe"
  WriteRegDWORD HKCU "${RegistryKey}" "DesktopShortcut" 1
SectionEnd

Function un.onInit
  SetShellVarContext current
  SetRegView 64
  ReadINIStr $0 "$INSTDIR\install-info.ini" "JOBS" "ProductId"
  StrCmp $0 "ToolMoa.JOBS" verified
  SetErrorLevel 3
  Abort
  verified:
  GetFullPathName $INSTDIR "$INSTDIR"
  ReadRegStr $0 HKCU "${RegistryKey}" "InstallLocation"
  StrCmp $0 "$INSTDIR" registered invalid_directory
  registered:
  ${GetFileName} "$INSTDIR" $0
  StrCmp $0 "JOBS" directory_ok invalid_directory
  invalid_directory:
    MessageBox MB_ICONSTOP "등록된 JOBS 설치 폴더와 일치하지 않아 제거를 중단합니다." /SD IDOK
    SetErrorLevel 3
    Abort
  directory_ok:
  ClearErrors
  FileOpen $0 "$INSTDIR\JOBS.exe" a
  IfErrors app_running app_closed
  app_running:
    MessageBox MB_ICONSTOP "실행 중인 JOBS 창을 닫고 다시 제거해 주세요." /SD IDOK
    SetErrorLevel 4
    Abort
  app_closed:
    FileClose $0
FunctionEnd

Section "Uninstall"
  ; Fixed application-specific data roots; unrelated ToolMoa applications are kept.
  GetFullPathName $0 "$LOCALAPPDATA\ToolMoa\JOBS"
  StrCmp $0 "" local_data_done
  RMDir /r "$0"
  local_data_done:
  GetFullPathName $0 "$APPDATA\ToolMoa\JOBS"
  StrCmp $0 "" roaming_data_done
  RMDir /r "$0"
  roaming_data_done:
  RMDir "$LOCALAPPDATA\ToolMoa"
  RMDir "$APPDATA\ToolMoa"
  DeleteRegKey HKCU "Software\ToolMoa\JOBS"
  DeleteRegKey /ifempty HKCU "Software\ToolMoa"
  ReadRegStr $0 HKCU "${RegistryKey}" "InstallLocation"
  StrCmp $0 "$INSTDIR" own_registration keep_registration
  own_registration:
    ReadRegDWORD $1 HKCU "${RegistryKey}" "DesktopShortcut"
    ${If} $1 == 1
      Delete "$DESKTOP\JOBS.lnk"
    ${EndIf}
    DeleteRegKey HKCU "${RegistryKey}"
    Delete "$SMPROGRAMS\JOBS\JOBS.lnk"
    Delete "$SMPROGRAMS\JOBS\JOBS 제거.lnk"
    RMDir "$SMPROGRAMS\JOBS"
  keep_registration:
  ; The canonical directory, marker and per-user registration were checked above.
  RMDir /r "$INSTDIR"
SectionEnd
