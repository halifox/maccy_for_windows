# Patch the CPack template shipped with the active CMake installation instead
# of vendoring a version-specific copy. CPack searches CPACK_MODULE_PATH before
# its built-in templates, so this keeps the custom pages aligned with CMake.
set(_maccy_default_nsis_template
    "${CMAKE_ROOT}/Modules/Internal/CPack/NSIS.template.in")
if(NOT EXISTS "${_maccy_default_nsis_template}")
    message(FATAL_ERROR "CPack's NSIS.template.in was not found under CMAKE_ROOT.")
endif()

file(READ "${_maccy_default_nsis_template}" _maccy_nsis_template)
string(REPLACE "\r\n" "\n" _maccy_nsis_template "${_maccy_nsis_template}")

function(maccy_replace_template_text variable needle replacement description)
    string(FIND "${${variable}}" "${needle}" _match)
    if(_match EQUAL -1)
        message(FATAL_ERROR "Cannot customize CPack NSIS template: missing ${description}.")
    endif()
    string(REPLACE "${needle}" "${replacement}" _updated "${${variable}}")
    set(${variable} "${_updated}" PARENT_SCOPE)
endfunction()

set(_maccy_nsis_runtime_code [=[
!define MACCY_SHUTDOWN_MESSAGE 0x8008
!define MACCY_ACTIVATION_WINDOW_CLASS "Maccy.SingleInstance.ActivationWindow"
!define MACCY_INSTANCE_MUTEX "Local\org.maccy.windows.ClipboardManager"

!macro MACCY_STOP_RUNNING_APP prefix
  Push $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5
  Push $6

  System::Call 'user32::FindWindowExW(p -3, p 0, w "${MACCY_ACTIVATION_WINDOW_CLASS}", p 0) p .r0'
  StrCmp $0 0 ${prefix}_check_mutex
  Goto ${prefix}_have_window

${prefix}_check_mutex:
  System::Call 'kernel32::OpenMutexW(i 0x00100000, i 0, w "${MACCY_INSTANCE_MUTEX}") p .r1'
  StrCmp $1 0 ${prefix}_done
  System::Call 'kernel32::WaitForSingleObject(p r1, i 10000) i .r2'
  System::Call 'kernel32::CloseHandle(p r1)'
  StrCmp $2 0 ${prefix}_done
  StrCmp $2 128 ${prefix}_done
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(MACCY_APP_NOT_RESPONDING)"
  Goto ${prefix}_abort

${prefix}_have_window:
  System::Call 'user32::GetWindowThreadProcessId(p r0, *i .r1) i .r2'
  StrCmp $1 0 ${prefix}_abort
  System::Call 'kernel32::OpenProcess(i 0x00100000, i 0, i r1) p .r3'
  StrCmp $3 0 ${prefix}_access_denied
  System::Call 'user32::SendMessageTimeoutW(p r0, i ${MACCY_SHUTDOWN_MESSAGE}, p 0, p 0, i 3, i 5000, *p .r4) p .r5'

${prefix}_wait:
  System::Call 'kernel32::WaitForSingleObject(p r3, i 10000) i .r6'
  StrCmp $6 0 ${prefix}_closed
  StrCmp $6 258 ${prefix}_still_running
  Goto ${prefix}_wait_failed

${prefix}_still_running:
  MessageBox MB_YESNOCANCEL|MB_ICONEXCLAMATION "$(MACCY_APP_CLOSE_PROMPT)" IDYES ${prefix}_force IDNO ${prefix}_wait
  Goto ${prefix}_cancel
${prefix}_force:
  System::Call 'kernel32::OpenProcess(i 0x00000001, i 0, i r1) p .r5'
  StrCmp $5 0 ${prefix}_force_open_failed
  System::Call 'kernel32::TerminateProcess(p r5, i 1) i .r6'
  System::Call 'kernel32::CloseHandle(p r5)'
  StrCmp $6 1 0 ${prefix}_force_failed
  System::Call 'kernel32::WaitForSingleObject(p r3, i 5000) i .r6'
  StrCmp $6 0 ${prefix}_closed
  Goto ${prefix}_force_failed

${prefix}_force_open_failed:
  System::Call 'kernel32::WaitForSingleObject(p r3, i 0) i .r6'
  StrCmp $6 0 ${prefix}_closed
  Goto ${prefix}_force_failed

${prefix}_closed:
  System::Call 'kernel32::CloseHandle(p r3)'
  Goto ${prefix}_done

${prefix}_access_denied:
  System::Call 'user32::SendMessageTimeoutW(p r0, i ${MACCY_SHUTDOWN_MESSAGE}, p 0, p 0, i 3, i 5000, *p .r4) p .r5'
  MessageBox MB_OK|MB_ICONEXCLAMATION "$(MACCY_APP_ACCESS_DENIED)"
  Goto ${prefix}_abort

${prefix}_wait_failed:
  System::Call 'kernel32::CloseHandle(p r3)'
${prefix}_force_failed:
  MessageBox MB_OK|MB_ICONSTOP "$(MACCY_APP_FORCE_FAILED)"
  Goto ${prefix}_abort

${prefix}_cancel:
  System::Call 'kernel32::CloseHandle(p r3)'
  Goto ${prefix}_abort

${prefix}_abort:
  Quit

${prefix}_done:
  Pop $6
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Pop $0
!macroend

Function MaccyStopRunningApp
  !insertmacro MACCY_STOP_RUNNING_APP "maccy_install"
FunctionEnd

Function un.MaccyStopRunningApp
  !insertmacro MACCY_STOP_RUNNING_APP "maccy_uninstall"
FunctionEnd

Function MaccyDirectoryPagePre
  StrCmp "$MACCY_UPGRADE_FOUND" "1" 0 +2
    Abort
FunctionEnd

LangString MACCY_APP_NOT_RESPONDING ${LANG_ENGLISH} "Maccy is starting or closing but has not made its installer control window available. Wait for it to finish, then retry."
LangString MACCY_APP_NOT_RESPONDING ${LANG_SIMPCHINESE} "Maccy 正在启动或退出，但暂时无法响应安装程序。请等待操作完成后重试。"
LangString MACCY_APP_CLOSE_PROMPT ${LANG_ENGLISH} "Maccy is still running. Yes: force close it; No: wait another 10 seconds; Cancel: stop setup."
LangString MACCY_APP_CLOSE_PROMPT ${LANG_SIMPCHINESE} "Maccy 仍在运行。选择“是”强制结束，“否”再等待 10 秒，“取消”则停止安装或卸载。"
LangString MACCY_APP_ACCESS_DENIED ${LANG_ENGLISH} "The installer could not wait for or safely close Maccy. Close it manually and retry."
LangString MACCY_APP_ACCESS_DENIED ${LANG_SIMPCHINESE} "安装程序无法等待或安全关闭 Maccy。请手动关闭程序后重试。"
LangString MACCY_APP_FORCE_FAILED ${LANG_ENGLISH} "Maccy could not be closed. Setup has been cancelled."
LangString MACCY_APP_FORCE_FAILED ${LANG_SIMPCHINESE} "无法关闭 Maccy，安装或卸载已取消。"
LangString MACCY_MACHINE_INSTALL_MESSAGE ${LANG_ENGLISH} "A previous all-users Maccy installation is present. Uninstall it from Windows Settings using an administrator account, then run this per-user installer again. Your data under LocalAppData is kept by the old uninstaller."
LangString MACCY_MACHINE_INSTALL_MESSAGE ${LANG_SIMPCHINESE} "检测到旧版全机安装。请使用管理员账户在 Windows 设置中卸载旧版，再重新运行此每用户安装程序。旧卸载程序会保留 LocalAppData 中的数据。"
LangString MACCY_BROKEN_INSTALL_MESSAGE ${LANG_ENGLISH} "A previous per-user Maccy installation has incomplete uninstall registration. Use its existing Uninstall.exe before installing again."
LangString MACCY_BROKEN_INSTALL_MESSAGE ${LANG_SIMPCHINESE} "检测到旧版每用户安装，但卸载记录不完整。请先运行原安装目录中的 Uninstall.exe。"
LangString MACCY_UNINSTALL_DATA_NOTICE ${LANG_ENGLISH} "Maccy keeps clipboard history and settings in your local user profile. They are kept unless you choose to remove them."
LangString MACCY_UNINSTALL_DATA_NOTICE ${LANG_SIMPCHINESE} "Maccy 会在当前用户配置文件中保存剪贴板历史和设置。默认保留，只有勾选后才会删除。"
LangString MACCY_UNINSTALL_DATA_OPTION ${LANG_ENGLISH} "Delete clipboard history, pinned items, settings, and other Maccy data"
LangString MACCY_UNINSTALL_DATA_OPTION ${LANG_SIMPCHINESE} "删除剪贴板历史、固定项目、设置及其他 Maccy 数据"
LangString MACCY_UNINSTALL_DATA_DELETE_FAILED ${LANG_ENGLISH} "Some Maccy user data could not be removed. Close any remaining Maccy processes and remove the folder manually: $LOCALAPPDATA\maccy"
LangString MACCY_UNINSTALL_DATA_DELETE_FAILED ${LANG_SIMPCHINESE} "部分 Maccy 用户数据无法删除。请关闭所有 Maccy 进程后手动删除：$LOCALAPPDATA\maccy"

Function un.MaccyUserDataPage
  nsDialogs::Create 1018
  Pop $0
  StrCmp $0 error 0 +2
    Abort
  ${NSD_CreateLabel} 0 0 100% 30u "$(MACCY_UNINSTALL_DATA_NOTICE)"
  Pop $1
  ${NSD_CreateCheckbox} 0 38u 100% 32u "$(MACCY_UNINSTALL_DATA_OPTION)"
  Pop $MACCY_DELETE_DATA_CHECKBOX
  nsDialogs::Show
FunctionEnd

Function un.MaccyUserDataPageLeave
  ${NSD_GetState} $MACCY_DELETE_DATA_CHECKBOX $MACCY_DELETE_USER_DATA
FunctionEnd

]=])

