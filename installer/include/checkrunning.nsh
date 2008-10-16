Function CheckTASClientRunning
  FindProcDLL::FindProc "TASClient.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_OK|MB_ICONEXCLAMATION "Please close TASClient before installing."
  Abort
proceed:
  Return
FunctionEnd


Function CheckSpringLobbyRunning
  FindProcDLL::FindProc "springlobby.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_OK|MB_ICONEXCLAMATION "Please close Spring Lobby before installing."
  Abort
proceed:
  Return
FunctionEnd


Function CheckSpringDownloaderRunning
  FindProcDLL::FindProc "SpringDownloader.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_OK|MB_ICONEXCLAMATION "Please close Spring Downloader before installing."
  Abort
proceed:
  Return
FunctionEnd


Function CheckCADownloaderRunning
  FindProcDLL::FindProc "CADownloader.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_OK|MB_ICONEXCLAMATION "Please close CA Downloader before installing."
  Abort
proceed:
  Return
FunctionEnd


Function CheckSpringSettingsRunning
  FindProcDLL::FindProc "springsettings.exe"
  ; $R0 == 1: process found
  ;        0: process not found
  ;       >1: some error, assume everything's ok
  IntCmp $R0 1 do_abort proceed proceed
do_abort:
  MessageBox MB_OK|MB_ICONEXCLAMATION "Please close Spring Settings before installing."
  Abort
proceed:
  Return
FunctionEnd
