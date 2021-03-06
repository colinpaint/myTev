cmake_minimum_required (VERSION 3.13..3.18)
set (CMAKE_CXX_STANDARD 17)

# version faff
file (STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/nanogui/common.h" nanogui_version_defines
      REGEX "#define NANOGUI_VERSION_(MAJOR|MINOR|PATCH) ")

foreach (ver ${nanogui_version_defines})
  if (ver MATCHES [[#define NANOGUI_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$]])
      set (NANOGUI_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
  endif()
endforeach()

if (NANOGUI_VERSION_PATCH MATCHES [[\.([a-zA-Z0-9]+)$]])
  set (NANOGUI_VERSION_TYPE "${CMAKE_MATCH_1}")
endif()

string (REGEX MATCH "^[0-9]+" NANOGUI_VERSION_PATCH "${NANOGUI_VERSION_PATCH}")
set (NANOGUI_VERSION "${NANOGUI_VERSION_MAJOR}.${NANOGUI_VERSION_MINOR}.${NANOGUI_VERSION_PATCH}")
message (STATUS "NanoGUI v${NANOGUI_VERSION} ${NANOGUI_VERSION_TYPE}")

# main project
project (nanogui DESCRIPTION "NanoGUI" LANGUAGES CXX C VERSION ${NANOGUI_VERSION} )

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message (STATUS "default to Release build")
  set (CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
string (TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

if (WIN32)
  set (NANOGUI_BUILD_GLAD_DEFAULT ON)
else()
  set (NANOGUI_BUILD_GLAD_DEFAULT OFF)
endif()

set (NANOGUI_BUILD_GLFW_DEFAULT ON)

if (CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
  set (CMAKE_CXX_COMPILER_ID "Emscripten")
  set (NANOGUI_BUILD_GLAD_DEFAULT   OFF)
  set (NANOGUI_BUILD_GLFW_DEFAULT   OFF)
  set (CMAKE_STATIC_LIBRARY_SUFFIX ".bc")
  set (CMAKE_EXECUTABLE_SUFFIX ".bc")
  set (CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_CXX_COMPILER> -o <TARGET> <LINK_FLAGS> <OBJECTS>")
  if (U_CMAKE_BUILD_TYPE MATCHES REL)
    add_compile_options (-O3 -DNDEBUG)
  endif()
endif()

option (NANOGUI_BUILD_GLAD                "Build GLAD OpenGL loader library? (needed on Windows)" ${NANOGUI_BUILD_GLAD_DEFAULT})
option (NANOGUI_BUILD_GLFW                "Build GLFW?" ${NANOGUI_BUILD_GLFW_DEFAULT})
option (NANOGUI_INSTALL                   "Install NanoGUI on `make install`?" ON)

set (NANOGUI_NATIVE_FLAGS_DEFAULT "")
set (NANOGUI_NATIVE_FLAGS ${NANOGUI_NATIVE_FLAGS_DEFAULT} CACHE STRING "Compilation flags")

if (NOT NANOGUI_BACKEND)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "armv7" OR
      CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR
      CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
    set (NANOGUI_BACKEND_DEFAULT "GLES 2")
  else()
    set (NANOGUI_BACKEND_DEFAULT "OpenGL")
  endif()

  set (NANOGUI_BACKEND ${NANOGUI_BACKEND_DEFAULT} CACHE STRING "backend OpenGL/GLES 2/GLES 3" FORCE)
endif()

set_property (CACHE NANOGUI_BACKEND PROPERTY STRINGS "OpenGL" "GLES 2" "GLES 3")

# Required libraries, flags, and include files for compiling and linking against nanogui (all targets)
include (CheckCXXCompilerFlag)
include (CheckCXXSourceRuns)

macro (CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set (CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set (CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set (CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_source_runs("#include <iostream>\nint main(int argc, char **argv) { std::cout << \"test\"; return 0; }" ${_RESULT})
  set (CMAKE_REQUIRED_FLAGS "")
  set (CMAKE_REQUIRED_LIBRARIES "")
endmacro()

set (NANOVG_SRC src/texture_gl.cpp
                src/shader_gl.cpp
                src/renderpass_gl.cpp
                src/opengl.cpp
                src/opengl_check.h )

if (NANOGUI_BACKEND STREQUAL "OpenGL")
  set (NANOGUI_BACKEND_DEFS -DNANOGUI_USE_OPENGL)
  set (NANOGUI_GLOB "resources/*.gl")
  message (STATUS "NanoGUI: using OpenGL backend.")
elseif (NANOGUI_BACKEND STREQUAL "GLES 2")
  set (NANOGUI_BACKEND_DEFS -DNANOGUI_USE_GLES -DNANOGUI_GLES_VERSION=2)
  set (NANOGUI_GLOB "resources/*.gles")
  message (STATUS "NanoGUI: using GLES 2 backend.")
elseif (NANOGUI_BACKEND STREQUAL "GLES 3")
  set (NANOGUI_BACKEND_DEFS -DNANOGUI_USE_GLES -DNANOGUI_GLES_VERSION=3)
  set (NANOGUI_GLOB "resources/*.gles")
  message (STATUS "NanoGUI: using GLES 3 backend.")
endif()

add_compile_options (${NANOGUI_NATIVE_FLAGS})

if (MSVC)
  add_definitions (/D "_CRT_SECURE_NO_WARNINGS")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # parallel build
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string (REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Prefer libc++ in conjunction with Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
  CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (HAS_LIBCPP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    message (STATUS "NanoGUI: using libc++.")
  else()
    CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP_AND_CPPABI "-stdlib=libc++" "-stdlib=libc++ -lc++abi")
    if (HAS_LIBCPP_AND_CPPABI)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
      set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
      set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
      message (STATUS "NanoGUI: using libc++ and libc++abi.")
    else()
      message (FATAL_ERROR "When Clang is used to compile NanoGUI, libc++ must be available -- GCC's libstdc++ is not supported! (please insteall the libc++ development headers, provided e.g. by the packages 'libc++-dev' and 'libc++abi-dev' on Debian/Ubuntu).")
    endif()
  endif()
endif()

# Compile GLFW
if (NANOGUI_BUILD_GLFW)
  set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
  set (GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
  set (GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
  set (GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)
  set (GLFW_INSTALL OFF CACHE BOOL " " FORCE)
  set (GLFW_USE_CHDIR OFF CACHE BOOL " " FORCE)
  set (BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
  endif()

  add_subdirectory (glfw)

  # Two targets have now been defined: `glfw_objects`, which will be merged into # NanoGUI at the end, and `glfw`.
  # The `glfw` target is the library itself # (e.g., libglfw.so), but can be skipped as we do not need to link against it
  # because we merge `glfw_objects` into NanoGUI
  set_target_properties (glfw PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
  mark_as_advanced (GLFW_BUILD_DOCS GLFW_BUILD_EXAMPLES GLFW_BUILD_INSTALL GLFW_BUILD_TESTS
                    GLFW_DOCUMENT_INTERNALS GLFW_INSTALL GLFW_USE_CHDIR GLFW_USE_MENUBAR
                    GLFW_USE_OSMESA GLFW_VULKAN_STATIC GLFW_USE_RETINA GLFW_USE_MIR
                    BUILD_SHARED_LIBS USE_MSVC_RUNTIME_LIBRARY_DLL)
endif()

if (NANOGUI_BUILD_GLAD)
  # Build and include GLAD on Windows
  list (APPEND NANOVG_SRC glad/src/glad.c
                          glad/include/glad/glad.h
                          glad/include/KHR/khrplatform.h)
  if (MSVC)
    set_source_files_properties (glad/src/glad.c PROPERTIES COMPILE_FLAGS /wd4055)
  endif()
endif()

# Required core libraries on various platforms
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Emscripten")
  # No dependencies
elseif (WIN32)
  list (APPEND NANOGUI_LIBS opengl32)
elseif (CMAKE_SYSTEM MATCHES "Linux")
  list (APPEND NANOGUI_LIBS dl X11 pthread)
  if (NANOGUI_BACKEND STREQUAL "OpenGL")
    list (APPEND NANOGUI_LIBS GL)
  elseif (NANOGUI_BACKEND STREQUAL "GLES 2")
    list(APPEND NANOGUI_LIBS GLESv2)
  elseif (NANOGUI_BACKEND STREQUAL "GLES 3")
    list (APPEND NANOGUI_LIBS GLESv3)
  endif()
endif()

# run cmake converter convert fonts,shader to c, concat fileNames to comma separated string
file (GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ttf" "${NANOGUI_GLOB}")
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")
message (STATUS "${resources_string}")
set (bin2c_cmdline -DOUTPUT_C=nanogui_resources.cpp
                   -DOUTPUT_H=nanogui_resources.h
                   "-DINPUT_FILES=${resources_string}"
                   -P "${CMAKE_CURRENT_SOURCE_DIR}/resources/bin2c.cmake")
message (STATUS "${bin2c_cmdline}")
add_custom_command (OUTPUT nanogui_resources.cpp nanogui_resources.h
                    COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
                    DEPENDS ${resources} ${resources_processed}
                    COMMENT "Running bin2c"
                    PRE_BUILD VERBATIM)

include_directories (${CMAKE_CURRENT_BINARY_DIR})

set (NANOGUI_LIBRARY_TYPE "STATIC")

if (NANOGUI_BUILD_GLFW)
  list (APPEND NANOVG_SRC $<TARGET_OBJECTS:glfw_objects>)
endif()

# Compile main NanoGUI library
add_library (nanogui ${NANOGUI_LIBRARY_TYPE}
  nanovg/nanovg.c
  nanovg/stb_image.h
  ${NANOVG_SRC}
  nanogui_resources.cpp
  include/nanogui/common.h src/common.cpp
  include/nanogui/widget.h src/widget.cpp
  include/nanogui/theme.h src/theme.cpp
  include/nanogui/layout.h src/layout.cpp
  include/nanogui/screen.h src/screen.cpp
  include/nanogui/label.h src/label.cpp
  include/nanogui/window.h src/window.cpp
  include/nanogui/popup.h src/popup.cpp
  include/nanogui/checkbox.h src/checkbox.cpp
  include/nanogui/button.h src/button.cpp
  include/nanogui/popupbutton.h src/popupbutton.cpp
  include/nanogui/combobox.h src/combobox.cpp
  include/nanogui/progressbar.h src/progressbar.cpp
  include/nanogui/slider.h src/slider.cpp
  include/nanogui/messagedialog.h src/messagedialog.cpp
  include/nanogui/textbox.h src/textbox.cpp
  include/nanogui/textarea.h src/textarea.cpp
  include/nanogui/imagepanel.h src/imagepanel.cpp
  include/nanogui/vscrollpanel.h src/vscrollpanel.cpp
  include/nanogui/colorwheel.h src/colorwheel.cpp
  include/nanogui/colorpicker.h src/colorpicker.cpp
  include/nanogui/graph.h src/graph.cpp
  include/nanogui/tabwidget.h src/tabwidget.cpp
  include/nanogui/canvas.h src/canvas.cpp
  include/nanogui/texture.h src/texture.cpp
  include/nanogui/shader.h src/shader.cpp
  include/nanogui/imageview.h src/imageview.cpp
  include/nanogui/traits.h src/traits.cpp
  include/nanogui/renderpass.h
  include/nanogui/formhelper.h
  include/nanogui/icons.h
  include/nanogui/toolbutton.h
  include/nanogui/opengl.h
  include/nanogui/nanogui.h
  )

target_compile_definitions (nanogui PUBLIC ${NANOGUI_BACKEND_DEFS}
                                    PRIVATE -DNANOGUI_BUILD -DNVG_STB_IMAGE_IMPLEMENTATION )

target_include_directories (nanogui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                                    PRIVATE nanovg ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories (nanogui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nanovg>
                                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nanovg>)

if (NANOGUI_BUILD_GLAD)
  target_compile_definitions (nanogui PUBLIC -DNANOGUI_GLAD)
  target_include_directories (nanogui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>
                                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/glad>)
endif()

if (NANOGUI_BUILD_GLFW)
  target_include_directories (nanogui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glfw/include>)
endif()

target_link_libraries (nanogui PUBLIC ${NANOGUI_LIBS})

# Auto-detect GLES include directory on Raspberry PI
if (EXISTS /opt/vc/include)
  target_include_directories (nanogui PUBLIC /opt/vc/include)
endif()

# install
if (NANOGUI_INSTALL)
  install (TARGETS nanogui LIBRARY
           DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install (DIRECTORY include/nanogui
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
  install (DIRECTORY nanovg/
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nanovg FILES_MATCHING PATTERN "*.h")

  if (NANOGUI_BUILD_GLFW)
    install (DIRECTORY glfw/include/GLFW
             DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
  endif()

  if (NANOGUI_BUILD_GLAD)
    install (DIRECTORY glad/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/glad FILES_MATCHING PATTERN "*.h")
  endif()

  install (TARGETS nanogui EXPORT nanoguiTargets)
  set (NANOGUI_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/nanogui")
  configure_package_config_file (resources/nanoguiConfig.cmake.in nanoguiConfig.cmake
                                 INSTALL_DESTINATION ${NANOGUI_CMAKECONFIG_INSTALL_DIR})
  write_basic_package_version_file (nanoguiConfigVersion.cmake
                                    VERSION ${PROJECT_VERSION}
                                    COMPATIBILITY AnyNewerVersion
                                    ARCH_INDEPENDENT)
  install (EXPORT nanoguiTargets DESTINATION ${NANOGUI_CMAKECONFIG_INSTALL_DIR})
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/nanoguiConfigVersion.cmake
                 ${CMAKE_CURRENT_BINARY_DIR}/nanoguiConfig.cmake
           DESTINATION ${NANOGUI_CMAKECONFIG_INSTALL_DIR})
endif()

# warnings
if (CMAKE_COMPILER_IS_GNUCC)
  set_source_files_properties (nanovg/nanovg.c PROPERTIES COMPILE_FLAGS -Wno-unused-result)
elseif (MSVC)
  set_source_files_properties (nanovg/nanovg.c PROPERTIES COMPILE_FLAGS "/wd4005 /wd4456 /wd4457")
endif()
