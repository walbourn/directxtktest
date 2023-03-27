@echo off
call startup.cmd %0

set ADPCMTOOL="%WindowsSdkVerBinPath%\x86\adpcmencode3.exe"
if exist %ADPCMTOOL% goto step1
set ADPCMTOOL=adpcmencode3.exe

:step1
%ADPCMTOOL% %REFDIR%\Alarm01.wav %OUTTESTDIR%\Alarm01ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
%ADPCMTOOL% %REFDIR%\Alarm02.wav %OUTTESTDIR%\Alarm02ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
%ADPCMTOOL% %REFDIR%\Alarm03.wav %OUTTESTDIR%\Alarm03ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -o %OUTTESTDIR%\bankcompact.wxb %OUTTESTDIR%\Alarm01ADPCM.wav %OUTTESTDIR%\Alarm02ADPCM.wav %OUTTESTDIR%\Alarm03ADPCM.wav
"%RUN%" -nologo -o %OUTTESTDIR%\bankcompact.wxb %OUTTESTDIR%\Alarm01ADPCM.wav %OUTTESTDIR%\Alarm02ADPCM.wav %OUTTESTDIR%\Alarm03ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -nc -o %OUTTESTDIR%\bank.wxb %OUTTESTDIR%\Alarm01ADPCM.wav %OUTTESTDIR%\Alarm02ADPCM.wav %OUTTESTDIR%\Alarm03ADPCM.wav
"%RUN%" -nologo -nc -o %OUTTESTDIR%\bank.wxb %OUTTESTDIR%\Alarm01ADPCM.wav %OUTTESTDIR%\Alarm02ADPCM.wav %OUTTESTDIR%\Alarm03ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -f -o %OUTTESTDIR%\banknames.wxb %OUTTESTDIR%\Alarm01ADPCM.wav %OUTTESTDIR%\Alarm02ADPCM.wav %OUTTESTDIR%\Alarm03ADPCM.wav
"%RUN%" -nologo -f -o %OUTTESTDIR%\banknames.wxb %OUTTESTDIR%\Alarm01ADPCM.wav %OUTTESTDIR%\Alarm02ADPCM.wav %OUTTESTDIR%\Alarm03ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo -nologo -s -o %OUTTESTDIR%\bankmix.wxb %REFDIR%\Alarm01.wav %OUTTESTDIR%\Alarm02ADPCM.wav
"%RUN%" -nologo -s -o %OUTTESTDIR%\bankmix.wxb %REFDIR%\Alarm01.wav %OUTTESTDIR%\Alarm02ADPCM.wav
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
