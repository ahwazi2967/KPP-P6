cmake_minimum_required(VERSION 3.16)

project(communication)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(communication
    main.cpp
    serial.cpp
    interpolation.cpp
)

target_include_directories(communication PRIVATE
    /usr/local/include/libusbp-1
)

target_link_directories(communication PRIVATE
    /usr/local/lib
)

target_link_libraries(communication PRIVATE
    usbp-1
    udev
)
