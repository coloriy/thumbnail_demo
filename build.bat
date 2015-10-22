echo off
 
path %SYSTEMROOT%\Microsoft.NET\Framework\v4.0.30319\
 
msbuild.exe %~dp0\thumbnail_demo.sln /t:Rebuild /p:Configuration=Release /p:VisualStudioVersion=12.0
::/l:FileLogger,Microsoft.Build.Engine;logfile=Build1.log

 
echo compile finished.
pause