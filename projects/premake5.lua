local osname = os.target()

workspace "tmysql4"
	configurations { "Release", "Debug" }
	platforms { "DLL" }

	language "C++"
	cppdialect "C++17"

	location ( osname .."-".. _ACTION )
	
	flags { "NoPCH", "MultiProcessorCompile" }

	editandcontinue "Off"
	vectorextensions "SSE"
	staticruntime "Off"
	defines { "NDEBUG", "GMMODULE", "ENABLE_QUERY_TIMERS"}
	optimize "On"
	floatingpoint "Fast"
	warnings "Off"

	kind "SharedLib"
	includedirs { "../include/GarrysMod", "../include/" .. osname, "../include/asio/asio/include" }
	files { "../src/**.*", "../include/GarrysMod/**.*", "../include/" .. osname .. "/**.*" }

	if osname == "windows" then
		links { "mariadbclient", "bcrypt" }
	elseif osname == "linux" then
		links { "mariadbclient", "rt" }
		buildoptions { "-pthread", "-Wl,-z,defs" }			
		linkoptions { "-lpthread", "-lcrypto", "-lssl" }
	else error("unknown os: " .. osname) end

	targetprefix "gmsv_"
	targetname "tmysql4_"
	targetextension ".dll"
	
	project "tmysql4_x86"
		architecture "x86"
		libdirs { "../library/x86" }
		targetsuffix (osname == "windows" and "win32" or osname)
		targetdir ("../bin/x86/")
		filter "Debug"
			symbols "On"
		filter "Release"
			symbols "Off"

	project "tmysql4_x64"
		architecture "x86_64"
		libdirs { "../library/x64" }
		targetsuffix (osname == "windows" and "win64" or osname)
		targetdir ("../bin/x64/")
		filter "Debug"
			symbols "On"
		filter "Release"
			symbols "Off"