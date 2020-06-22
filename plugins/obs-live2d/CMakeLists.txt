option(CORE_CRL_MD
	"Use Cubism Core that is multithread-specific and DLL-specific version"
	OFF)

# Set project.
project(obs-live2d)

# Set directory paths.
if(DEFINED CSMDIR)
	set(CubismCore_Dir ${CSMDIR}/Core)
	set(CubismFramework_Dir ${CSMDIR}/Framework)
endif()

if(NOT DEFINED STBDIR)
	message(FATAL_ERROR "Missing STB library")
endif()

if(NOT DEFINED GLEWDIR)
	message(FATAL_ERROR "Missing GLEW library")
endif()

if(NOT DEFINED GLFWDIR)
	message(FATAL_ERROR "Missing GLFWDIR library")
endif()

# Suppress generation of ZERO_CHECK project.
set(CMAKE_SUPPRESS_REGENERATION ON)

# Surpress GLEW and GLFW process.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_UTILS OFF CACHE BOOL "" FORCE)

# Detect architecture.
if(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64 /OPT:REF")
	set(ARCH x86_64)
elseif(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86 /OPT:REF")
	set(ARCH x86)
else()
	message(FATAL_ERROR "Invalid linker flag ${CMAKE_EXE_LINKER_FLAGS}.")
endif()
# Detect compiler.
if(MSVC_VERSION MATCHES 1800)
	# Visual Studio 2013
	set(COMPILER 120)
elseif(MSVC_VERSION MATCHES 1900)
	# Visual Studio 2015
	set(COMPILER 140)
elseif(MSVC_VERSION GREATER_EQUAL 1910)
	# Visual Studio 2017
	set(COMPILER 141)
elseif(MSVC)
	message(FATAL_ERROR "Unsupported Visual C++ compiler used.")
else()
	message(FATAL_ERROR "Unsupported compiler used.")
endif()
# Detect core crt.
if(CORE_CRL_MD)
	set(CRT MD)
else()
	set(CRT MT)
endif()

# Add Cubism Core.
# Import as static library.
add_library(Live2DCubismCore STATIC IMPORTED)
# Find library path.
set(CORE_LIB_SUFFIX ${CubismCore_Dir}/lib/windows/${ARCH}/${COMPILER})
set_target_properties(Live2DCubismCore
	PROPERTIES
		IMPORTED_LOCATION_DEBUG
			${CORE_LIB_SUFFIX}/Live2DCubismCore_${CRT}d.lib
		IMPORTED_LOCATION_RELEASE
			${CORE_LIB_SUFFIX}/Live2DCubismCore_${CRT}.lib
		INTERFACE_INCLUDE_DIRECTORIES ${CubismCore_Dir}/include)

# Add GLEW ,GLFW.
add_subdirectory(${GLEWDIR}/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/glew)
add_subdirectory(${GLFWDIR} ${CMAKE_CURRENT_BINARY_DIR}/glfw)

# Specify Cubism Framework rendering.
set(FRAMEWORK_SOURCE OpenGL)
# Add Cubism Native Framework.
add_subdirectory(${CubismFramework_Dir} ${CMAKE_CURRENT_BINARY_DIR}/Framework)
# Add rendering definition to framework.
target_compile_definitions(Framework PUBLIC CSM_TARGET_WIN_GL)
# Add include path of GLEW to framework.
target_include_directories(Framework PUBLIC ${GLEWDIR}/include)
# Link libraries to framework.
target_link_libraries(Framework Live2DCubismCore glew_s)

# Find opengl libraries.
find_package(OpenGL REQUIRED)

set(obs-live2d_PLATFORM_DEPS)
set(obs-live2d_SOURCES
	LAppAllocator.cpp
	LAppAllocator.hpp
	LAppDefine.cpp
	LAppDefine.hpp
	LAppDelegate.cpp
	LAppDelegate.hpp
	LAppLive2DManager.cpp
	LAppLive2DManager.hpp
	LAppModel.cpp
	LAppModel.hpp
	LAppPal.cpp
	LAppPal.hpp
	LAppSprite.cpp
	LAppSprite.hpp
	LAppTextureManager.cpp
	LAppTextureManager.hpp
	LAppView.cpp
	LAppView.hpp
	obs-live2d-plugin.cpp
	TouchManager.cpp
	TouchManager.hpp)

if(WIN32)
	set(MODULE_DESCRIPTION "OBS Live2D module")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in obs-live2d.rc)
	list(APPEND obs-live2d_SOURCES obs-live2d.rc)
endif()

# Make executable app.
add_library(obs-live2d MODULE ${obs-live2d_SOURCES})
# Link libraries to app.
target_link_libraries(obs-live2d
	Framework
	glfw
	${OPENGL_LIBRARIES}
	# Solve the MSVCRT confliction.
	debug -NODEFAULTLIB:libcmtd.lib
	optimized -NODEFAULTLIB:libcmt.lib
	libobs
	${obs-live2d_PLATFORM_DEPS})
# Specify include directories.
target_include_directories(obs-live2d PRIVATE ${STBDIR})

# Set project properties.
set_target_properties(obs-live2d PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(obs-live2d data)

# You can change target that renderer draws by enabling following definition.
#
# * USE_RENDER_TARGET
#	 Renderer draws to target of LAppView.
# * USE_MODEL_RENDER_TARGET
#	 Renderer draws to target of each LAppModel.
# * default
#	 Renderer draws to default main framebuffer.
#
# INFO: USE_RENDER_TARGET has higher priority than USE_MODEL_RENDER_TARGET.
#
# target_compile_definitions(obs-live2d
#	 PRIVATE
#		 USE_RENDER_TARGET
#		 USE_MODEL_RENDER_TARGET)
