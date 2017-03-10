@echo off
REM This pre-build file is for MSVS VC++. It parses the git master hash and
REM converts it into GitVersion.h for compiling into builds. [George M1GEO]

cd %1
setlocal enabledelayedexpansion
set HEADFILE=.git\HEAD
set HASHFILE=0
if exist %HEADFILE% (
	for /F "tokens=4 delims=/:" %%a in ('type %HEADFILE%') do set HEADBRANCH=%%a	
	set HASHFILE=.git\refs\heads\!HEADBRANCH!
	echo Found Git HEAD file: %HEADFILE%   
	echo Git HEAD branch: !HEADBRANCH!
	echo Git HASH file: !HASHFILE!
	call :USEHASH
) else (
	echo No head file :(
	call :USENULL
)

goto :EOF

:USENULL
set GITHASH=0000000000000000000000000000000000000000
goto :WRITEGITVERSIONHEADER

:USEHASH
for /f %%i in ('type !HASHFILE!') do set GITHASH=%%i
goto :WRITEGITVERSIONHEADER

:WRITEGITVERSIONHEADER
echo // File contains Git commit ID SHA1 present at buildtime (prebuild.cmd) > GitVersion.h
echo const char *gitversion = "%GITHASH%"; >> GitVersion.h
echo Current Git HASH: %GITHASH%
goto :FINISHED

:FINISHED
echo GitVersion.h written...
