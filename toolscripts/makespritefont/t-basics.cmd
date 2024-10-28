@echo off
call startup.cmd %0

echo.
echo "Comic Sans MS" %OUTTESTDIR%\myfile.spritefont /FontSize:16
"%RUN%" "Comic Sans MS" %OUTTESTDIR%\myfile.spritefont /FontSize:16
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Courier New" %OUTTESTDIR%\Courier_16.spritefont /FontSize:16
"%RUN%" "Courier New" %OUTTESTDIR%\Courier_16.spritefont /FontSize:16
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Courier New" %OUTTESTDIR%\Courier_36.spritefont /FontSize:36
"%RUN%" "Courier New" %OUTTESTDIR%\Courier_36.spritefont /FontSize:36
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_18.spritefont /FontSize:18
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_18.spritefont /FontSize:18
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_18_Bold.spritefont /FontSize:18 /FontStyle:Bold
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_18_Bold.spritefont /FontSize:18 /FontStyle:Bold
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_18_Italic.spritefont /FontSize:18 /FontStyle:Italic
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_18_Italic.spritefont /FontSize:18 /FontStyle:Italic
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_22.spritefont /FontSize:22
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_22.spritefont /FontSize:22
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_22_Bold.spritefont /FontSize:22 /FontStyle:Bold
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_22_Bold.spritefont /FontSize:22 /FontStyle:Bold
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_22_Italic.spritefont /FontSize:22 /FontStyle:Italic
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_22_Italic.spritefont /FontSize:22 /FontStyle:Italic
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_36.spritefont /FontSize:36
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_36.spritefont /FontSize:36
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_36_Bold.spritefont /FontSize:36 /FontStyle:Bold
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_36_Bold.spritefont /FontSize:36 /FontStyle:Bold
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\SegoeUI_36_Italic.spritefont /FontSize:36 /FontStyle:Italic
"%RUN%" "Segoe UI" %OUTTESTDIR%\SegoeUI_36_Italic.spritefont /FontSize:36 /FontStyle:Italic
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
