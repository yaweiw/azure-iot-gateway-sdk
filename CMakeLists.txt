#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
project(azure_iot_gateway_sdk)

set(GATEWAY_VERSION 0.0.2 CACHE INTERNAL "")
set(COMPANY_NAME "Microsoft")

if(WIN32)
    set(VERSION_PREFIX "${GATEWAY_VERSION}" CACHE PATH "Version Prefix")
else()
    set(VERSION_PREFIX "${PROJECT_NAME}-${GATEWAY_VERSION}" CACHE PATH "Version Prefix")
endif()

message(STATUS "Version prefix is ${VERSION_PREFIX}")

if(WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}-${VERSION_PREFIX}/" CACHE INTERNAL "CMake install prefix")
endif()

message(STATUS "CMake Install Prefix is ${CMAKE_INSTALL_PREFIX}")

#the following variables are project-wide and can be used with cmake-gui
option(skip_unittests "set skip_unittests to ON to skip unittests (default is OFF)[if possible, they are always built]" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF) [if possible, they are always built]" OFF)
option(install_executables "should cmake run cmake's install function (that includes dynamic link libraries) [it does for yocto]" OFF)
option(install_modules "should cmake install the default gateway modules" OFF)
option(enable_java_binding "set enable_java_binding to ON to enable building of Java binding (default is OFF)" OFF)
option(enable_nodejs_binding "set enable_nodejs_binding to ON to enable building of Node JS binding (default is OFF)" OFF)
option(run_as_a_service "Flags that we have the goal of running gateway as a service for samples and OS that supports it." OFF)
option(enable_dotnet_binding "set enable_dotnet_binding to ON to build dotnet binding host binaries (default is OFF)" OFF)
option(enable_ble_module "set enable_ble_module to OFF to remove ble module from gateway build (default is ON)" ON)

SET(use_condition ON CACHE BOOL "Build C shared utility with condition code" FORCE)

#making a nice global variable to know if we are on linux or not.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
  find_program( MEMORYCHECK_COMMAND valgrind )
  set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --suppressions=${CMAKE_CURRENT_LIST_DIR}/core/valgrind_suppressions.txt --error-exitcode=1" )
endif()

include (CTest)

include(CheckSymbolExists)
function(detect_architecture symbol arch) 
    if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch}) 
        unset(CMAKE_REQUIRED_QUIET) 
  
        # The output variable needs to be unique across invocations otherwise 
        # CMake's crazy scope rules will keep it defined 
        if (ARCHITECTURE_${arch}) 
            set(ARCHITECTURE "${arch}" PARENT_SCOPE) 
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE) 
            add_definitions(-DARCHITECTURE_${arch}=1) 
        endif()
    endif()
endfunction()
if (MSVC)
    detect_architecture("_M_AMD64" x86_64)
    detect_architecture("_M_IX86" x86)
    detect_architecture("_M_ARM" ARM)
else() 
    detect_architecture("__x86_64__" x86_64)
    detect_architecture("__i386__" x86)
    detect_architecture("__arm__" ARM)
endif() 
if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "") 
    set(ARCHITECTURE "GENERIC") 
endif() 
message(STATUS "AIG architecture: ${ARCHITECTURE}")

#setting #defines
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/guard:cf)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /guard:cf")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /guard:cf")
endif()

