cmake_minimum_required(VERSION 3.20)
project(tkde-app VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall -Wextra -Wpedantic

)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Source groups
file(GLOB_RECURSE CORE_SOURCES   "src/core/*.cpp")
file(GLOB_RECURSE DISPLAY_SOURCES "src/display/*.cpp")
file(GLOB_RECURSE GPU_SOURCES    "src/gpu/*.cpp")
file(GLOB_RECURSE KDE_SOURCES   "src/kde/*.cpp")
file(GLOB_RECURSE TERMUX_SOURCES "src/termux/*.cpp")
file(GLOB_RECURSE APPSTORE_SOURCES "src/appstore/*.cpp")
file(GLOB_RECURSE UI_SOURCES     "src/ui/*.cpp")

# Headers
file(GLOB_RECURSE CORE_HEADERS   "include/*.hpp")

# Include directories for all targets
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(tkde_core STATIC
    ${CORE_SOURCES}
    ${GPU_SOURCES}
    ${TERMUX_SOURCES}
)

add_library(tkde_kde STATIC
    ${KDE_SOURCES}
)

add_library(tkde_display STATIC
    ${DISPLAY_SOURCES}
)

add_library(tkde_appstore STATIC
    ${APPSTORE_SOURCES}
)

add_executable(tkde-app
    src/main.cpp
    ${UI_SOURCES}
)

target_link_libraries(tkde-app PRIVATE
    tkde_core
    tkde_kde
    tkde_display
    tkde_appstore
)

# Link pthread on Linux
find_package(Threads REQUIRED)
target_link_libraries(tkde-app PRIVATE Threads::Threads)
target_link_libraries(tkde_core PRIVATE Threads::Threads)

# Detect Termux/Android
if(EXISTS "/system/build.prop")
    target_compile_definitions(tkde-core PUBLIC TERMUX_BUILD=1)
    target_compile_definitions(tkde-app PUBLIC TERMUX_BUILD=1)
endif()

# Config file generation
configure_file(config/app.conf.in config/app.conf @ONLY)

install(TARGETS tkde-app RUNTIME DESTINATION bin)
