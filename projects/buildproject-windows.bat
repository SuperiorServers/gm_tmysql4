@echo off
echo.
echo Creating VS 2022 solution
echo.
premake5.exe --os=windows vs2022
echo.
echo Solution is located in the windows-vs2022 folder
echo.
pause