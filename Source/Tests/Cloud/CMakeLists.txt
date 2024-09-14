add_executable(CloudTest "Main.cpp" "ControllerManipulator.h" "ImGuiHandler.h")

include_directories("D:/Code/OSG/osgearth3.5/include")

LINK_WITH_VARIABLES(CloudTest OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY)
#target_link_libraries(CloudTest PRIVATE Core Render ThirdParty ${OPENGL_LIBRARIES} "D:/Code/OSG/osgearth3.5/lib/Debug/osgEarthd.lib")
target_link_libraries(CloudTest PRIVATE Core Render ThirdParty ${OPENGL_LIBRARIES} "D:/Code/OSG/osgearth3.5/lib/Release/osgEarth.lib")
#target_link_libraries(CloudTest PRIVATE Core Render ThirdParty ${OPENGL_LIBRARIES})
