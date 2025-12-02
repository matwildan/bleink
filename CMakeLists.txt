cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(myown-ble-ht)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE
	src/main.c
	include/ble_rgb_service.c
	include/ble_ess_service.c
	include/display_epaper_cfb.c
	include/battery.c
)
