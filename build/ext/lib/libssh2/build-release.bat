@echo off

setlocal

set "PROJECT_ROOT=%~dp0..\..\..\.."
set "LIBSSH2_ROOT=%PROJECT_ROOT%\ext\lib\libssh2"
set "BUILD_OUTPUT=%PROJECT_ROOT%/_out/build/libssh2/release"
set "LIB_OUTPUT=%PROJECT_ROOT%/_out/lib/libssh2/release"

if not exist "%BUILD_OUTPUT%/*" call :CMD mkdir "%%BUILD_OUTPUT%%" || exit /b
if not exist "%LIB_OUTPUT%/*" call :CMD mkdir "%%LIB_OUTPUT%%" || exit /b

call :CMD pushd "%%BUILD_OUTPUT%%" || exit /b

rem generator example: `-G "Visual Studio 16 2019"`
call :CMD cmake -S "%%LIBSSH2_ROOT%%" -B "%%BUILD_OUTPUT%%" -DCMAKE_BUILD_TYPE=Release || exit /b
echo.

call :CMD cmake --build "%%BUILD_OUTPUT%%" --config Release --target libssh2_static || exit /b
echo.

call :CMD copy /Y /B "%%BUILD_OUTPUT:/=\%%\src\Release\libssh2_static.lib" "%%LIB_OUTPUT:/=\%%\" || exit /b

exit /b 0

:CMD
echo.^> %*
echo.
(
  %*
)
