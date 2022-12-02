cmake_minimum_required(VERSION 2.8)
project(wyze)

set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_CXX_FLAGES "$ENV(CXXFLAGS) -rdynamic -O0 -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

link_directories(/usr/local/lib/yaml-cpp)

set(LIB_SRC
    wyze/address.cpp
    wyze/bytearray.cpp
    wyze/config.cpp
    wyze/fdmanager.cpp
    wyze/fiber.cpp
    wyze/hook.cpp
    wyze/http/http.cpp
    wyze/http/http_parser.cpp
    wyze/http/http11_parser.cpp
    wyze/http/httpclient_parser.cpp
    wyze/iomanager.cpp
    wyze/log.cpp
    wyze/scheduler.cpp
    wyze/socket.cpp
    wyze/thread.cpp
    wyze/timer.cpp
    wyze/util.cpp
    )

set (LIBS 
        wyze
        yaml-cpp
        pthread
        dl
    )

add_library(wyze SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test wyze)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config wyze)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread wyze)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util wyze)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber wyze)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler wyze)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager wyze)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_timer tests/test_timer.cpp)
add_dependencies(test_timer wyze)
target_link_libraries(test_timer ${LIBS})

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook wyze)
target_link_libraries(test_hook ${LIBS})

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket wyze)
target_link_libraries(test_socket ${LIBS})

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address wyze)
target_link_libraries(test_address ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray wyze)
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http wyze)
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser wyze)
target_link_libraries(test_http_parser ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)