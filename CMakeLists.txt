cmake_minimum_required(VERSION 3.20)
project(zmq-proxy LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/binaries/)

# Apply properties to all executables at once
function(apply_target_to_executable_names)
  get_directory_property(targets BUILDSYSTEM_TARGETS)
  foreach(target ${targets})
      get_target_property(target_type ${target} TYPE)
      if(target_type STREQUAL "EXECUTABLE")
          # Do something with each executable
          set_target_properties(${target} PROPERTIES OUTPUT_NAME "${target}")
      endif()

      # Only strip in Release builds
      if(CMAKE_BUILD_TYPE STREQUAL "Release")
          add_custom_command(TARGET ${target} POST_BUILD
          COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${target}> COMMENT "Stripping ${target}")
      endif()
  endforeach()
endfunction()

include(FetchContent)


# Fetch docopt
FetchContent_Declare(
  docopt
  GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
  GIT_SHALLOW TRUE
  GIT_TAG v0.6.3
)
FetchContent_MakeAvailable(docopt)


set(ZLIB_USE_STATIC_LIBS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZLIB REQUIRED zlib)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(zmq-proxy src/zmq-proxy.cpp)
target_include_directories(zmq-proxy PRIVATE
                                     ${docopt_SOURCE_DIR}
                                     ${ZMQ_INCLUDE_DIRS})
target_link_libraries(zmq-proxy PRIVATE ${ZMQ_LIBRARIES} docopt_s)

