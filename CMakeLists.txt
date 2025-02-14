cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

project(telegram-curses VERSION 0.4.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  message(STATUS "Found ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else()
  message(STATUS "Could NOT find ccache")
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  set(GCC 1)
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
  set(CLANG 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
  set(INTEL 1)
elseif (NOT MSVC)
  message(FATAL_ERROR "Compiler isn't supported")
endif()

include(CheckCXXCompilerFlag)

if (GCC OR CLANG OR INTEL)
  if (WIN32 AND INTEL)
    set(STD17_FLAG /Qstd=c++17)
  else()
    set(STD17_FLAG -std=c++17)
  endif()
  check_cxx_compiler_flag(${STD17_FLAG} HAVE_STD17)
  if (NOT HAVE_STD17)
    string(REPLACE "c++17" "c++1y" STD17_FLAG "${STD17_FLAG}")
    check_cxx_compiler_flag(${STD17_FLAG} HAVE_STD1Y)
    set(HAVE_STD17 ${HAVE_STD1Y})
  endif()
elseif (MSVC)
  set(HAVE_STD17 MSVC_VERSION>=1900)
endif()

if (NOT HAVE_STD17)
  message(FATAL_ERROR "No C++17 support in the compiler. Please upgrade the compiler.")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
if (THREADS_HAVE_PTHREAD_ARG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

if (MSVC)
  if (CMAKE_CXX_FLAGS_DEBUG MATCHES "/RTC1")
    string(REPLACE "/RTC1" " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  endif()
  add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR- /W4 /wd4100 /wd4127 /wd4324 /wd4505 /wd4702")
elseif (CLANG OR GCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STD17_FLAG} -fno-omit-frame-pointer")
  if (APPLE)
    #use "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/export_list" for exported symbols
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fvisibility=hidden -Wl,-dead_strip,-x,-S")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,ALL")
  endif()
elseif (INTEL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STD17_FLAG}")
endif()

if (WIN32)
  add_definitions(-DNTDDI_VERSION=0x06020000 -DWINVER=0x0602 -D_WIN32_WINNT=0x0602 -DNOMINMAX -DUNICODE -D_UNICODE)
endif()
if (CYGWIN)
  add_definitions(-D_DEFAULT_SOURCE=1 -DFD_SETSIZE=4096)
endif()

if (NOT ANDROID) # _FILE_OFFSET_BITS is broken in ndk r15 and r15b and doesn't work prior to Android 7.0
  add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

include(AddCXXCompilerFlag)
if (NOT MSVC)
  add_cxx_compiler_flag("-Wall")
endif()
add_cxx_compiler_flag("-Wextra")
add_cxx_compiler_flag("-Wimplicit-fallthrough=2")
add_cxx_compiler_flag("-Wpointer-arith")
add_cxx_compiler_flag("-Wcast-qual")
add_cxx_compiler_flag("-Wsign-compare")
add_cxx_compiler_flag("-Wduplicated-branches")
add_cxx_compiler_flag("-Wduplicated-cond")
add_cxx_compiler_flag("-Walloc-zero")
add_cxx_compiler_flag("-Wlogical-op")
add_cxx_compiler_flag("-Wno-tautological-compare")
add_cxx_compiler_flag("-Wpointer-arith")
add_cxx_compiler_flag("-Wvla")
add_cxx_compiler_flag("-Wnon-virtual-dtor")
add_cxx_compiler_flag("-Wno-unused-parameter")
add_cxx_compiler_flag("-Wconversion")
add_cxx_compiler_flag("-Wno-sign-conversion")
add_cxx_compiler_flag("-Qunused-arguments")
add_cxx_compiler_flag("-Wno-unused-private-field")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem /usr/include/c++/v1")
if (CLANG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)

#Compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include (CheckIncludeFiles)
check_include_files (execinfo.h HAVE_EXECINFO_H)
if (HAVE_EXECINFO_H)
  add_definitions("-DHAVE_EXECINFO_H=1")
endif (HAVE_EXECINFO_H)

check_include_files (pwd.h HAVE_PWD_H)
if (HAVE_PWD_H)
  add_definitions("-DHAVE_PWD_H=1")
endif (HAVE_PWD_H)

add_subdirectory(td)
add_subdirectory(third-party/libtickit)
add_subdirectory(third-party/notcurses)

add_executable(telegram-curses 
  windows/BackendNotcurses.cpp
  windows/BackendNotcurses.h
  windows/BackendTickit.cpp
  windows/BackendTickit.h  
  windows/BorderedWindow.hpp
  windows/BorderedWindow.cpp
  windows/BorderedWindow.hpp
  windows/EditorWindow.cpp
  windows/EditorWindow.hpp
  windows/EmptyWindow.cpp
  windows/EmptyWindow.hpp
  windows/Input.cpp
  windows/Input.hpp
  windows/LogWindow.hpp
  windows/Markup.hpp
  windows/OneLineInputWindow.cpp
  windows/OneLineInputWindow.hpp
  windows/Output.cpp
  windows/Output.hpp
  windows/PadWindow.cpp
  windows/PadWindow.hpp
  windows/Screen.cpp
  windows/Screen.hpp
  windows/SelectionWindow.hpp
  windows/TextEdit.cpp
  windows/TextEdit.hpp
  windows/unicode.cpp
  windows/unicode.h
  windows/ViewWindow.cpp
  windows/ViewWindow.hpp
  windows/Window.cpp
  windows/Window.hpp
  windows/WindowLayout.cpp
  windows/WindowLayout.hpp

  ChatInfoWindow.cpp
  ChatInfoWindow.hpp
  ChatManager.cpp
  ChatManager.hpp
  ChatSearchWindow.cpp
  ChatSearchWindow.hpp
  ChatWindow.cpp
  ChatWindow.hpp
  CommandLineWindow.hpp
  CommonGroupsWindow.cpp
  CommonGroupsWindow.hpp
  ComposeWindow.cpp
  ComposeWindow.hpp
  ConfigWindow.cpp
  ConfigWindow.hpp
  Debug.cpp
  Debug.hpp
  DebugInfoWindow.cpp
  DebugInfoWindow.hpp
  DialogListWindow.cpp
  DialogListWindow.hpp
  FieldEditWindow.hpp
  FileManager.cpp
  FileManager.hpp
  FolderSelectionWindow.hpp
  GlobalParameters.cpp
  GlobalParameters.hpp
  GroupMembersWindow.cpp
  GroupMembersWindow.hpp
  MenuWindow.cpp
  MenuWindow.hpp
  MenuWindowCommon.cpp
  MenuWindowCommon.hpp
  MenuWindowPad.hpp
  MenuWindowView.hpp
  MessageInfoWindow.cpp
  MessageInfoWindow.hpp
  MessageProcess.cpp
  MessageProcess.hpp
  NotificationManager.cpp
  NotificationManager.hpp
  Outputter.cpp
  Outputter.hpp
  ReactionSelectionWindow.cpp
  ReactionSelectionWindow.hpp
  StatusLineWindow.hpp
  StickerManager.cpp
  StickerManager.hpp
  Tdcurses.cpp
  Tdcurses.hpp
  TdcursesLayout.cpp
  TdcursesLayout.hpp
  TdcursesWindowBase.cpp
  TdcursesWindowBase.hpp
  TdObjectsOutput.cpp
  TdObjectsOutput.h
  YesNoWindow.hpp
  
  settings-menu/AboutWindow.cpp
  settings-menu/AboutWindow.hpp
  settings-menu/AccountSettingsWindow.cpp
  settings-menu/AccountSettingsWindow.hpp
  settings-menu/ActiveSessionsWindow.cpp
  settings-menu/ActiveSessionsWindow.hpp
  settings-menu/ActiveSessionWindow.cpp
  settings-menu/ActiveSessionWindow.hpp
  settings-menu/AdvancedSettingsWindow.cpp
  settings-menu/AdvancedSettingsWindow.hpp
  settings-menu/ChangeScopeNotificationsSettingsWindow.cpp
  settings-menu/ChangeScopeNotificationsSettingsWindow.hpp
  settings-menu/ChatSettingsWindow.cpp
  settings-menu/ChatSettingsWindow.hpp
  settings-menu/FoldersSettingsWindow.cpp
  settings-menu/FoldersSettingsWindow.hpp
  settings-menu/MainSettingsWindow.cpp
  settings-menu/MainSettingsWindow.hpp
  settings-menu/NotificationsSettingsWindow.cpp
  settings-menu/NotificationsSettingsWindow.hpp
  settings-menu/PrivacyAndSecuritySettingsWindow.cpp
  settings-menu/PrivacyAndSecuritySettingsWindow.hpp
  settings-menu/ScopeNotificationsSettingsWindow.cpp
  settings-menu/ScopeNotificationsSettingsWindow.hpp

  port/dirlist.cpp
  port/dirlist.h

  third-party/qrcodegen/qrcodegen.cpp 
)
target_link_libraries(telegram-curses PUBLIC TdStatic tdutils icuio icui18n icuuc icudata config++ tickit utf8proc notcurses-core notcurses notify jpeg ${GDKPIXBUF_LDFLAGS} ${GLIB_LDFLAGS} )
target_include_directories(telegram-curses PRIVATE ${CURSES_INCUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${GDKPIXBUF_INCLUDE_DIRS})
target_include_directories(telegram-curses PRIVATE
  ${TL_TD_AUTO_INCLUDES}
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/td/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/td/tddb/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/td/tdnet/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/td/tdactor/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/td/tdutils/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/td/td/generate/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party/>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party/notcurses/include/>
  )
set_target_properties(telegram-curses PROPERTIES
  VERSION ${PROJECT_VERSION}
)
