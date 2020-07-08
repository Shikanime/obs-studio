option(CORE_CRL_MD
	"Use Cubism Core that is multithread-specific and DLL-specific version"
	OFF)

# Set project.
project(obs-live2d)

# Set directory paths.
if(DEFINED CSMDIR)
	set(CubismCore_Dir ${CSMDIR}/Core)
	set(CubismFramework_Dir ${CSMDIR}/Framework)
elseif(NOT DEFINED CubismCore_Dir OR NOT DEFINED CubismFramework_Dir)
	message(FATAL_ERROR "Missing Cubism SDK")
endif()

if(NOT DEFINED DXTKDIR)
	message(FATAL_ERROR "Missing DirectX Toolkit")
endif()

# Detect architecture.
if(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64 /OPT:REF")
	set(ARCH x86_64)
	set(DX_ARCH x64)
elseif(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86 /OPT:REF")
	set(ARCH x86)
	set(DX_ARCH Win32)
else()
	message(FATAL_ERROR "Invalid linker flag ${CMAKE_EXE_LINKER_FLAGS}.")
endif()
# Detect compiler.
if(MSVC_VERSION MATCHES 1800)
	# Visual Studio 2013
	set(COMPILER 120)
	set(MSVC_YEAR 2013)
elseif(MSVC_VERSION MATCHES 1900)
	# Visual Studio 2015
	set(COMPILER 140)
	set(MSVC_YEAR 2015)
elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS 1920)
	# Visual Studio 2017
	set(COMPILER 141)
	set(MSVC_YEAR 2017)
elseif(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS 1930)
	# Visual Studio 2019
	set(COMPILER 142)
	set(MSVC_YEAR 2019)
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

# Specify Cubism Framework rendering.
set(FRAMEWORK_SOURCE D3D11)
# Add Cubism Native Framework.
add_subdirectory(${CubismFramework_Dir} ${CMAKE_CURRENT_BINARY_DIR}/Framework)
# Add include path to framework.
target_include_directories(Framework PUBLIC ${DXTKDIR}/Inc)
# Link libraries to framework.
target_link_libraries(Framework
	Live2DCubismCore
	${DXTKDIR}/Bin/Desktop_${MSVC_YEAR}/${DX_ARCH}/$<CONFIG>/DirectXTK.lib
	d3d11.lib
	d3dcompiler.lib
)

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
	obs-live2d-plugin.cpp)

set(MODULE_DESCRIPTION "OBS Live2D module")
configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in obs-live2d.rc)
list(APPEND obs-live2d_SOURCES obs-live2d.rc)

# Make executable app.
add_library(obs-live2d MODULE ${obs-live2d_SOURCES})
# Link libraries to app.
target_link_libraries(obs-live2d
	Framework
	# Solve the MSVCRT confliction.
	debug -NODEFAULTLIB:libcmtd.lib
	optimized -NODEFAULTLIB:libcmt.lib
	libobs
	${obs-live2d_PLATFORM_DEPS})

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
