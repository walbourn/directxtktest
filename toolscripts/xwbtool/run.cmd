@echo off

setlocal

set TARGETEXE=..\..\..\out\build\x64-Debug\bin\xwbtool.exe
REM set TARGETEXE=..\..\..\xwbtool\Bin\Desktop_2019\Win32\Debug\xwbtool.exe
REM set TARGETEXE=..\..\..\xwbtool\Bin\Desktop_2019\Win32\Release\xwbtool.exe
REM set TARGETEXE=D:\vcpkg\installed\arm64-windows\tools\directxtk\xwbtool.exe
set SPEC=*

call onetime.cmd

:args
if '%1' EQU '' goto ready
if '%~x1' NEQ '' set TARGETEXE=%1
if '%~x1' EQU '' set SPEC=%1

:ready
echo Target Exe: %TARGETEXE%
echo   Run Spec: %SPEC%

for %%f in ("%~dp0t-%SPEC%.cmd") do (echo. && echo %%~nf ... && cmd.exe /C "%%f")

if exist "%FAILURELOG%" ( echo. && echo FAILURE LOG & type "%FAILURELOG%" )

endlocal
