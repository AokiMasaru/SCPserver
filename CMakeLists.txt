cmake_minimum_required(VERSION 3.15)
project(SCPserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_LIBSSH "Build with libssh support (recommended for real SCP)" ON)

# libssh の有無を検出して安全に扱う
set(LIBSSH_AVAILABLE OFF)
if(USE_LIBSSH)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBSSH libssh)
  if(LIBSSH_FOUND)
    message(STATUS "Found libssh: ${LIBSSH_VERSION}")
    add_definitions(-DHAVE_LIBSSH)
    # include / link 情報はターゲットに付与する
    set(LIBSSH_AVAILABLE ON)
    set(LIBSSH_INCLUDE_DIRS ${LIBSSH_INCLUDE_DIRS})
    set(LIBSSH_LIBRARY_DIRS ${LIBSSH_LIBRARY_DIRS})
    set(LIBSSH_LIBRARIES ${LIBSSH_LIBRARIES})
  else()
    message(FATAL_ERROR "libssh not found but -DUSE_LIBSSH=ON. Install libssh-dev (Debian/Ubuntu: libssh-dev pkg-config) or re-run cmake with -DUSE_LIBSSH=OFF")
  endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES src/*.cpp)

add_executable(scpserver ${SOURCES})

if(LIBSSH_AVAILABLE)
  target_include_directories(scpserver PRIVATE ${LIBSSH_INCLUDE_DIRS})
  target_link_directories(scpserver PRIVATE ${LIBSSH_LIBRARY_DIRS})
  target_link_libraries(scpserver PRIVATE ${LIBSSH_LIBRARIES})
endif()

# スタティックライブラリとして実装部分を分離
add_library(scpserver_lib STATIC
    src/scp_server.cpp
)

if(LIBSSH_AVAILABLE)
    target_include_directories(scpserver_lib PRIVATE ${LIBSSH_INCLUDE_DIRS})
    target_link_directories(scpserver_lib PRIVATE ${LIBSSH_LIBRARY_DIRS})
    target_link_libraries(scpserver_lib PRIVATE ${LIBSSH_LIBRARIES})
endif()

# メイン実行ファイルは scpserver_lib をリンク
target_link_libraries(scpserver PRIVATE scpserver_lib)

# テストを有効化
enable_testing()

# Google Test のインストールチェック
find_package(GTest REQUIRED)

# テストディレクトリの追加
add_subdirectory(tests)
