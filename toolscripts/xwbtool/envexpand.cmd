@echo off

if '%1' EQU '' goto done
if '%2' EQU '' goto done

copy /Y %1 %2 2> NUL > NUL
for /F "usebackq tokens=1,2 delims==" %%s in (`set`) do rep %%%%s%% "%%t" %2 > NUL

:done
