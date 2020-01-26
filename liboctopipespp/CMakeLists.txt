cmake_minimum_required(VERSION 3.0)
project(liboctopipes VERSION 0.1.0)

set (ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set (ROOT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/")
set (OCTOPIPES_C_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
set (OCTOPIPES_C_DIR_BIN "${CMAKE_CURRENT_BINARY_DIR}/liboctopipes/")

add_subdirectory(${OCTOPIPES_C_DIR} ${OCTOPIPES_C_DIR_BIN})

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  "${OCTOPIPES_C_DIR}/include/"
)
#Includes
file(GLOB INCLUDE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/octopipespp/*.hpp"
)

#Library
file(GLOB LIBOCTOPIPESPP_SRC
  "${ROOT_SOURCE_DIR}/*.cpp"
)

#Tests source
file(GLOB PARSER_TEST_SRC
  "${ROOT_TESTS_DIR}/parser/*.cpp"
)
file(GLOB CLIENT_TEST_SRC
  "${ROOT_TESTS_DIR}/client/*.cpp"
)
file(GLOB SERVER_TEST_SRC
  "${ROOT_TESTS_DIR}/server/*.cpp"
)

set(CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE RELEASE)

add_library(octopipespp_shared SHARED ${LIBOCTOPIPESPP_SRC})
set_target_properties(octopipespp_shared PROPERTIES OUTPUT_NAME octopipespp)
set_target_properties(octopipespp_shared PROPERTIES VERSION 0.1.0)
target_link_libraries(octopipespp_shared -lpthread octopipes_shared)
add_library(octopipespp_static STATIC ${LIBOCTOPIPESPP_SRC})
set_target_properties(octopipespp_static PROPERTIES OUTPUT_NAME octopipespp)
target_link_libraries(octopipespp_static -lpthread octopipes_static)

#Build tests
add_executable(test_parserpp ${PARSER_TEST_SRC})
target_link_libraries(test_parserpp octopipespp_shared)
add_executable(test_clientpp ${CLIENT_TEST_SRC})
target_link_libraries(test_clientpp octopipespp_shared)
add_executable(test_serverpp ${SERVER_TEST_SRC})
target_link_libraries(test_serverpp octopipespp_shared)
if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(test_parserpp PUBLIC "--coverage")
  target_link_libraries(test_parserpp gcov)
  target_compile_options(test_clientpp PUBLIC "--coverage")
  target_link_libraries(test_clientpp gcov)
  target_compile_options(test_serverpp PUBLIC "--coverage")
  target_link_libraries(test_serverpp gcov)
endif()
#Install rules
install(TARGETS octopipespp_shared CONFIGURATIONS Release LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(TARGETS octopipespp_static CONFIGURATIONS Release ARCHIVE DESTINATION lib)
INSTALL(FILES ${INCLUDE_FILES} DESTINATION include/octopipespp/)
