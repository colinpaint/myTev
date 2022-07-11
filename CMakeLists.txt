cmake_minimum_required (VERSION 3.13)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

project (tev VERSION 1.24 DESCRIPTION "HDR image viewer, mostly OpenEXR" LANGUAGES C CXX)
set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT tev)

set (TEV_VERSION "${CMAKE_PROJECT_VERSION}")
if (NOT TEV_DEPLOY)
  set (TEV_VERSION "${TEV_VERSION}dev")
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set (TEV_VERSION "${TEV_VERSION} (64 bit)")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set (TEV_VERSION "${TEV_VERSION} (32 bit)")
endif()

# Set a default configuration if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message (STATUS "No release type specified. Setting to 'Release'")
  set (CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
  set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS /MP24")
  set (CompilerFlags
       CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
       CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  foreach (CompilerFlag ${CompilerFlags})
    string (REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ZLIB=1 -s DISABLE_EXCEPTION_CATCHING=0")
  endif()
endif()

# Prefer libc++ in conjunction with Clang
include (CheckCXXSourceRuns)
include (GNUInstallDirs)
include (CMakePackageConfigHelpers)

macro (CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set (CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set (CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set (CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_source_runs ("#include <iostream>\nint main(int argc, char **argv) { std::cout << \"test\"; return 0; }" ${_RESULT})
  set (CMAKE_REQUIRED_FLAGS "")
  set (CMAKE_REQUIRED_LIBRARIES "")
endmacro()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
  CHECK_CXX_COMPILER_AND_LINKER_FLAGS (HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (HAS_LIBCPP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    message (STATUS "TEV: using libc++.")
  else()
    CHECK_CXX_COMPILER_AND_LINKER_FLAGS (HAS_LIBCPP_AND_CPPABI "-stdlib=libc++" "-stdlib=libc++ -lc++abi")
    if (HAS_LIBCPP_AND_CPPABI)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
      set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
      set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
      message (STATUS "TEV: using libc++ and libc++abi.")
    else()
      message (FATAL_ERROR
               "When Clang is used to compile TEV, libc++ must be available -- GCC's libstdc++ is not supported!"
               "- install libc++ development headers by provided by libc++-dev and libc++abi-dev")
    endif()
  endif()
endif()

set (UTILS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/utils)
set (QOI_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/qoi)
set (UTFCPP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/utfcpp/source)

# Compile nanogui
set (NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set (NANOGUI_BUILD_SHARED OFF CACHE BOOL " " FORCE)
set (NANOGUI_BUILD_PYTHON OFF CACHE BOOL " " FORCE)
set (NANOGUI_INSTALL OFF CACHE BOOL " " FORCE)
add_subdirectory (nanogui)

set (NANOGUI_TARGETS nanogui glfw glfw_objects)

if (WIN32)
  # Compile DirectXTex
  set (BUILD_TOOLS OFF CACHE BOOL " " FORCE)
  set (BUILD_DX11 OFF CACHE BOOL " " FORCE)
  set (BUILD_DX12 OFF CACHE BOOL " " FORCE)
  set (BC_USE_OPENMP OFF CACHE BOOL " " FORCE)
  add_subdirectory (DirectXTex)
  set (DIRECTXTEX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/DirectXTex" CACHE PATH " " FORCE)
  set (DIRECTXTEX_LIBRARY DirectXTex)
  include_directories (${DIRECTXTEX_INCLUDE_DIR})
endif()

# Compile OpenEXR
set (PYILMBASE_ENABLE OFF CACHE BOOL " " FORCE)
set (ILMBASE_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
set (OPENEXR_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
set (ILMBASE_NAMESPACE_VERSIONING OFF CACHE BOOL " " FORCE)
set (OPENEXR_NAMESPACE_VERSIONING OFF CACHE BOOL " " FORCE)
add_subdirectory (openexr EXCLUDE_FROM_ALL)

# compile openexr
set (OPENEXR_INCLUDE_DIRS
     ${CMAKE_CURRENT_SOURCE_DIR}/openexr/IlmBase/Imath
     ${CMAKE_CURRENT_SOURCE_DIR}/openexr/IlmBase/Iex
     ${CMAKE_CURRENT_SOURCE_DIR}/openexr/IlmBase/Half
     ${CMAKE_CURRENT_SOURCE_DIR}/openexr/OpenEXR/IlmImf
     ${CMAKE_CURRENT_BINARY_DIR}/openexr/OpenEXR/config
     ${CMAKE_CURRENT_BINARY_DIR}/openexr/IlmBase/config)

# compile clip
add_subdirectory (clip)
set (CLIP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/clip)

if (WIN32)
  # Compile zlib
  set (ZLIB_BUILD_STATIC_LIBS ON CACHE BOOL " " FORCE)
  set (ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
  set (SKIP_INSTALL_ALL ON CACHE BOOL " " FORCE)
  add_subdirectory (zlib)
  set (ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib" CACHE PATH " " FORCE)
  set (ZLIB_LIBRARY zlibstatic)
  include_directories (${ZLIB_INCLUDE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/zlib")
endif()

# compile nanogui
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  set (GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/glfw/include)
endif()
set (NANOGUI_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/nanovg/src
                     ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/include)
set (NANOGUI_EXTRA_INCS ${NANOGUI_EXTRA_INCS})
set (NANOGUI_EXTRA_DEFS ${NANOGUI_EXTRA_DEFS})
set (NANOGUI_EXTRA_LIBS ${NANOGUI_EXTRA_LIBS})

add_compile_options (${NANOGUI_NATIVE_FLAGS})

# Enable more warnings
if (MSVC)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string (REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()

  # Disable warnings that are present in dependencies and irrelevant to us
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unused arguments
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd5054") # deprecated enum & operator

  # wildcards in command-line path on windows, link wsetargv.obj http://msdn.microsoft.com/en-us/library/8bch7bkk.aspx
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} wsetargv.obj")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types -Wno-deprecated-register -Wno-deprecated-anon-enum-enum-conversion")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-misleading-indentation -Wno-deprecated-declarations")
  endif()
endif()

# Coroutines need to be explicitly enabled on g++
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()

set (TEV_LIBS clip IlmImf nanogui ${NANOGUI_EXTRA_LIBS})
if (MSVC)
  set (TEV_LIBS ${TEV_LIBS} zlibstatic DirectXTex wsock32 ws2_32)
endif()

set (TEV_SOURCES
     src/main.cpp
     src/Box.h src/Box.cpp
     src/Channel.h src/Channel.cpp
     src/Common.h src/Common.cpp
     src/FalseColor.h src/FalseColor.cpp
     src/HelpWindow.h src/HelpWindow.cpp
     src/Image.h src/Image.cpp
     src/ImageButton.h src/ImageButton.cpp
     src/ImageCanvas.h src/ImageCanvas.cpp
     src/ImageViewer.h src/ImageViewer.cpp
     src/Ipc.h src/Ipc.cpp
     src/Lazy.h src/Lazy.cpp
     src/MultiGraph.h src/MultiGraph.cpp
     src/SharedQueue.h src/SharedQueue.cpp
     src/Task.h src/Task.cpp
     src/ThreadPool.h src/ThreadPool.cpp
     src/UberShader.h src/UberShader.cpp
     src/imageio/ClipboardImageLoader.h src/imageio/ClipboardImageLoader.cpp
     src/imageio/EmptyImageLoader.h src/imageio/EmptyImageLoader.cpp
     src/imageio/ExrImageLoader.h src/imageio/ExrImageLoader.cpp
     src/imageio/ExrImageSaver.h src/imageio/ExrImageSaver.cpp
     src/imageio/ImageLoader.h src/imageio/ImageLoader.cpp
     src/imageio/ImageSaver.h src/imageio/ImageSaver.cpp
     src/imageio/PfmImageLoader.h src/imageio/PfmImageLoader.cpp
     src/imageio/QoiImageLoader.h src/imageio/QoiImageLoader.cpp
     src/imageio/QoiImageSaver.h src/imageio/QoiImageSaver.cpp
     src/imageio/StbiHdrImageSaver.h src/imageio/StbiHdrImageSaver.cpp
     src/imageio/StbiImageLoader.h src/imageio/StbiImageLoader.cpp
     src/imageio/StbiLdrImageSaver.h src/imageio/StbiLdrImageSaver.cpp
     )

if (MSVC)
  set (TEV_SOURCES ${TEV_SOURCES} resources/icon.rc)
  set (TEV_SOURCES ${TEV_SOURCES} resources/tev.manifest)
  set (TEV_SOURCES ${TEV_SOURCES} src/imageio/DdsImageLoader.h src/imageio/DdsImageLoader.cpp)
endif()

add_executable (tev ${TEV_SOURCES})

include_directories (${CLIP_INCLUDE}
                     ${DIRECTXTEX_INCLUDE}
                     ${GLFW_INCLUDE}
                     ${NANOGUI_EXTRA_INCS}
                     ${NANOGUI_INCLUDE}
                     ${OPENEXR_INCLUDE_DIRS}
                     ${QOI_INCLUDE}
                     ${UTILS_INCLUDE}
                     ${UTFCPP_INCLUDE}
                     "${CMAKE_CURRENT_SOURCE_DIR}/src")

set (TEV_DEFINITIONS -DTEV_VERSION="${TEV_VERSION}")

add_definitions (${TEV_DEFINITIONS} ${NANOGUI_EXTRA_DEFS})

target_link_libraries (tev ${TEV_LIBS})

if (WIN32)
  install (TARGETS tev RUNTIME DESTINATION "bin")
else()
  install (TARGETS tev)
  configure_file (resources/tev.desktop resources/tev.desktop)
  install (FILES ${CMAKE_BINARY_DIR}/resources/tev.desktop DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications")
  install (FILES resources/icon-512.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/512x512/apps" RENAME tev.png)
  install (FILES resources/icon-256.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME tev.png)
  install (FILES resources/icon-128.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME tev.png)
  install (FILES resources/icon-96.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/96x96/apps" RENAME tev.png)
  install (FILES resources/icon-64.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME tev.png)
  install (FILES resources/icon-48.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/48x48/apps" RENAME tev.png)
  install (FILES resources/icon-32.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME tev.png)
  install (FILES resources/icon-24.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/24x24/apps" RENAME tev.png)
  install (FILES resources/icon-16.png DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/16x16/apps" RENAME tev.png)
endif()
