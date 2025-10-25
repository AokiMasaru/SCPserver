cmake_minimum_required(VERSION 3.15)
project(SCPserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_LIBSSH "Build with libssh support (recommended for real SCP)" ON)

if(USE_LIBSSH)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBSSH libssh)
  if(LIBSSH_FOUND)
    message(STATUS "Found libssh: ${LIBSSH_VERSION}")
    add_definitions(-DHAVE_LIBSSH)
    include_directories(${LIBSSH_INCLUDE_DIRS})
    link_directories(${LIBSSH_LIBRARY_DIRS})
  else()
    message(WARNING "libssh not found — building fallback TCP skeleton. To enable libssh install libssh-dev and re-run cmake with -DUSE_LIBSSH=ON")
  endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES src/*.cpp)

add_executable(scpserver ${SOURCES})

if(LIBSSH_FOUND)
  target_link_libraries(scpserver ${LIBSSH_LIBRARIES})
endif()

enable_testing()
