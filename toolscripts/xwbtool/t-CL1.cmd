@echo off
call startup.cmd %0

REM ** Verify usage works.  need to run without debugger so we get consistent output

"%RUNNODBG%" --help > "%STDLOG%"
