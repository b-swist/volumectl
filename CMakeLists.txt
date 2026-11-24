cmake_minimum_required(VERSION 3.20)

project(volumectl C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPUSE REQUIRED IMPORTED_TARGET libpulse)

add_executable(volumectl
	volumectl.c
	common.c
	pulseaudio.c)

target_compile_options(-Wall -Wextra -Wpedantic)
target_link_libraries(volumectl PUBLIC
	PkgConfig::LIBPUSE
)
