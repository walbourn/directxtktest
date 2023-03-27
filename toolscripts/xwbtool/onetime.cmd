@echo off

set BASE=.\
set REFDIR=%WINDIR%\Media
set OUTDIR=.\%out
set FAILURELOG=%OUTDIR%\Failures.txt

rd /q /s "%OUTDIR%"
