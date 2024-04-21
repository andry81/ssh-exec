@echo off

setlocal

set "PROJECT_ROOT=%~dp0..\..\..\.."
set "RAPIDYAML_ROOT=%PROJECT_ROOT%\ext\lib\rapidyaml"
set "BUILD_OUTPUT=%PROJECT_ROOT%/_out/build/rapidyaml/release"
set "LIB_OUTPUT=%PROJECT_ROOT%/_out/lib/rapidyaml/release"

if not exist "%BUILD_OUTPUT%/*" call :CMD mkdir "%%BUILD_OUTPUT%%" || exit /b
if not exist "%LIB_OUTPUT%/*" call :CMD mkdir "%%LIB_OUTPUT%%" || exit /b

call :CMD pushd "%%BUILD_OUTPUT%%" || exit /b

rem generator example: `-G "Visual Studio 16 2019"`
call :CMD cmake -S "%%RAPIDYAML_ROOT%%" -B "%%BUILD_OUTPUT%%" -DCMAKE_BUILD_TYPE=Release || exit /b
echo.

call :CMD cmake --build "%%BUILD_OUTPUT%%" --config Release --target ryml || exit /b
echo.

call :CMD copy /Y /B "%%BUILD_OUTPUT:/=\%%\release\ryml.lib" "%%LIB_OUTPUT:/=\%%\" || exit /b

exit /b 0

:CMD
echo.^> %*
echo.
(
  %*
)
