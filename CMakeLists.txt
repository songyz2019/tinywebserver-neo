cmake_minimum_required(VERSION 3.14...3.22)
project(
  tinywebsrv
  VERSION 1.0
  LANGUAGES CXX
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")


add_executable(tinywebsrv ${sources})

target_include_directories(
  ${PROJECT_NAME} PUBLIC "include/" "/usr/include/mysql"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")
