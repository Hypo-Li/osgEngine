add_executable(QuickTest "Main.cpp" "NewPipeline.cpp" )

include_directories("D:/Code/OSG/osgearth3.5/include")

LINK_WITH_VARIABLES(QuickTest OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY OSGPARTICLE_LIBRARY)
target_link_libraries(QuickTest PRIVATE Core Render ThirdParty ${OPENGL_LIBRARIES} "D:/Code/OSG/osgearth3.5/lib/Debug/osgEarthd.lib" "fast-lzma2.lib")
#target_link_libraries(QuickTest PRIVATE Core Render ThirdParty ${OPENGL_LIBRARIES} "D:/Code/OSG/osgearth3.5/lib/Release/osgEarth.lib")
#target_link_libraries(QuickTest PRIVATE Core Render ThirdParty ${OPENGL_LIBRARIES})
