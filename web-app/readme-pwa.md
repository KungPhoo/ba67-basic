# HTML Port
## About
This port tries to bring BA67 into a browser window
and finally provide a PWA (portable web app) that
you can install on a PC, tablet or smartphone.

## Build
Here's a Windows batch script to compile the
PWA:
```
rem @echo off
rem Path to root folder of BA67 the git project
set BA67=%~dp0..\..

rem path to where emscripten is installed
set EMSDK=%USERPROFILE%\emsdk

rem add you path to CMake, here
set PATH=%PATH%;C:\Qt\Tools\CMake_64\bin

rem add you path to Ninja, here
set PATH=%PATH%;C:\Qt\Tools\Ninja

rem assume you installed emscripten to "%USERPROFILE%\emsdk
call "%EMSDK%\emsdk_env.bat"
pushd "%~dp0"

call emcmake cmake "%BA67%"
rem optionally force a rebuild
rem call cmake --build . --target clean
call cmake --build . --

if exist "%BA67%\bin\BA67.html" (
call emrun "%BA67%\bin\BA67.html"
)
popd
pause
```


Notes:

## Run/Deploy
Serve the `html/` directory over HTTPS
(required for some PWA features and clipboard APIs).
You can use `python -m http.server` for
local testing (no HTTPS locally).