maccy_replace_template_text(
    _maccy_nsis_template
    "RequestExecutionLevel admin"
    "RequestExecutionLevel user"
    "the requested execution level")
maccy_replace_template_text(
    _maccy_nsis_template
    "  !include \"MUI.nsh\""
    "  !include \"MUI.nsh\"\n  !include \"nsDialogs.nsh\""
    "the MUI include")
maccy_replace_template_text(
    _maccy_nsis_template
    ";Require administrator access"
    ";Run with the current user's permissions"
    "the execution-level comment")
maccy_replace_template_text(
    _maccy_nsis_template
    "  Var IS_DEFAULT_INSTALLDIR"
    "  Var IS_DEFAULT_INSTALLDIR\n  Var MACCY_UPGRADE_FOUND\n  Var MACCY_DELETE_USER_DATA\n  Var MACCY_DELETE_DATA_CHECKBOX"
    "the installer variable declarations")
maccy_replace_template_text(
    _maccy_nsis_template
    "  !insertmacro MUI_PAGE_DIRECTORY"
    "  !define MUI_PAGE_CUSTOMFUNCTION_PRE MaccyDirectoryPagePre\n  !insertmacro MUI_PAGE_DIRECTORY\n  !undef MUI_PAGE_CUSTOMFUNCTION_PRE"
    "the install directory page")
