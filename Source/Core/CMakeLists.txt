add_executable (Core "Core.cpp" "Core.h" "Entity.h" "Entity.cpp" "Component.h" "Context.h" "Pipeline.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Core PROPERTY CXX_STANDARD 20)
endif()

LINK_WITH_VARIABLES(Core OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY OSGEARTH_LIBRARY)
target_link_libraries(Core PRIVATE imgui ${OPENGL_LIBRARIES})
