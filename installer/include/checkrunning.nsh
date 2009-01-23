Function CheckTASClientRunning
begin:
  FindProcDLL::FindProc "TASClient.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Please close TASClient before installing." IDRETRY begin
  Abort
proceed:
  Return
FunctionEnd


Function CheckSpringLobbyRunning
begin:
  FindProcDLL::FindProc "springlobby.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Please close Spring Lobby before installing." IDRETRY begin
  Abort
proceed:
  Return
FunctionEnd


Function CheckSpringDownloaderRunning
begin:
  FindProcDLL::FindProc "SpringDownloader.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Please close Spring Downloader before installing." IDRETRY begin
  Abort
proceed:
  Return
FunctionEnd


Function CheckCADownloaderRunning
begin:
  FindProcDLL::FindProc "CADownloader.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Please close CA Downloader before installing." IDRETRY begin
  Abort
proceed:
  Return
FunctionEnd


Function CheckSpringSettingsRunning
begin:
  FindProcDLL::FindProc "springsettings.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Please close Spring Settings before installing." IDRETRY begin
  Abort
proceed:
  Return
FunctionEnd
