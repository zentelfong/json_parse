solution "json_parse"
	location ( "build" )
	targetdir("build")
	configurations { "Debug", "Release" }
	defines { "_CRT_SECURE_NO_WARNINGS" }
	characterset ("MBCS")
	
	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols", "ExtraWarnings"}

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize", "ExtraWarnings"}			
	
	
   	project "json_parse"
		language "C++"
		kind "StaticLib"
		files { 
			"src/*.h",
			"src/*.cpp",
		}

	project "test"
		kind "ConsoleApp"
		language "C++"
		files { "test.cpp" }
		links { "json_parse"}	
			


