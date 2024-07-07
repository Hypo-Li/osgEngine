cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

macro (LINK_WITH_VARIABLES TRGTNAME)
    foreach (varname ${ARGN})
        if (${varname}_DEBUG)
            if (${varname}_RELEASE)
                target_link_libraries (${TRGTNAME} PRIVATE optimized "${${varname}_RELEASE}" debug "${${varname}_DEBUG}")
            else (${varname}_RELEASE)
                target_link_libraries (${TRGTNAME} PRIVATE optimized "${${varname}}" debug "${${varname}_DEBUG}")
            endif (${varname}_RELEASE)
        else (${varname}_DEBUG)
            target_link_libraries (${TRGTNAME} PRIVATE ${${varname}} )
        endif (${varname}_DEBUG)
    endforeach (varname)
endmacro (LINK_WITH_VARIABLES TRGTNAME)

project ("osgEngine")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/CMakeModules")
add_definitions(-DTEMP_DIR="${CMAKE_SOURCE_DIR}/Temp/")
add_definitions(-DSHADER_DIR="${CMAKE_SOURCE_DIR}/Shader/")
find_package(OSG REQUIRED)
find_package(OpenGL REQUIRED)
include_directories(
	${OSG_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/Source
	${CMAKE_SOURCE_DIR}/Source/ThirdParty
)

link_directories(
    ${CMAKE_SOURCE_DIR}/Library
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory ("Source/Core")
add_subdirectory ("Source/Editor")
add_subdirectory ("Source/ThirdParty")
add_subdirectory ("Source/Test/Core")
add_subdirectory ("Source/Test/Cloud")
