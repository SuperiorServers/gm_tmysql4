premake4 --os=windows --file=BuildProjects.lua vs2010
premake4 --os=macosx --platform=universal32 --file=BuildProjects.lua gmake
premake4 --os=linux --file=BuildProjects.lua gmake
pause