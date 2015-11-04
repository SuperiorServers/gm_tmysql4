local boost = "/media/storage/Documents/Developer/boost_1_59_0/"
if os.get() == "windows" then
	boost = "D:/Documents/Developer/boost_1_59_0"
end

solution "gm_tmysql4"

	language "C++"
	location ( os.get() .."-".. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "NoPCH", "StaticRuntime", "EnableSSE" }
	targetdir ( "lib/" .. os.get() .. "/" )
	includedirs { "include/GarrysMod", "include/" .. os.get(), boost }
	platforms{ "x32" }
	libdirs { "library/" .. os.get(), boost .. "/stage/lib" }
	if os.get() == "windows" then
		links { "mysqlclient" }
	elseif os.get() == "linux" then
		links { "mysqlclient", "boost_system" }
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
	
	project "gm_tmysql4"
		defines { "GMMODULE" }
		files { "src/**.*", "include/**.*" }
		kind "SharedLib"
		
