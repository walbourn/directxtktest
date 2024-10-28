@echo off
call startup.cmd %0

echo.
echo -nologo -o %OUTTESTDIR%\bankcompact.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
"%RUN%" -nologo -o %OUTTESTDIR%\bankcompact.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo --no-compact -o %OUTTESTDIR%\bank.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
"%RUN%" -nologo --no-compact -o %OUTTESTDIR%\bank.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo --friendly-names -o %OUTTESTDIR%\banknames.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
"%RUN%" -nologo --friendly-names -o %OUTTESTDIR%\banknames.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
