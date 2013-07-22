solution "Kinect Espresso Code"
    configurations { "Debug", "Release" }
    location "./build"
    
    includedirs { "./include", "$(KINECTSDK10_DIR)/inc", "$(FTSDK_DIR)/inc" }
    libdirs { "./lib", "$(KINECTSDK10_DIR)/lib/x86", "$(FTSDK_DIR)/Lib/x86" }
    links { "opengl32", "glu32", "freeglut", "FreeImage", "Kinect10", "FaceTrackLib" }
    
    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }
        targetdir "./bin"
        targetsuffix "_d"
        
    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }
        targetdir "./bin"
    
    project "ch2_01_OpenGL_Env"
        kind "ConsoleApp"
        language "C++"
        files { "ch2_01.cpp" }
        
    project "ch2_02_Init_Kinect"
        kind "ConsoleApp"
        language "C++"
        files { "ch2_02.cpp", "./common/*.*" }
        
    project "ch3_01_Color_Depth"
        kind "ConsoleApp"
        language "C++"
        files { "ch3_01.cpp", "./common/*.*" }
        
    project "ch3_02_Background_Subtraction"
        kind "ConsoleApp"
        language "C++"
        files { "ch3_02.cpp", "./common/*.*" }
        
    project "ch3_03_Magic_Photographer"
        kind "ConsoleApp"
        language "C++"
        files { "ch3_03.cpp", "./common/*.*" }
        
    project "ch4_01_Skeleton_Drawer"
        kind "ConsoleApp"
        language "C++"
        files { "ch4_01.cpp", "./common/*.*" }
        
    project "ch4_02_Hand_Linetrails"
        kind "ConsoleApp"
        language "C++"
        files { "ch4_02.cpp", "./common/*.*" }
        
    project "ch4_03_Face_Tracking"
        kind "ConsoleApp"
        language "C++"
        files { "ch4_03.cpp", "./common/*.*" }
        
    project "ch4_04_Face_Modeling"
        kind "ConsoleApp"
        language "C++"
        files { "ch4_04.cpp", "./common/*.*" }
        
    project "ch5_01_Hand_Cursors"
        kind "ConsoleApp"
        language "C++"
        files { "ch5_01.cpp", "./common/*.*" }
        
    project "ch5_02_Swiping_And_Holding"
        kind "ConsoleApp"
        language "C++"
        files { "ch5_02.cpp", "./common/*.*" }
        
    project "ch5_03_Emulating_Mouse"
        kind "ConsoleApp"
        language "C++"
        files { "ch5_03.cpp", "./common/*.*" }
        
    project "ch6_01_Integrating_Everything"
        kind "ConsoleApp"
        language "C++"
        files { "ch6_01.cpp", "./common/*.*" }
        
    project "ch6_02_Cutting_Fruits"
        kind "ConsoleApp"
        language "C++"
        files { "ch6_02.cpp", "./common/*.*" }
        
    project "ch6_03_Playing_Game"
        kind "ConsoleApp"
        language "C++"
        files { "ch6_03.cpp", "./common/*.*" }
        