#!/bin/sh
echo
echo Creating makefiles
echo
./premake5 --os=linux gmake
echo
echo Makefile is located in ./linux-gmake
echo Configurations: release_x86 release_x64
echo
