cmake_minimum_required(VERSION 3.0)
project(liboctopipes VERSION 0.1.0)

set (ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set (ROOT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/")
include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  )

file(GLOB LIBOCTOPIPES_SRC
  "${ROOT_SOURCE_DIR}/*.c"
)

file(GLOB PIPES_TEST_SRC
  "${ROOT_TESTS_DIR}/pipes/*.c"
)

set(CXX_FLAGS "-g -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE RELEASE)

add_library(octopipes_shared SHARED ${LIBOCTOPIPES_SRC})
set_target_properties(octopipes_shared PROPERTIES OUTPUT_NAME octopipes)
set_target_properties(octopipes_shared PROPERTIES VERSION 0.1.0)
target_link_libraries(octopipes_shared -lpthread)
add_library(octopipes_static STATIC ${LIBOCTOPIPES_SRC})
set_target_properties(octopipes_static PROPERTIES OUTPUT_NAME octopipes)
target_link_libraries(octopipes_static -lpthread)

#Build tests
add_executable(test_pipes ${PIPES_TEST_SRC})
target_compile_definitions(test_pipes PRIVATE STANDALONE)
target_link_libraries(test_pipes octopipes_shared -lpthread)
