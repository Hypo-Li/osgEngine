add_executable(CoreTest "Main.cpp" )

LINK_WITH_VARIABLES(CoreTest OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY)
target_link_libraries(CoreTest PRIVATE Core Render Component Utility ThirdParty ${OPENGL_LIBRARIES} "fast-lzma2.lib")
