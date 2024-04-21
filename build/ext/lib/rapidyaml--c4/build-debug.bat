@echo off

setlocal

set "PROJECT_ROOT=%~dp0..\..\..\.."
set "RAPIDYAML_C4CORE_ROOT=%PROJECT_ROOT%\ext\lib\rapidyaml\ext\c4core"
set "BUILD_OUTPUT=%PROJECT_ROOT%/_out/build/rapidyaml--c4core/debug"
set "LIB_OUTPUT=%PROJECT_ROOT%/_out/lib/rapidyaml--c4core/debug"

if not exist "%BUILD_OUTPUT%/*" call :CMD mkdir "%%BUILD_OUTPUT%%" || exit /b
if not exist "%LIB_OUTPUT%/*" call :CMD mkdir "%%LIB_OUTPUT%%" || exit /b

call :CMD pushd "%%BUILD_OUTPUT%%" || exit /b

rem generator example: `-G "Visual Studio 16 2019"`
call :CMD cmake -S "%%RAPIDYAML_C4CORE_ROOT%%" -B "%%BUILD_OUTPUT%%" -DCMAKE_BUILD_TYPE=Debug || exit /b
echo.

call :CMD cmake --build "%%BUILD_OUTPUT%%" --config Debug --target c4core || exit /b
echo.

call :CMD copy /Y /B "%%BUILD_OUTPUT:/=\%%\debug\c4core.lib" "%%LIB_OUTPUT:/=\%%\" || exit /b

exit /b 0

:CMD
echo.^> %*
echo.
(
  %*
)
