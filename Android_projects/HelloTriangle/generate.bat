gradlew assembleDebug
IF %ERRORLEVEL% NEQ 0 GOTO ProcessError  
exit /b 0
:ProcessError
exit /b 1