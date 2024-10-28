@echo off
call startup.cmd %0

echo.
echo "Segoe UI" %OUTTESTDIR%\auto.spritefont /FontSize:18 /TextureFormat:Auto
"%RUN%" "Segoe UI" %OUTTESTDIR%\auto.spritefont /FontSize:18 /TextureFormat:Auto
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\rgba32.spritefont /FontSize:18 /TextureFormat:Rgba32 /DebugOutputSpriteSheet:%OUTTESTDIR%\rgba32.bmp
"%RUN%" "Segoe UI" %OUTTESTDIR%\rgba32.spritefont /FontSize:18 /TextureFormat:Rgba32 /DebugOutputSpriteSheet:%OUTTESTDIR%\rgba32.bmp
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\bgra4444.spritefont /FontSize:18 /TextureFormat:Bgra4444
"%RUN%" "Segoe UI" %OUTTESTDIR%\bgra4444.spritefont /FontSize:18 /TextureFormat:Bgra4444
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )

echo.
echo "Segoe UI" %OUTTESTDIR%\CompressedMono.spritefont /FontSize:18 /TextureFormat:CompressedMono
"%RUN%" "Segoe UI" %OUTTESTDIR%\CompressedMono.spritefont /FontSize:18 /TextureFormat:CompressedMono
if %ERRORLEVEL% NEQ 0 ( call failure.cmd "Failed" )
