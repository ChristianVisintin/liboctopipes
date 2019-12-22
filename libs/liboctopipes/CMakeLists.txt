cmake_minimum_required(VERSION 3.0)
project(liboctopipes VERSION 0.1.0)

set (ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set (ROOT_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/clients/")
set (ROOT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/")
include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)
#Includes
file(GLOB INCLUDE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/octopipes/*.h"
)

#Library
file(GLOB LIBOCTOPIPES_SRC
  "${ROOT_SOURCE_DIR}/*.c"
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
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE RELEASE)

add_library(octopipes_shared SHARED ${LIBOCTOPIPES_SRC})
set_target_properties(octopipes_shared PROPERTIES OUTPUT_NAME octopipes)
set_target_properties(octopipes_shared PROPERTIES VERSION 0.1.0)
target_link_libraries(octopipes_shared -lpthread)
add_library(octopipes_static STATIC ${LIBOCTOPIPES_SRC})
set_target_properties(octopipes_static PROPERTIES OUTPUT_NAME octopipes)
target_link_libraries(octopipes_static -lpthread)

#Build clients
add_executable(octopipes_send ${CLIENT_SEND_SRC})
target_link_libraries(octopipes_send octopipes_shared -lpthread)
add_executable(octopipes_sub ${CLIENT_SUB_SRC})
target_link_libraries(octopipes_sub octopipes_shared -lpthread)

#Build tests
add_executable(test_pipes ${PIPES_TEST_SRC})
target_link_libraries(test_pipes octopipes_shared -lpthread)
add_executable(test_parser ${PARSER_TEST_SRC})
target_link_libraries(test_parser octopipes_shared -lpthread)
if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(test_pipes PUBLIC "--coverage")
  target_compile_options(test_parser PUBLIC "--coverage")
  target_link_libraries(test_parser gcov)
  target_link_libraries(test_pipes gcov)
endif()
#Install rules
install(TARGETS octopipes_shared CONFIGURATIONS Release LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(TARGETS octopipes_static CONFIGURATIONS Release ARCHIVE DESTINATION lib)
install(TARGETS octopipes_send CONFIGURATIONS Release RUNTIME DESTINATION bin)
install(TARGETS octopipes_sub CONFIGURATIONS Release RUNTIME DESTINATION bin)
INSTALL(FILES ${INCLUDE_FILES} DESTINATION include/octopipes/)
