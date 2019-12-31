-- bjam release address-model=32 runtime-link=static --with-system --with-thread --with-date_time --with-regex --with-serialization stage

local osname = os.target()
local dllname = osname

if dllname == "windows" then
	dllname = "win32"
end

solution "tmysql4"

	language "C++"
	location ( osname .."-".. _ACTION )
	
	flags {"NoPCH"}
	symbols"On"
	editandcontinue"Off"
	vectorextensions"SSE"
	staticruntime"On"
	
	targetdir ( "bin/" .. osname .. "/" )
	includedirs { "include/GarrysMod", "include/" .. osname, "include/asio/asio/include" }
	platforms{ "x86","x86_64" }
	libdirs { "library/" .. osname }

	targetprefix ("gmsv_")
	targetname(solution().name)
	targetsuffix("_" .. dllname)
	targetextension ".dll"

	if osname == "windows" then
		links { "mysqlclient" }
	elseif osname == "linux" then
		links { "mysqlclient", "rt" }
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
		optimize"On"
		floatingpoint"Fast"
	
	project "tmysql4"
		defines { "GMMODULE", "ENABLE_QUERY_TIMERS" }
		files { "src/**.*", "include/GarrysMod/**.*", "include/" .. osname .. "/**.*" }
		kind "SharedLib"
		
		filter'platforms:x86'
			architecture'x86'
		filter'platforms:x64'
			architecture'x86_64'
		filter{}
