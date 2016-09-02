#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for the iothubhttprelay module

set(iothubhttprelay_sources
	./src/iothubhttprelay.c
)

set(iothubhttprelay_headers
	./inc/iothubhttprelay.h
)

set(iothubhttprelay_hl_sources
	./src/iothubhttprelay_hl.c
)

set(iothubhttprelay_hl_headers
	./inc/iothubhttprelay_hl.h
)

include_directories(./inc)
include_directories(${GW_INC})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})

#this builds the IoTHubHTTPRelay dynamic library
add_library(iothubhttprelay MODULE
    ${iothubhttprelay_sources}
    ${iothubhttprelay_headers}
)
target_link_libraries(iothubhttprelay
    gateway
    iothub_client
    iothub_client_http_transport
)

#this builds the IoTHubHTTPRelay static library
add_library(iothubhttprelay_static
    ${iothubhttprelay_sources}
    ${iothubhttprelay_headers}
)
target_compile_definitions(iothubhttprelay_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothubhttprelay_static
    gateway
    iothub_client
    iothub_client_http_transport
)

#this builds the IoTHubHTTPRelay_HL dynamic library (by default it uses IoTHub Module linked statically)
add_library(iothubhttprelay_hl MODULE
    ${iothubhttprelay_hl_sources}
    ${iothubhttprelay_hl_headers}
)
target_link_libraries(iothubhttprelay_hl
    iothubhttprelay_static
)

#this builds the IoTHubHTTPRelay_HL static library (by default it uses IoTHub Module linked statically)
add_library(iothubhttprelay_hl_static
    ${iothubhttprelay_hl_sources}
    ${iothubhttprelay_hl_headers}
)
target_compile_definitions(iothubhttprelay_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(iothubhttprelay_hl_static
    iothubhttprelay_static
)

linkSharedUtil(iothubhttprelay)
linkSharedUtil(iothubhttprelay_static)
linkSharedUtil(iothubhttprelay_hl)
linkSharedUtil(iothubhttprelay_hl_static)

linkHttp(iothubhttprelay)
linkHttp(iothubhttprelay_static)
linkHttp(iothubhttprelay_hl)
linkHttp(iothubhttprelay_hl_static)

add_module_to_solution(iothubhttprelay)

add_subdirectory(tests)

if(install_executables)
	install(TARGETS iothubhttprelay LIBRARY DESTINATION lib) 
    install(TARGETS iothubhttprelay_hl LIBRARY DESTINATION lib) 
endif()
