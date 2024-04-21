@echo off

setlocal

set "RAPIDYAML_ROOT=%~dp0..\..\..\..\ext\lib\rapidyaml"

call :CMD cmake "-DRAPIDYAML_ROOT=%%RAPIDYAML_ROOT%%" -P "%%~dpn0.cmake" || exit /b

exit /b 0

:CMD
echo.^> %*
echo.
(
  %*
)
