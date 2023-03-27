@echo off
call startup.cmd %0

echo.
echo -nologo -o %OUTTESTDIR%\bankcompact.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
"%RUN%" -nologo -o %OUTTESTDIR%\bankcompact.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -nc -o %OUTTESTDIR%\bank.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
"%RUN%" -nologo -nc -o %OUTTESTDIR%\bank.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -f -o %OUTTESTDIR%\banknames.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
"%RUN%" -nologo -f -o %OUTTESTDIR%\banknames.wxb %REFDIR%\Alarm01.wav %REFDIR%\Alarm02.wav %REFDIR%\Alarm03.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -o %OUTTESTDIR%\bankbig.wxb -h %OUTTESTDIR%\bankbig.h %REFDIR%\Alarm*.wav
"%RUN%" -nologo -o %OUTTESTDIR%\bankbig.wxb -h %OUTTESTDIR%\bankbig.h %REFDIR%\Alarm*.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
