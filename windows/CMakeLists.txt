# Build the KazType Windows input method (a TSF text service DLL).
#   cmake -B build -A x64   && cmake --build build --config Release
#   cmake -B build32 -A Win32 && cmake --build build32 --config Release
# The core library and its tests also build on macOS/Linux:
#   cmake -B build && cmake --build build && ctest --test-dir build
cmake_minimum_required(VERSION 3.20)
project(KazType LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8 /W4 /permissive-)
  # One DLL with no runtime dependency: it loads into every app.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(kazcore STATIC
  core/Converter.cpp core/Tagger.cpp core/Text.cpp core/UserDict.cpp core/Vocab.cpp)
target_include_directories(kazcore PUBLIC core)

enable_testing()
add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests kazcore)
add_test(NAME core_tests COMMAND core_tests ${CMAKE_CURRENT_SOURCE_DIR}/Resources/kaztype.bin)

if(WIN32)
  add_library(KazType SHARED
    tsf/CandidateWindow.cpp tsf/Dll.cpp tsf/Globals.cpp tsf/Keys.cpp tsf/TextService.cpp
    tsf/KazType.def tsf/KazType.rc)
  target_compile_definitions(KazType PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
  target_link_libraries(KazType PRIVATE kazcore dwmapi shell32 ole32 oleaut32 uuid advapi32 user32 gdi32)
  if(MINGW)
    target_link_options(KazType PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  # The model sits next to the DLL.
  add_custom_command(TARGET KazType POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/kaztype.bin $<TARGET_FILE_DIR:KazType>/kaztype.bin)
endif()
