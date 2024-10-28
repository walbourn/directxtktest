@echo off

set BASE=.\
set REFDIR=.\ref
set OUTDIR=.\%out
set FAILURELOG=%OUTDIR%\Failures.txt

rd /q /s "%OUTDIR%"