maccy_replace_template_text(
    _maccy_nsis_template
    "  !insertmacro MUI_UNPAGE_CONFIRM"
    "  !insertmacro MUI_UNPAGE_CONFIRM\n  UninstPage custom un.MaccyUserDataPage un.MaccyUserDataPageLeave"
    "the uninstall confirmation page")
maccy_replace_template_text(
    _maccy_nsis_template
    ";Reserve Files"
    "${_maccy_nsis_runtime_code}\n\n;Reserve Files"
    "the NSIS reserve-files section")

string(FIND "${_maccy_nsis_template}" "Function .onInit\n" _maccy_on_init_start)
if(_maccy_on_init_start EQUAL -1)
    message(FATAL_ERROR "Cannot customize CPack NSIS template: installer .onInit was not found.")
endif()
string(SUBSTRING "${_maccy_nsis_template}" ${_maccy_on_init_start} -1 _maccy_on_init_tail)
string(FIND "${_maccy_on_init_tail}" "FunctionEnd" _maccy_on_init_end)
if(_maccy_on_init_end EQUAL -1)
    message(FATAL_ERROR "Cannot customize CPack NSIS template: installer .onInit end was not found.")
endif()
string(LENGTH "FunctionEnd" _maccy_function_end_length)
math(EXPR _maccy_on_init_length "${_maccy_on_init_end} + ${_maccy_function_end_length}")
string(SUBSTRING "${_maccy_nsis_template}" ${_maccy_on_init_start} ${_maccy_on_init_length} _maccy_old_on_init)

