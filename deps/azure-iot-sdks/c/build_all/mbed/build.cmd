
@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set repo-build-root=%current-path%\..\..
rem // resolve to fully qualified path
for %%i in ("%repo-build-root%") do set repo-build-root=%%~fi

rem -----------------------------------------------------------------------------
rem -- build (clean) compilembed tool
rem -----------------------------------------------------------------------------

call "%repo-build-root%\tools\compilembed\build.cmd" --clean
if not %errorlevel%==0 exit /b %errorlevel%

rem -----------------------------------------------------------------------------
rem -- build iothub client samples
rem -----------------------------------------------------------------------------

call %repo-build-root%\iothub_client\build\mbed\build.cmd %*
if not %errorlevel%==0 exit /b %errorlevel%

goto :eof
