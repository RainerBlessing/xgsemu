cmake_minimum_required(VERSION 3.10)
project(xgsemu VERSION 0.6.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use C++11 standard instead of C++98
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type Konfiguration (Debug/Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type: Debug or Release" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)

# Compiler flags basierend auf Build-Typ
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wno-write-strings ${WX_CFLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${WX_LIBS}")

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "C++ Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")

# Find required packages
find_package(SDL REQUIRED)
find_package(wxWidgets REQUIRED COMPONENTS core base html)

# Debug-Ausgaben
message(STATUS "SDL Include dirs: ${SDL_INCLUDE_DIRS}")
message(STATUS "SDL Libraries: ${SDL_LIBRARIES}")
message(STATUS "wxWidgets Include dirs: ${wxWidgets_INCLUDE_DIRS}")
message(STATUS "wxWidgets Libraries: ${wxWidgets_LIBRARIES}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")

# Include directories
include_directories(${SDL_INCLUDE_DIRS})
include_directories(${wxWidgets_INCLUDE_DIRS})

# Add wxWidgets definitions and flags
include(${wxWidgets_USE_FILE})

# Source files - nur aus src/ Verzeichnis
file(GLOB SOURCES
    "src/*.cpp"
)

# Header files - nur aus src/ Verzeichnis
file(GLOB HEADERS
    "src/*.h"
)

# Create executable
add_executable(xgsemu ${SOURCES} ${HEADERS})

# Link with libraries
target_link_libraries(xgsemu ${SDL_LIBRARIES} ${wxWidgets_LIBRARIES})

# On Unix-like systems (Linux, macOS), we may need to link with additional libraries
if(UNIX)
    target_link_libraries(xgsemu pthread)
    
    # Additional compiler definitions for Unix
    target_compile_definitions(xgsemu PRIVATE
        _LARGE_FILES
        _FILE_OFFSET_BITS=64
        __WXGTK__
    )
endif()

# Define preprocessor macros
target_compile_definitions(xgsemu PRIVATE 
    XGSE_VERSION="0.07"
)

# Füge NDEBUG nur für Release-Builds hinzu
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(xgsemu PRIVATE NDEBUG)
endif()

target_compile_definitions(xgsemu PRIVATE _THREAD_SAFE)

# Install the executable
install(TARGETS xgsemu DESTINATION bin)

# Install README.htm if it exists
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.htm 
        DESTINATION share/xgsemu
        OPTIONAL)

# Git Archive Funktionalität
find_package(Git)
if(GIT_FOUND)
    # Git Version Information
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Git Branch Information
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Add version info to build
    target_compile_definitions(xgsemu PRIVATE 
        GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
        GIT_BRANCH="${GIT_BRANCH}"
    )
    
    # Git Archive Target for 'main' branch
    add_custom_target(git-archive
        COMMAND ${CMAKE_COMMAND} -E echo "Creating archive from main branch..."
        COMMAND ${GIT_EXECUTABLE} archive --format=zip --output=${CMAKE_BINARY_DIR}/xgsemu-${PROJECT_VERSION}.zip main
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Creating ZIP archive from main branch"
    )
    
    # Optional: Git Archive mit Tag-Information
    add_custom_target(git-archive-tagged
        COMMAND ${CMAKE_COMMAND} -E echo "Creating archive with current tag/version..."
        COMMAND ${GIT_EXECUTABLE} archive --format=zip --prefix=xgsemu-${PROJECT_VERSION}/ --output=${CMAKE_BINARY_DIR}/xgsemu-${PROJECT_VERSION}.zip HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Creating ZIP archive with version prefix"
    )
endif()

# Option für Unit Tests
option(BUILD_TESTS "Build the unit tests" ON)

if(BUILD_TESTS)
    # Sammle alle Testquellen ein
    file(GLOB TEST_SOURCES "tests/*.cpp")

    # Prüfe, ob wir tatsächlich etwas gefunden haben
    if(TEST_SOURCES)
        # GoogleTest via FetchContent laden, wie gehabt
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        # Bibliothek aus deinem Projekt
        add_library(xgsemu_lib STATIC ${SOURCES})
        target_link_libraries(xgsemu_lib ${SDL_LIBRARIES} ${wxWidgets_LIBRARIES})
        if(UNIX)
          target_link_libraries(xgsemu_lib pthread)
        endif()

        # Test-Executable erstellen
        add_executable(unit_tests ${TEST_SOURCES})
        target_link_libraries(unit_tests gtest gtest_main xgsemu_lib)

        # Include‐Pfad anpassen
        target_include_directories(unit_tests PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${SDL_INCLUDE_DIRS}
            ${wxWidgets_INCLUDE_DIRS}
        )

        # GTest-Integration
        include(GoogleTest)
        gtest_discover_tests(unit_tests)

        # Optional bequemes Test-Target
        add_custom_target(test
            COMMAND unit_tests
            DEPENDS unit_tests
        )
    else()
        message(WARNING "BUILD_TESTS ist eingeschaltet, aber es existieren keine Test‐Dateien in tests/*.cpp.")
    endif()
endif()

# Find clang-format
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h)

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source files with clang-format"
    )
else()
    message(WARNING "clang-format not found. Code formatting will not be available.")
endif()
