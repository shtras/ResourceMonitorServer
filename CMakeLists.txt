# CMakeList.txt : CMake project for ResourceMonitorServer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.24)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (ResourceMonitorServer VERSION 0.1)

add_library(resource_monitor_server_compiler_flags INTERFACE)
target_compile_features(resource_monitor_server_compiler_flags INTERFACE cxx_std_20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Simple-Web-Server)

find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)

add_executable (ResourceMonitorServer main.cpp Metric.cpp Server.cpp)

target_link_libraries(ResourceMonitorServer PUBLIC 
					resource_monitor_server_compiler_flags 
					spdlog::spdlog_header_only 
					pdh 
					simple-web-server
					nlohmann_json::nlohmann_json)
target_compile_definitions(ResourceMonitorServer PRIVATE SPDLOG_WCHAR_TO_UTF8_SUPPORT)
target_compile_definitions(ResourceMonitorServer PRIVATE UNICODE)
