@echo off

rem -----------------------------------------------------
rem Tiny batch script to build the input file revision.h
rem revision.h contains the revision number of the wc.
rem It is included by assimp.rc.
rem -----------------------------------------------------

rem This is not very elegant, but it works.
rem ./bin shouldn't have any local modifications

cd ..\bin
svnversion > tmpfile.txt
set /p addtext= < tmpfile.txt
del /q tmpfile.txt
cd ..\mkutil

echo #define SVNRevision > tmpfile.txt

if exist revision.h del /q revision.h
for /f "delims=" %%l in (tmpfile.txt) Do (
      echo %%l %addtext% >> revision.h
)
del /q tmpfile.txt



