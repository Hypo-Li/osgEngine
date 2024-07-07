file (GLOB_RECURSE IMGUI_FILES "./imgui/*.cpp" "./imgui/*.h")
file (GLOB_RECURSE JSON_FILES "./nlohmann/*.hpp")
file (GLOB_RECURSE SPDLOG_FILES "./spdlog/*.cpp" "./spdlog/*.h")
file (GLOB_RECURSE TINYGLTF_FILES "./tinygltf/tiny_gltf.cc" "./tinygltf/tiny_gltf.h")

add_library (
    ThirdParty
    ${IMGUI_FILES}
    ${JSON_FILES}
    ${SPDLOG_FILES}
    ${TINYGLTF_FILES}
)
