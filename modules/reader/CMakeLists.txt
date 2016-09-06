#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for the reader module
cmake_minimum_required(VERSION 2.8.11)

set(reader_sources
	./src/reader.c
)

set(reader_headers
	./inc/reader.h
)

set(reader_static_sources
	${reader_sources}
)

set(reader_static_headers
	${reader_headers}
)

set(reader_hl_sources
	./src/reader_hl.c
)

set(reader_hl_headers
	./inc/reader_hl.h
)

set(reader_hl_static_sources
	${reader_hl_sources}
)

set(reader_hl_static_headers
	${reader_hl_headers}
)


include_directories(./inc)
include_directories(${GW_INC})




#this builds the reader dynamic library
add_library(reader MODULE ${reader_sources}  ${reader_headers})
target_link_libraries(reader gateway)

#this builds the reader static library
add_library(reader_static  ${reader_static_sources} ${reader_static_headers})
target_compile_definitions(reader_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(reader_static gateway)

#this builds the reader dynamic library (by default it uses reader module linked statically)
add_library(reader_hl MODULE ${reader_hl_sources} ${reader_hl_headers})
target_link_libraries(reader_hl reader_static gateway)

#this builds the reader static library (by default it uses reader module linked statically)
add_library(reader_hl_static ${reader_hl_static_sources} ${reader_hl_static_headers})
target_compile_definitions(reader_hl_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(reader_hl_static reader_static gateway)

linkSharedUtil(reader)
linkSharedUtil(reader_static)
linkSharedUtil(reader_hl)
linkSharedUtil(reader_hl_static)

add_module_to_solution(reader)
if(install_executables)
	install(TARGETS reader LIBRARY DESTINATION lib) 
    install(TARGETS reader_hl LIBRARY DESTINATION lib) 
endif()

