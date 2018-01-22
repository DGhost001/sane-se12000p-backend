project(mustekDriver2)
cmake_minimum_required(VERSION 2.8)
find_package (Threads)
set(SRC_LIST
    a4s2600.cpp
    a4s2600.hpp
    parallelport.cpp
    parallelport.hpp
    scannercontrol.cpp
    scannercontrol.hpp
    wm8144.cpp
    wm8144.hpp
    sane-backend.cpp
    sanedevicehandle.cpp
    sanedevicehandle.hpp
)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=gnu++11)

add_library(sane-se12000p SHARED ${SRC_LIST})
target_link_libraries(sane-se12000p  ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(sane-se12000p PROPERTIES VERSION "1.0.25"
                                           SOVERSION 1)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}
                       sane-se12000p
)

