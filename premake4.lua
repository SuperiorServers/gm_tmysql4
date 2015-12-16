-- bjam release address-model=32 runtime-link=static --with-system --with-thread --with-date_time --with-regex --with-serialization stage

local boost = "/media/jake/storage/Documents/Developer/boost_1_59_0/"
if os.get() == "windows" then
	boost = "D:/Documents/Developer/boost_1_59_0"
end

solution "tmysql4"

	language "C++"
	location ( os.get() .."-".. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "NoPCH", "StaticRuntime", "EnableSSE" }
	targetdir ( "bin/" .. os.get() .. "/" )
	includedirs { "include/GarrysMod", "include/" .. os.get(), boost }
	platforms{ "x32" }
	libdirs { "library/" .. os.get(), boost .. "/stage/lib" }

	targetprefix ("gmsv_")
	targetname(solution().name)
	targetsuffix("_" .. os.get())
	targetextension ".dll"

	if os.get() == "windows" then
		links { "mysqlclient" }
	elseif os.get() == "linux" then
		links { "mysqlclient", "boost_system", "rt" }
	else error( "unknown os: " .. os.get() ) end
	
	configurations
	{ 
		"Release"
	}
	
	configuration "Release"
		if os.get() == "linux" then
			buildoptions { "-std=c++0x -pthread -Wl,-z,defs" }
		end
		defines { "NDEBUG" }
		flags{ "Optimize", "FloatFast" }
	
	project "tmysql4"
		defines { "GMMODULE", "ENABLE_QUERY_TIMERS" }
		files { "src/**.*", "include/**.*" }
		kind "SharedLib"
		
