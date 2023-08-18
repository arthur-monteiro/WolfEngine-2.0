FOR /F "tokens=*" %%g IN ('curl http://127.0.0.1:5000/graphictests') do (SET VAR=%%g)
IF %VAR% NEQ 2 GOTO ProcessError  
@code
exit /b 0
:ProcessError
@codeprocess error
exit /b 1