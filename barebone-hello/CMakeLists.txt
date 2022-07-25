cmake_minimum_required(VERSION 3.14)
project(hello)

add_executable(hello.elf hello.c)
target_include_directories(hello.elf PUBLIC "${user_space_inc}")

target_link_options (hello.elf 
    PUBLIC "-Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/hello.ld,-melf_i386,-dn")
set_target_properties(hello.elf PROPERTIES POSITION_INDEPENDENT_CODE True)
