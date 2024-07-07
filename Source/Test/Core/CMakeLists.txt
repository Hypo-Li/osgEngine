add_executable(CoreTest "Main.cpp" "TestEventHandler.h"  "Template.cpp" "GLTFLoader.h")

LINK_WITH_VARIABLES(CoreTest OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY)
target_link_libraries(CoreTest PRIVATE Core ThirdParty ${OPENGL_LIBRARIES} "fast-lzma2.lib")
