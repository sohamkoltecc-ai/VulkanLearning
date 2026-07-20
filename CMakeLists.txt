# CMakeList.txt : CMake project for VulkanLearning, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# project name
project ("VulkanLearning")

# cpp version
set(CMAKE_CXX_STANDARD 20)

# search for vulkan in drive and get it listed here.
find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw-3.4)



# Add source to this project's executable.
add_executable (VulkanLearning  "main.cpp")

# Include directories
target_include_directories(
    VulkanLearning
    PRIVATE
        external/glm-1.0.1
)

add_custom_command(TARGET VulkanLearning POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:VulkanLearning>/shaders
)

# set a target link with vulkan
target_link_libraries(VulkanLearning
    PRIVATE
        Vulkan::Vulkan
        glfw
)


# default stuff
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VulkanLearning PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
