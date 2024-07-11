add_executable(CloudTest "Main.cpp" "ControllerManipulator.h" "ImGuiHandler.h")

LINK_WITH_VARIABLES(CloudTest OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY)
target_link_libraries(CloudTest PRIVATE Core ThirdParty ${OPENGL_LIBRARIES})
