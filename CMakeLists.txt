cmake_minimum_required (VERSION 3.13)

project(DMCAnalyzer)

add_definitions( -DLOGIC2 )

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/DMCAnalyzer.cpp
src/DMCAnalyzer.h
src/DMCAnalyzerResults.cpp
src/DMCAnalyzerResults.h
src/DMCAnalyzerSettings.cpp
src/DMCAnalyzerSettings.h
src/DMCSimulationDataGenerator.cpp
src/DMCSimulationDataGenerator.h
src/DMCProtocol.cpp
src/DMCProtocol.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

enable_testing()
add_executable(DMCProtocolTests tests/DMCProtocolTests.cpp src/DMCProtocol.cpp)
target_include_directories(DMCProtocolTests PRIVATE src ${analyzersdk_SOURCE_DIR}/include)
add_test(NAME DMCProtocolTests COMMAND DMCProtocolTests)
