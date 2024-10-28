@echo off
call startup.cmd %0

echo.
echo %REFDIR%\xboxControllerSpriteFont.png %OUTTESTDIR%\xboxController.spritefont
"%RUN%" %REFDIR%\xboxControllerSpriteFont.png %OUTTESTDIR%\xboxController.spritefont
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo %REFDIR%\xboxOneControllerSpriteFont.png %OUTTESTDIR%\xboxOneController.spritefont
"%RUN%" %REFDIR%\xboxOneControllerSpriteFont.png %OUTTESTDIR%\xboxOneController.spritefont
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
