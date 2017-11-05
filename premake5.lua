-- bjam release address-model=32 runtime-link=static --with-system --with-thread --with-date_time --with-regex --with-serialization stage

local osname = os.get()
local dllname = osname

if dllname == "windows" then
	dllname = "win32"
end

solution "tmysql4"

	language "C++"
	location ( osname .."-".. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "NoPCH", "StaticRuntime", "EnableSSE" }
	targetdir ( "bin/" .. osname .. "/" )
	includedirs { "include/GarrysMod", "include/" .. osname }
	platforms{ "x32" }
	libdirs { "library/" .. osname }

	targetprefix ("gmsv_")
	targetname(solution().name)
	targetsuffix("_" .. dllname)
	targetextension ".dll"

	if osname == "windows" then
		links { "mysqlclient" }
	elseif osname == "linux" then
		links { "mysqlclient", "boost_system", "rt" }
	else error( "unknown os: " .. osname ) end
	
	configurations
	{ 
		"Release"
	}
	
	configuration "Release"
		if osname == "linux" then
			buildoptions { "-std=c++0x -pthread -Wl,-z,defs" }
		end
		defines { "NDEBUG" }
		flags{ "Optimize", "FloatFast" }
	
	project "tmysql4"
		defines { "GMMODULE", "ENABLE_QUERY_TIMERS" }
		files { "src/**.*", "include/**.*" }
		kind "SharedLib"
		
