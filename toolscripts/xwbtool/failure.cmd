@echo off
setLocal

set ERRMSG=ERROR: [%TEST%] %1

echo %ERRMSG%
if exist %DASHLOG% ( type %DASHLOG% )
if exist %DIFFLOG% ( type %DIFFLOG% )
if exist %FAILURELOG% ( echo %ERRMSG% >> %FAILURELOG% ) else ( echo %ERRMSG% > %FAILURELOG% )

endLocal
set ERRORLEVEL=1
exit 1
