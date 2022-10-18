cmake_minimum_required(VERSION 2.8)
project(wyze)

set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_CXX_FLAGES "$ENV(CXXFLAGS) -rdynamic -O0 -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

link_directories(/usr/local/lib/yaml-cpp)

set(LIB_SRC
    wyze/config.cpp
    wyze/fiber.cpp
    wyze/iomanager.cpp
    wyze/log.cpp
    wyze/scheduler.cpp
    wyze/thread.cpp
    wyze/util.cpp
    )

add_library(wyze SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test wyze)
target_link_libraries(test wyze yaml-cpp pthread)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config wyze)
target_link_libraries(test_config wyze yaml-cpp pthread)

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread wyze)
target_link_libraries(test_thread wyze yaml-cpp pthread)

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util wyze)
target_link_libraries(test_util wyze yaml-cpp pthread)

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber wyze)
target_link_libraries(test_fiber wyze yaml-cpp pthread)

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler wyze)
target_link_libraries(test_scheduler wyze yaml-cpp pthread)

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager wyze)
target_link_libraries(test_iomanager wyze yaml-cpp pthread)



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)