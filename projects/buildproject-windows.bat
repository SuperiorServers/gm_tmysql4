@echo off
echo.
echo Creating VS 2019 solution
echo.
premake5.exe --os=windows vs2019
echo.
echo Solution is located in the windows-vs2019 folder
echo.
pause