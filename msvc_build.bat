"F:\Visual Studio\MSBuild\Current\Bin\MSBuild.exe" "Wolf Engine 2.0.sln" "/p:Configuration=Debug - Graphic Tests
IF %ERRORLEVEL% NEQ 0 GOTO ProcessError  
exit /b 0
:ProcessError
exit /b 1