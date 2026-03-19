# CMake entry point
cmake_minimum_required (VERSION 3.0)
project (Tutorials)

find_package(OpenGL REQUIRED)


if( CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR )
    message( FATAL_ERROR "Please select another Build Directory ! (and give it a clever name, like bin_Visual2012_64bits/)" )
endif()
if( CMAKE_SOURCE_DIR MATCHES " " )
	message( "Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()
if( CMAKE_BINARY_DIR MATCHES " " )
	message( "Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()



# Compile external dependencies 
add_subdirectory (external)

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if(INCLUDE_DISTRIB)
	add_subdirectory(distrib)
endif(INCLUDE_DISTRIB)



include_directories(
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/stb_image/
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

option(ENABLE_LSAN "Enable LeakSanitizer (LSan) for TP1" OFF)

# TP1
add_executable(TP1
        TP1/TP1.cpp
		common/gameobject/Transform.cpp
		common/physics/PhysicEngine.cpp
		common/physics/RigidBody.cpp
		common/stb_image_impl.cpp
        TP1/vertex_shader.glsl
        TP1/fragment_shader.glsl
)

if(ENABLE_LSAN)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		target_compile_options(TP1 PRIVATE -fsanitize=leak -fno-omit-frame-pointer -g)
		target_link_options(TP1 PRIVATE -fsanitize=leak)
	else()
		message(WARNING "ENABLE_LSAN is only supported with Clang/GCC")
	endif()
endif()
target_link_libraries(TP1
	${ALL_LIBS}
)
# Xcode and Visual working directories
set_target_properties(TP1 PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/TP1/")
create_target_launcher(TP1 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/TP1/")




SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*" )
SOURCE_GROUP(shaders REGULAR_EXPRESSION ".*/.*shader$" )


if (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )
add_custom_command(
   TARGET TP1 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/TP1${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/TP1/"
)

add_custom_command(
	TARGET TP1 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/TP1/img" "${CMAKE_CURRENT_BINARY_DIR}/img"
)

add_custom_command(
	TARGET TP1 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/TP1/vertex_shader.glsl" "${CMAKE_CURRENT_BINARY_DIR}/vertex_shader.glsl"
)

add_custom_command(
	TARGET TP1 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/TP1/fragment_shader.glsl" "${CMAKE_CURRENT_BINARY_DIR}/fragment_shader.glsl"
)

add_custom_command(
	TARGET TP1 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/TP1/built-in_shaders" "${CMAKE_CURRENT_BINARY_DIR}/built-in_shaders"
)

add_custom_command(
	TARGET TP1 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/TP1/built-in_mesh" "${CMAKE_CURRENT_BINARY_DIR}/built-in_mesh"
)


elseif (${CMAKE_GENERATOR} MATCHES "Xcode" )

endif (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )

