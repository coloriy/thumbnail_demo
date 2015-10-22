:START
echo
set Part=%~dp0
cls

echo %Part%
pause

cls
echo
echo cleanning...
for /r %Part% %%x in (.) do (
for %%c in (.ncb .res .suo .sdf .aps .user .pdb .ilk .exp .log .obj .idb .tlog .lastbuildstate .ipch .pch) do (
if exist "%%~sx\*%%c" attrib -h "%%~sx\*%%c" &del /s /q "%%~sx\*%%c"
)
)


echo cleanning the folder

for /r %Part% %%x in (.) do (
set p=%%~sx
if "!p:~-5!"=="Debug" rd /s /q "%%~sx"
if "!p:~-7!"=="Release" rd /s /q "%%~sx"
)


echo clean finished!
pause