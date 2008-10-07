 ; IsDotNETInstalled
 ;
 ; Usage:
 ;   Call IsDotNETInstalled
 ;   Pop $0
 ;   StrCmp $0 1 found.NETFramework no.NETFramework

 Function IsDotNETInstalled
   Push $0
   Push $1
   Push $2
   Push $3
   Push $4

   ReadRegStr $4 HKEY_LOCAL_MACHINE \
     "Software\Microsoft\.NETFramework" "InstallRoot"
   # remove trailing back slash
   Push $4
   Exch $EXEDIR
   Exch $EXEDIR
   Pop $4
   # if the root directory doesn't exist .NET is not installed
   IfFileExists $4 0 noDotNET

   StrCpy $0 0

   EnumStart:

     EnumRegKey $2 HKEY_LOCAL_MACHINE \
       "Software\Microsoft\.NETFramework\Policy"  $0
     IntOp $0 $0 + 1
     StrCmp $2 "" noDotNET

     StrCpy $1 0

     EnumPolicy:

       EnumRegValue $3 HKEY_LOCAL_MACHINE \
         "Software\Microsoft\.NETFramework\Policy\$2" $1
       IntOp $1 $1 + 1
        StrCmp $3 "" EnumStart
         IfFileExists "$4\$2.$3" foundDotNET EnumPolicy

   noDotNET:
     StrCpy $0 0
     Goto done

   foundDotNET:
     StrCpy $0 1

   done:
     Pop $4
     Pop $3
     Pop $2
     Pop $1
     Exch $0
 FunctionEnd