if(${LINUX})
  set (CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
endif()

function(add_module_to_solution undecoratedModuleName)
  set_target_properties(${undecoratedModuleName}
    PROPERTIES FOLDER "Gateway/Modules/${undecoratedModuleName}")
  set_target_properties(${undecoratedModuleName}_static
    PROPERTIES FOLDER "Gateway/Modules/${undecoratedModuleName}")
endfunction()

function(add_binding_to_solution bindingLanguage)
    set_target_properties(${bindingLanguage} PROPERTIES FOLDER "Gateway/Bindings")
endfunction()
    
function(add_sample_to_solution sampleName)
  set_target_properties(${sampleName} PROPERTIES FOLDER "Gateway/Samples")
endfunction()


macro(compileAsC99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC99)

macro(compileAsC11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c11 ${CMAKE_C_FLAGS}")
      set (CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
  else()
    set (CMAKE_C_STANDARD 11)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC11)

enable_testing()
compileAsC99()

#this add the deps folder that will add C shared utility and IotHubClient
add_subdirectory(./deps)

#this makes available from now on the includes of azure-c-shared-utility
include_directories(${SHARED_UTIL_INC_FOLDER})

function(linkSharedUtil whatIsBuilding)
  target_link_libraries(${whatIsBuilding} aziotsharedutil)
endfunction(linkSharedUtil)

#this adds parson
include_directories(./deps/parson)

#this adds nanomsg
option(NN_ENABLE_DOC "" OFF )
if (WIN32)
  option(NN_STATIC_LIB "" OFF )
else()
  option(NN_STATIC_LIB "" ON )
  # This should be set with the NN_STATIC_LIB options
  add_definitions (-DNN_STATIC_LIB)
endif()

option(NN_TESTS "" OFF )
option(NN_ENABLE_NANOCAT "" OFF)
set(nanomsg_target_dll ${CMAKE_CURRENT_BINARY_DIR}/deps/nanomsg/$(Configuration)/nanomsg.dll CACHE INTERNAL "The location of the nanomsg.dll (windows)" FORCE)
add_subdirectory(./deps/nanomsg)
include_directories(./deps/nanomsg/src)

set(NANOMSG_TARGETS dist )
foreach(nanomsgTarget IN LISTS NANOMSG_TARGETS)
  set_target_properties(${nanomsgTarget} PROPERTIES FOLDER "nanomsg Targets")
endforeach(nanomsgTarget)

function(link_broker whatIsBuilding)

target_link_libraries(${whatIsBuilding} nanomsg )
  
endfunction(link_broker)

function(install_broker whatIsBuilding whatIsBuildingLocation)
  if(WIN32)
  add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${nanomsg_target_dll}
          ${whatIsBuildingLocation})
  endif()
endfunction(install_broker)


if(${ARCHITECTURE} STREQUAL "x86_64")
   set(dotnet_managed_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway/bin/x64/$(Configuration)/Microsoft.Azure.IoT.Gateway.dll CACHE INTERNAL "The location of the Microsoft.Azure.IoT.Gateway.dll (windows)" FORCE)

   set(dotnet_e2etest_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/E2ETestModule/bin/x64/$(Configuration)/E2ETestModule.dll CACHE INTERNAL "The location of the E2ETestModule.dll (windows)" FORCE)

   set(dotnet_sensor_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/SensorModule/bin/x64/$(Configuration)/SensorModule.dll CACHE INTERNAL "The location of the SensorModule.dll (windows)" FORCE)

   set(dotnet_printer_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/PrinterModule/bin/x64/$(Configuration)/PrinterModule.dll CACHE INTERNAL "The location of the PrinterModule.dll (windows)" FORCE)
elseif(${ARCHITECTURE} STREQUAL "x86")
   set(dotnet_managed_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/Microsoft.Azure.IoT.Gateway/bin/x86/$(Configuration)/Microsoft.Azure.IoT.Gateway.dll CACHE INTERNAL "The location of the Microsoft.Azure.IoT.Gateway.dll (windows)" FORCE)

   set(dotnet_e2etest_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/E2ETestModule/bin/x86/$(Configuration)/E2ETestModule.dll CACHE INTERNAL "The location of the E2ETestModule.dll (windows)" FORCE)

   set(dotnet_sensor_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/SensorModule/bin/x86/$(Configuration)/SensorModule.dll CACHE INTERNAL "The location of the SensorModule.dll (windows)" FORCE)

   set(dotnet_printer_module_dll ${CMAKE_CURRENT_BINARY_DIR}/../bindings/dotnet/dotnet-binding/PrinterModule/bin/x86/$(Configuration)/PrinterModule.dll CACHE INTERNAL "The location of the PrinterModule.dll (windows)" FORCE)
endif()

set(dotnet_host_binding_dll ${CMAKE_CURRENT_BINARY_DIR}/bindings/dotnet/$(Configuration)/dotnet.dll CACHE INTERNAL "The location of dotnet.dll" FORCE)

function(install_binaries whatIsBuilding whatIsBuildingLocation whatIsBinaryLocation)
  add_custom_command(TARGET ${whatIsBuilding} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${whatIsBinaryLocation}
          ${whatIsBuildingLocation})
endfunction(install_binaries)


add_subdirectory(./core)

include_directories(${GW_INC})

add_subdirectory(modules)

add_subdirectory(bindings)

add_subdirectory(samples)
