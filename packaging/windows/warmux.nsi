; Warmux installer for Windows (64-bit), built by build-windows.sh:
;   makensis -DVERSION=<x.y.z> -DSTAGE=<staged files> -DOUTFILE=<setup.exe> \
;            -DUNINSTALL_LIST=<generated .nsh> warmux.nsi
; STAGE is the folder that also goes into the portable zip. UNINSTALL_LIST
; deletes exactly those files, so uninstalling never touches anything else
; in the install folder.

Unicode true
!pragma warning disable 6040   ; LangString missing for a language: English is used
ManifestDPIAware true
SetCompressor /SOLID lzma
RequestExecutionLevel admin

!include "MUI2.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"

!define APP       "Warmux"
!define PUBLISHER "KennyStier and the Warmux team"
!define HOMEPAGE  "https://github.com/KennyStier/warmux"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"

Name "${APP}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\${APP}"
InstallDirRegKey HKLM "${UNINST_KEY}" "InstallLocation"
BrandingText "${APP} ${VERSION}"

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=0 "ProductName" "${APP}"
VIAddVersionKey /LANG=0 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=0 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=0 "FileDescription" "${APP} installer"
VIAddVersionKey /LANG=0 "CompanyName" "${PUBLISHER}"
VIAddVersionKey /LANG=0 "LegalCopyright" "GPL-2.0-or-later"

; --- Pages ------------------------------------------------------------------
!define MUI_ICON   "${STAGE}/data/icon/warmux.ico"
!define MUI_UNICON "${STAGE}/data/icon/uninstall.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${__FILEDIR__}/welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${__FILEDIR__}/welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP         "${__FILEDIR__}/header.bmp"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${STAGE}/COPYING.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\warmux.exe"
; The "show readme" checkbox, repurposed for the desktop shortcut
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT "$(DesktopShortcut)"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION CreateDesktopShortcut
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; --- Languages: Windows' display language is picked automatically ----------
; (the ones Warmux is translated into that NSIS also supports)
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Catalan"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Esperanto"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"

; Our one custom string; languages without a translation fall back to English
LangString DesktopShortcut ${LANG_ENGLISH}    "Create a desktop shortcut"
LangString DesktopShortcut ${LANG_GERMAN}     "Verknüpfung auf dem Desktop erstellen"
LangString DesktopShortcut ${LANG_FRENCH}     "Créer un raccourci sur le bureau"
LangString DesktopShortcut ${LANG_SPANISH}    "Crear un acceso directo en el escritorio"
LangString DesktopShortcut ${LANG_ITALIAN}    "Crea un collegamento sul desktop"
LangString DesktopShortcut ${LANG_DUTCH}      "Snelkoppeling op het bureaublad maken"
LangString DesktopShortcut ${LANG_PORTUGUESEBR} "Criar um atalho na área de trabalho"
LangString DesktopShortcut ${LANG_PORTUGUESE} "Criar um atalho no ambiente de trabalho"
LangString DesktopShortcut ${LANG_POLISH}     "Utwórz skrót na pulpicie"
LangString DesktopShortcut ${LANG_RUSSIAN}    "Создать ярлык на рабочем столе"

LangString Need64Bit ${LANG_ENGLISH} "Warmux needs a 64-bit version of Windows."

; --- Install ----------------------------------------------------------------
Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "$(Need64Bit)"
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd

Section "Warmux" SecMain
  SectionIn RO
  SetShellVarContext all

  ; Upgrading: remove the previous version's files first, so files dropped
  ; from the new version don't linger. Settings in %APPDATA% are kept.
  ReadRegStr $0 HKLM "${UNINST_KEY}" "UninstallString"
  ReadRegStr $1 HKLM "${UNINST_KEY}" "InstallLocation"
  ${If} $0 != ""
  ${AndIf} ${FileExists} "$1\uninstall.exe"
    ExecWait '"$1\uninstall.exe" /S _?=$1'
    Delete "$1\uninstall.exe"
  ${EndIf}

  SetOutPath "$INSTDIR"
  File /r "${STAGE}/*"

  WriteUninstaller "$INSTDIR\uninstall.exe"
  CreateShortcut "$SMPROGRAMS\${APP}.lnk" "$INSTDIR\warmux.exe"

  ; Settings > Apps entry
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"     "${APP}"
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"  "${VERSION}"
  WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"       "${PUBLISHER}"
  WriteRegStr   HKLM "${UNINST_KEY}" "URLInfoAbout"    "${HOMEPAGE}"
  WriteRegStr   HKLM "${UNINST_KEY}" "HelpLink"        "${HOMEPAGE}/issues"
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\warmux.exe"
  WriteRegStr   HKLM "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize" $0
SectionEnd

Function CreateDesktopShortcut
  SetShellVarContext all
  CreateShortcut "$DESKTOP\${APP}.lnk" "$INSTDIR\warmux.exe"
FunctionEnd

; --- Uninstall --------------------------------------------------------------
Function un.onInit
  SetRegView 64
FunctionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$SMPROGRAMS\${APP}.lnk"
  Delete "$DESKTOP\${APP}.lnk"

  !include "${UNINSTALL_LIST}"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"   ; only if empty: never removes the user's own files

  DeleteRegKey HKLM "${UNINST_KEY}"
SectionEnd
