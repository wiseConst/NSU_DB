
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME db_runner)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SRC_FILES "${SOURCE_DIR}/*.c??" "${SOURCE_DIR}/*.h??") 
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})
target_precompile_headers(${PROJECT_NAME} PRIVATE ${SOURCE_DIR}/pch.hpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        -DVK_NO_PROTOTYPES
        -DGLFW_INCLUDE_NONE
        -DIMGUI_IMPL_VULKAN_USE_VOLK
)

# Automatically group all sources into folders for MVS.
foreach (FILE ${SRC_FILES})
    file(RELATIVE_PATH REL_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    get_filename_component(DIR "${REL_FILE}" DIRECTORY)
    string(REPLACE "/" "\\" GROUP "${DIR}")

    source_group("${GROUP}" FILES ${FILE})
endforeach()

# TODO: refactor CMakeLists.txt

# ============= ThirdParty =============

set(PROJECT_DEPS)

# pgfe - postgres frontend, feature rich c++ drv
FetchContent_Declare(
	dmitigr_pgfe
    GIT_REPOSITORY https://github.com/dmitigr/pgfe.git
    GIT_TAG main
    GIT_PROGRESS true
	GIT_SHALLOW true
)
list(APPEND PROJECT_DEPS dmitigr_pgfe)

# Vulkan SDK
FetchContent_Declare(
	vulkan
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
    GIT_TAG main
    GIT_PROGRESS true
	GIT_SHALLOW true
)
list(APPEND PROJECT_DEPS vulkan)

# glfw - window management lib
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
    GIT_SHALLOW true
    GIT_PROGRESS true
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

list(APPEND PROJECT_DEPS glfw)

# spdlog
message(STATUS "Fetching spdlog...")
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.x
        GIT_SHALLOW true
)

set(SPDLOG_ENABLE_PCH ON CACHE BOOL "" FORCE)

list(APPEND PROJECT_DEPS spdlog)

# volk
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
    GIT_SHALLOW true
    GIT_PROGRESS true
)
list(APPEND PROJECT_DEPS volk)

# imgui - immediate mode UI
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW true
    GIT_PROGRESS true
)
list(APPEND PROJECT_DEPS imgui)

if(PROJECT_DEPS)
    message("${PROJECT_NAME}: downloading dependencies:")
    message(STATUS "${PROJECT_DEPS} ...")
    FetchContent_MakeAvailable(${PROJECT_DEPS})

    foreach(dep IN LISTS PROJECT_DEPS)
        if(TARGET ${dep})
            message(STATUS "Linking dependency target: ${dep}")
            target_link_libraries(${PROJECT_NAME} PRIVATE ${dep})
        else()
            message(WARNING "Dependency target '${dep}' not found - skipping link.")
        endif()

        target_include_directories(${PROJECT_NAME} PRIVATE "${${dep}_SOURCE_DIR}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${${dep}_SOURCE_DIR}/src")
        target_include_directories(${PROJECT_NAME} PRIVATE "${${dep}_SOURCE_DIR}/backends")
        target_include_directories(${PROJECT_NAME} PRIVATE "${${dep}_SOURCE_DIR}/include")
    endforeach()

endif()

# postgres
#find_package(PostgreSQL REQUIRED)
# target_link_libraries(${PROJECT_NAME} PUBLIC ${PostgreSQL_LIBRARIES})
#target_include_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_INCLUDE_DIRS})

# bake imgui into application instead of static lib
file(GLOB IMGUI_SOURCES "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp" "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp" ${imgui_SOURCE_DIR}/*.c?? ${imgui_SOURCE_DIR}/*.h??)
target_include_directories(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR} ${glfw_SOURCE_DIR}/include ${volk_SOURCE_DIR} ${vulkan_SOURCE_DIR})
target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES})
