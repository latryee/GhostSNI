cmake_minimum_required(VERSION 3.15)
project(GhostSNI C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(GhostSNI
    src/main.c
    src/cli.c
    src/packet_parser.c
    src/injection.c
    src/tray.c
    ghost.rc
)

target_include_directories(GhostSNI PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(GhostSNI PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/WinDivert.lib
    ws2_32
    shell32
    gdi32
)

add_custom_command(TARGET GhostSNI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/WinDivert.dll"
        "$<TARGET_FILE_DIR:GhostSNI>/WinDivert.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/WinDivert64.sys"
        "$<TARGET_FILE_DIR:GhostSNI>/WinDivert64.sys"
)

if(MSVC)
    target_compile_options(GhostSNI PRIVATE /W4)
else()
    target_compile_options(GhostSNI PRIVATE -Wall -Wextra -pedantic)
endif()

if(MSVC)
    target_compile_options(GhostSNI PRIVATE $<$<CONFIG:Release>:/O2>)
else()
    target_compile_options(GhostSNI PRIVATE $<$<CONFIG:Release>:-O2>)
endif()