set(_maccy_new_on_init [=[
Function .onInit
  SetShellVarContext current
  StrCpy $SV_ALLUSERS "JustMe"
  StrCpy $MACCY_UPGRADE_FOUND "0"
  StrCpy $INSTDIR "@CPACK_NSIS_INSTALL_ROOT@\@CPACK_PACKAGE_INSTALL_DIRECTORY@"

  ReadRegStr $1 HKCU "Software\@CPACK_PACKAGE_VENDOR@\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@" ""
  StrCmp "$1" "" check_machine_install
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@" "UninstallString"
  StrCmp "$0" "" broken_user_install
  StrCpy $INSTDIR "$1"
  StrCpy $MACCY_UPGRADE_FOUND "1"
  Goto init_done

check_machine_install:
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@" "UninstallString"
  StrCmp "$0" "" init_done
  MessageBox MB_OK|MB_ICONSTOP "$(MACCY_MACHINE_INSTALL_MESSAGE)"
  Abort

broken_user_install:
  MessageBox MB_OK|MB_ICONSTOP "$(MACCY_BROKEN_INSTALL_MESSAGE)"
  Abort

init_done:
  StrCmp "@CPACK_NSIS_MODIFY_PATH@" "ON" 0 noOptionsPage
    !insertmacro MUI_INSTALLOPTIONS_EXTRACT "NSIS.InstallOptions.ini"
  noOptionsPage:
FunctionEnd
]=])
string(REPLACE "${_maccy_old_on_init}" "${_maccy_new_on_init}" _maccy_nsis_template "${_maccy_nsis_template}")

string(FIND "${_maccy_nsis_template}" "Function un.onInit\n" _maccy_un_on_init_start)
if(_maccy_un_on_init_start EQUAL -1)
    message(FATAL_ERROR "Cannot customize CPack NSIS template: uninstaller .onInit was not found.")
endif()
string(SUBSTRING "${_maccy_nsis_template}" ${_maccy_un_on_init_start} -1 _maccy_un_on_init_tail)
string(FIND "${_maccy_un_on_init_tail}" "FunctionEnd" _maccy_un_on_init_end)
if(_maccy_un_on_init_end EQUAL -1)
    message(FATAL_ERROR "Cannot customize CPack NSIS template: uninstaller .onInit end was not found.")
endif()
math(EXPR _maccy_un_on_init_length "${_maccy_un_on_init_end} + ${_maccy_function_end_length}")
string(SUBSTRING "${_maccy_nsis_template}" ${_maccy_un_on_init_start} ${_maccy_un_on_init_length} _maccy_old_un_on_init)
set(_maccy_new_un_on_init [=[
Function un.onInit
  SetShellVarContext current
  StrCpy $MACCY_DELETE_USER_DATA "0"
FunctionEnd
]=])
string(REPLACE "${_maccy_old_un_on_init}" "${_maccy_new_un_on_init}" _maccy_nsis_template "${_maccy_nsis_template}")

set(_maccy_cpack_module_dir "${CMAKE_CURRENT_BINARY_DIR}/cpack-modules")
file(MAKE_DIRECTORY "${_maccy_cpack_module_dir}")
file(WRITE "${_maccy_cpack_module_dir}/NSIS.template.in" "${_maccy_nsis_template}")
set(CPACK_MODULE_PATH "${_maccy_cpack_module_dir}")
