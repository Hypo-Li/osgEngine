add_executable (Editor "Editor.h" "Editor.cpp" "ImGuiHandler.h" "Main.cpp" "PickEventHandler.h" "Widget.h" "SceneView.h" "Window.h" "Inspector.h" "AssetBrowser.h" "DockSpace.h" "WindowManager.h" "ControllerManipulator.h")

LINK_WITH_VARIABLES(Editor OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY OSGEARTH_LIBRARY)
target_link_libraries(Editor PRIVATE ThirdParty Core Component Render ${OPENGL_LIBRARIES})
