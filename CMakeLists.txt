cmake_minimum_required(VERSION 2.8.12)

project(znctwitch)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

find_package(ZNC REQUIRED)
find_package(CURL REQUIRED)

set(twitch_SOURCES
	twitchtmi.cpp
	twitchtmi.h
	jload.cpp
	jload.h
	json/jsoncpp.cpp
	json/json.h)

add_library(twitch MODULE
	${twitch_SOURCES})
target_link_libraries(twitch
	${ZNC_LIBRARIES}
	${CURL_LIBRARIES})
target_include_directories(twitch SYSTEM
	PRIVATE
		${ZNC_INCLUDE_DIRS}
		${CURL_INCLUDE_DIRS})
target_compile_options(twitch
	PRIVATE ${ZNC_DEFINITIONS} -Wno-shadow
	PUBLIC -fno-strict-aliasing
	PUBLIC -std=gnu++11)

set_target_properties(twitch PROPERTIES
	PREFIX "")
