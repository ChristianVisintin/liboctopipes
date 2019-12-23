cmake_minimum_required(VERSION 3.0)
project(liboctopipes VERSION 0.1.0)

set (ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set (ROOT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/")
set (OCTOPIPES_C_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../liboctopipes/")

add_subdirectory(OCTOPIPES_C_DIR)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)
#Includes
file(GLOB INCLUDE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/octopipes/*.hpp"
)

#Library
file(GLOB LIBOCTOPIPESPP_SRC
  "${ROOT_SOURCE_DIR}/*.cpp"
)

#Clients
set (CLIENT_SEND_SRC "${ROOT_CLIENT_DIR}/octopipes_send.c")
set (CLIENT_SUB_SRC "${ROOT_CLIENT_DIR}/octopipes_sub.c")

#Tests source
file(GLOB PIPES_TEST_SRC
  "${ROOT_TESTS_DIR}/pipes/*.c"
)
file(GLOB PARSER_TEST_SRC
  "${ROOT_TESTS_DIR}/parser/*.c"
)

set(CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE RELEASE)

add_library(octopipespp_shared SHARED ${LIBOCTOPIPESPP_SRC})
set_target_properties(octopipes_shared PROPERTIES OUTPUT_NAME octopipespp)
set_target_properties(octopipes_shared PROPERTIES VERSION 0.1.0)
target_link_libraries(octopipes_shared -lpthread -loctopipes)
add_library(octopipespp_static STATIC ${LIBOCTOPIPESPP_SRC})
set_target_properties(octopipespp_static PROPERTIES OUTPUT_NAME octopipespp)
target_link_libraries(octopipespp_static -lpthread -loctopipes)

#Build tests
#add_executable(test_pipes ${PIPES_TEST_SRC})
#target_link_libraries(test_pipes octopipes_shared -lpthread)
#add_executable(test_parser ${PARSER_TEST_SRC})
#target_link_libraries(test_parser octopipes_shared -lpthread)
#if(CMAKE_COMPILER_IS_GNUCXX)
#  target_compile_options(test_pipes PUBLIC "--coverage")
#  target_compile_options(test_parser PUBLIC "--coverage")
#  target_link_libraries(test_parser gcov)
#  target_link_libraries(test_pipes gcov)
#endif()
#Install rules
install(TARGETS octopipespp_shared CONFIGURATIONS Release LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(TARGETS octopipespp_static CONFIGURATIONS Release ARCHIVE DESTINATION lib)
INSTALL(FILES ${INCLUDE_FILES} DESTINATION include/octopipes/)
