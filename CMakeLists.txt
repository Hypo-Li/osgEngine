cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

MACRO(LINK_WITH_VARIABLES TRGTNAME)
    FOREACH(varname ${ARGN})
        IF(${varname}_DEBUG)
            IF(${varname}_RELEASE)
                TARGET_LINK_LIBRARIES(${TRGTNAME} PRIVATE optimized "${${varname}_RELEASE}" debug "${${varname}_DEBUG}")
            ELSE(${varname}_RELEASE)
                TARGET_LINK_LIBRARIES(${TRGTNAME} PRIVATE optimized "${${varname}}" debug "${${varname}_DEBUG}")
            ENDIF(${varname}_RELEASE)
        ELSE(${varname}_DEBUG)
            TARGET_LINK_LIBRARIES(${TRGTNAME} PRIVATE ${${varname}} )
        ENDIF(${varname}_DEBUG)
    ENDFOREACH(varname)
ENDMACRO(LINK_WITH_VARIABLES TRGTNAME)

project ("osgEngine")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/CMakeModules")
add_definitions(-DTEMP_DIR="${CMAKE_SOURCE_DIR}/Temp/")
find_package(OSG REQUIRED)
find_package(OpenGL REQUIRED)
include_directories(
	${OSG_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/Source
)

add_subdirectory ("Source/Core")
add_subdirectory ("Source/Editor")
add_subdirectory ("Source/ThirdParty/imgui")
