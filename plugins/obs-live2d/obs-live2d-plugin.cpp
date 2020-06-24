/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include <obs-module.h>
#include <GL/glew.h>
#include "LAppDelegate.hpp"
#include "LAppPal.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppDefine.hpp"

using namespace Csm;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-live2d", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Cubism Live2D model source";
}

struct obs_live2d_data {
};

static const char *live2d_getname([[maybe_unused]] void *type_data)
{
	return obs_module_text("Live2DModel");
}

static void *live2d_create([[maybe_unused]] obs_data_t *settings,
			   [[maybe_unused]] obs_source_t *source)
{
	return malloc(sizeof(obs_live2d_data));
}

void live2d_destroy([[maybe_unused]] void *data)
{
	LAppDelegate::GetInstance()->Release();
	delete reinterpret_cast<obs_live2d_data *>(data);
}

void live2d_video_render(void *data, gs_effect_t *effect)
{
	LAppDelegate::GetInstance()->Render();
}

static uint32_t live2d_get_width([[maybe_unused]] void *data)
{
	return 1920;
}

static uint32_t live2d_get_height([[maybe_unused]] void *data)
{
	return 1080;
}

static void live2d_get_defaults([[maybe_unused]] obs_data_t *settings) {}

obs_properties_t *live2d_get_properties([[maybe_unused]] void *data)
{
	return obs_properties_create();
}

void live2d_update(void *data, obs_data_t *settings) {}

namespace {
LAppAllocator cubismAllocator;        ///< Cubism SDK Allocator
CubismFramework::Option cubismOption; ///< Cubism SDK Option
} // namespace

bool obs_module_load()
{
	// GLFWの初期化
	if (glfwInit() == GL_FALSE) {
		blog(LOG_ERROR, "Can't initialize GLFW");
		return false;
	}

	// Cubism SDK の初期化
	cubismOption.LogFunction = LAppPal::PrintMessage;
	cubismOption.LoggingLevel = LAppDefine::CubismLoggingLevel;
	if (!CubismFramework::StartUp(&cubismAllocator, &cubismOption)) {
		blog(LOG_ERROR, "Can't start Cubism Framework");
		return false;
	}

	//Initialize cubism
	CubismFramework::Initialize();

	struct obs_source_info info = {};
	info.id = "obs_live2d";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.icon_type = OBS_ICON_TYPE_CUSTOM;
	info.output_flags = OBS_SOURCE_VIDEO;
	info.get_name = live2d_getname;
	info.create = live2d_create;
	info.destroy = live2d_destroy;
	info.video_render = live2d_video_render;
	info.get_width = live2d_get_width;
	info.get_height = live2d_get_height;
	info.get_defaults = live2d_get_defaults;
	info.get_properties = live2d_get_properties;
	info.update = live2d_update;
	obs_register_source(&info);

	return true;
}

void obs_module_unload()
{
	glfwTerminate();

	// リソースを解放
	LAppLive2DManager::ReleaseInstance();

	//Cubism SDK の解放
	CubismFramework::Dispose();

	LAppDelegate::ReleaseInstance();
}
