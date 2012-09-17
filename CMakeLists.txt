# @@@LICENSE
#
# Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# LICENSE@@@

cmake_minimum_required(VERSION 2.8.7)

project(activitymanager CXX)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(2 0 0)

include(FindPkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(OPENSSL REQUIRED openssl)
include_directories(${OPENSSL_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${OPENSSL_CFLAGS_OTHER})

pkg_check_modules(LS2 REQUIRED luna-service2)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED     OFF)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost REQUIRED COMPONENTS regex)
include_directories(Boost_INCLUDE_DIRS)

find_library(MJCORE NAMES mojocore libmojocore)
if(${MJCORE} STREQUAL MJCORE-NOTFOUND)
	message(FATAL_ERROR "Failed to find libmojocore. Please install openwebos/db8")
endif()

find_library(MJLUNA NAMES mojoluna libmojoluna)
if(${MJLUNA} STREQUAL MJLUNA-NOTFOUND)
	message(FATAL_ERROR "Failed to find libmojoluna. Please install openwebos/db8")
endif()

find_library(MJDB NAMES mojodb libmojodb)
if(${MJDB} STREQUAL MJDB-NOTFOUND)
	message(FATAL_ERROR "Failed to find libmojodb. Please install openwebos/db8")
endif()

find_path(MJ_INC NAMES mojodb/core/MojErr.h)
if(MJ_INC STREQUAL "MJ_INC-NOTFOUND")
	message(FATAL_ERROR "Failed to find db8 include files")
endif()
include_directories(${MJ_INC}/mojodb)
webos_add_compiler_flags(ALL -I${MJ_INC}/mojodb)

# There is a call to system("initctl ...") in ServiceApp.cpp which is compiled in for
# all implementations except WEBOS_TARGET_MACHINE_IMPL_SIMULATOR
webos_machine_impl_dep()

include_directories(include/internal)
webos_add_compiler_flags(ALL -Wall -Wno-error=strict-aliasing -Wconversion -DMOJ_LINUX -DBOOST_SP_DISABLE_THREADS )

# MojoDb generates warnings in core/MojObject.h, so no -Werror
#webos_add_compiler_flags(ALL -Werror )

webos_add_linker_options (ALL "--no-undefined")

file(GLOB SOURCE_FILES src/*.cpp)

add_executable(activitymanager ${SOURCE_FILES})
target_link_libraries(activitymanager
                      ${MJCORE}
                      ${MJLUNA}                    
                      ${MJDB}                     
                      ${Boost_LIBRARIES}
                      ${GLIB2_LDFLAGS}
                      ${OPENSSL_LDFLAGS}
                      ${LS2_LDFLAGS}
                     )

webos_build_daemon()
webos_build_system_bus_files()
webos_build_db8_files()
