@echo off
echo Starting Wolf-Engine android build...

call gradlew.bat clean assembleDebug copyStaticLibs

echo.
if %ERRORLEVEL% NEQ 0 (
    echo Build failed with code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Build successful!