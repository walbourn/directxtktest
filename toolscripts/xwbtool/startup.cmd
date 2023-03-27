@echo off
REM ** Verifies some basics that every test needs to handle

:checkcmd
if '"%TARGETEXE%"' NEQ '' goto checkexe
echo TARGETEXE is not set to anything, don't know how to find target executable
exit -1

:checkexe
if exist "%TARGETEXE%" goto allgood
echo "%TARGETEXE%" does not exist, cannot run target executable
exit -2

:allgood

set TEST=%~n1
set REFFILE=%REFDIR%\%~n1
set OUTFILE=%OUTDIR%\%~n1
set DBGLOG=%OUTFILE%-dbg.log
set ERRLOG=%OUTFILE%-stderr.log
set STDLOG=%OUTFILE%-stdout.log
set REFERRLOG=%REFFILE%-stderr.log
set REFLOG=%REFFILE%-stdout.log
set DIFFLOG=%OUTFILE%-difflog.log

set OUTTESTDIR=%OUTDIR%\%~n1

set RUNNODBG=%TARGETEXE%
REM set RUN=windbg -o -xn av -xn dz -xn iov -xn eh -c ".outmask- /l 0xFFFFFF7F;g;k;.lastevent;q" -loga "%DBGLOG%" "%TARGETEXE%""
set RUN=%RUNNODBG%

if not exist "%OUTDIR%" md "%OUTDIR%"
if not exist "%OUTTESTDIR%" md "%OUTTESTDIR%"
if exist "%DBGLOG%" del /f "%DBGLOG%"

