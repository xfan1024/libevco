cmake_minimum_required(VERSION 3.10)
project(coroutine)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)

find_package(Libev REQUIRED)
find_package(Boost REQUIRED COMPONENTS coroutine)

file(GLOB_RECURSE EVCO_SOURCES src/*.cpp)
add_library(evco STATIC ${EVCO_SOURCES})
# add compile option on evco
target_compile_options(evco PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(evco PUBLIC include)
file(GLOB_RECURSE EVCO_PUBLIC_HEADERS include/*.h)

set_target_properties(evco PROPERTIES PUBLIC_HEADER "${EVCO_PUBLIC_HEADERS}")
target_link_libraries(evco PUBLIC Boost::coroutine)
target_link_libraries(evco PUBLIC ${LIBEV_LIBRARIES})

add_subdirectory(examples)
