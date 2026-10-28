Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"

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
  WriteRegDWORD HKCU "${RegistryKey}" "DesktopShortcut" 0
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
FunctionEnd

Section "Uninstall"
  ; Generated explicit file names; never recursively delete the install directory.
  !include "${RemovalScript}"
  Delete "$INSTDIR\Uninstall.exe"
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
  RMDir "$INSTDIR"
SectionEnd
