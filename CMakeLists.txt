cmake_minimum_required(VERSION 3.14)
project(bej_tool C CXX)  

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)            # <--- C++11 for GTest
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable strict warnings for GCC/Clang
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wcast-align -Wconversion
                      -Wdouble-promotion -Wformat=2 -Wno-unused-parameter)
endif()

# Library sources (without main.c!)
set(BEJ_SOURCES
    src/bej_reader.c
    src/bej_json.c
    src/bej_dict.c
    src/bej_decode.c
    src/main.c
)

# Headers
set(BEJ_HEADERS
    src/bej.h
    src/bej_reader.h
    src/bej_json.h
    src/bej_dict.h
    src/bej_decode.h
)

# Create static library
add_library(bej STATIC ${BEJ_SOURCES})
target_include_directories(bej PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Executable
add_executable(bej_tool src/main.c)
target_link_libraries(bej_tool PRIVATE bej)

# Run target
add_custom_target(run
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bej_tool -s Memory_v1.bin -a annotation.bin -b example.bin -o out.json
  DEPENDS bej_tool
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running bej_tool with default arguments"
)

# Installation (optional)
install(TARGETS bej_tool RUNTIME DESTINATION bin)
install(TARGETS bej         ARCHIVE DESTINATION lib)
install(FILES   ${BEJ_HEADERS} DESTINATION include/bej)

# Documentation generation with Doxygen (if found)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  set(DOXYGEN_IN  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} COPYONLY)
  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()

# --- Tests (GoogleTest via FetchContent) ---

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
  enable_language(CXX)
  include(FetchContent)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(bej_tests tests/test_min.cpp)
  target_link_libraries(bej_tests PRIVATE bej GTest::gtest GTest::gtest_main)
  target_include_directories(bej_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

  # If MinGW: sometimes needs ANSI stdio and pthreads
  if (MINGW)
    target_compile_definitions(bej PRIVATE __USE_MINGW_ANSI_STDIO=1)
    target_compile_definitions(bej_tool PRIVATE __USE_MINGW_ANSI_STDIO=1)
    target_compile_definitions(bej_tests PRIVATE __USE_MINGW_ANSI_STDIO=1)
  endif()

  enable_testing()
  add_test(NAME bej_unit_tests COMMAND $<TARGET_FILE:bej_tests>)

  add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS bej_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

# --- Lightweight C tests (no C++, no threads) ---
option(BUILD_MIN_C_TESTS "Build minimal C unit tests" ON)

if(BUILD_MIN_C_TESTS)
  add_executable(bej_tests_c tests/test_bej_c.c)
  target_include_directories(bej_tests_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(bej_tests_c PRIVATE bej)

  # Register in ctest
  enable_testing()
  add_test(NAME bej_min_c_tests COMMAND $<TARGET_FILE:bej_tests_c>)
endif()
