
CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY "fmtlib/fmt"
    GIT_TAG 7.1.3
)

CPMAddPackage("gh:leethomason/tinyxml2#9.0.0")
CPMAddPackage(
    NAME miniz
    GITHUB_REPOSITORY "richgel999/miniz"
    GIT_TAG 2.2.0
    OPTIONS
        "BUILD_SHARED_LIBS Off"
)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY "glfw/glfw"
    GIT_TAG 3.3.8
    GIT_SHALLOW ON
    OPTIONS
        "BUILD_SHARED_LIBS Off"
        "GLFW_BUILD_EXAMPLES Off"
        "GLFW_BUILD_TESTS Off"
        "GLFW_BUILD_DOCS Off"
        "GLFW_INSTALL Off"
)

CPMAddPackage(
    NAME imgui
    GIT_TAG docking
    GITHUB_REPOSITORY ocornut/imgui
    DOWNLOAD_ONLY True
)

if (imgui_ADDED)
    message(STATUS "Adding IMGui: ${imgui_SOURCE_DIR}")
    add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    )

    target_include_directories(imgui
        PUBLIC
            "include"
            "${imgui_SOURCE_DIR}/"
    )

    target_link_libraries(imgui
        PRIVATE
            glfw
    )

    target_compile_options(imgui
        PUBLIC
            -DIMGUI_IMPL_OPENGL_LOADER_GLAD
    )
else()
    message(FATAL_ERROR "IMGUI not found")
endif()
