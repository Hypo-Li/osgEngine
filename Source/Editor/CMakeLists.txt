add_executable (Editor "Editor.h" "Editor.cpp" "ImGuiHandler.h" "Main.cpp" "PickEventHandler.h" "Window/Widget.h" "Window/SceneView.h" "Window/Window.h" "Window/Inspector.h" "Window/AssetBrowser.h" "Window/DockSpace.h" "Window/WindowManager.h" "ControllerManipulator.h" "Window/MaterialEditor.h" "Window/ShaderEditor.h" "Window/Hierarchy.h" "GLDebugCallback.h" "Window/TextureViewer.h" "Gizmo.h")

LINK_WITH_VARIABLES(Editor OPENTHREADS_LIBRARY OSG_LIBRARY OSGDB_LIBRARY OSGUTIL_LIBRARY OSGGA_LIBRARY OSGVIEWER_LIBRARY OSGEARTH_LIBRARY)
target_link_libraries(Editor PRIVATE ThirdParty Core Component Render Utility ${OPENGL_LIBRARIES} "fast-lzma2.lib")
