include_directories(
	${OSG_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/Source
	${CMAKE_SOURCE_DIR}/Source/ThirdParty
)

file (GLOB_RECURSE IMGUI_FILES "./imgui/*.cpp" "./imgui/*.h")
file (GLOB_RECURSE SPDLOG_FILES "./spdlog/*.cpp" "./spdlog/*.h")

add_library (
    ThirdParty
    ${IMGUI_FILES}
    ${SPDLOG_FILES}
)
