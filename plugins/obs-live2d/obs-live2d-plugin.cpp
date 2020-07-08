/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include <obs-module.h>
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

static const char *live2d_getname(void *)
{
	return obs_module_text("Live2DModel");
}

static void *live2d_create(obs_data_t *, obs_source_t *)
{
	return malloc(sizeof(obs_live2d_data));
}

void live2d_destroy(void *data)
{
	LAppDelegate::GetInstance()->Release();
	delete reinterpret_cast<obs_live2d_data *>(data);
}

void live2d_video_render(void *data, gs_effect_t *effect) {}

static uint32_t live2d_get_width(void *data)
{
	return 0;
}

static uint32_t live2d_get_height(void *)
{
	return 0;
}

static void live2d_get_defaults(obs_data_t *) {}

obs_properties_t *live2d_get_properties(void *)
{
	return obs_properties_create();
}

void live2d_update(void *data, obs_data_t *settings) {}

namespace {
LAppAllocator s_cubismAllocator;        ///< Cubism SDK Allocator
CubismFramework::Option s_cubismOption; ///< Cubism SDK Option
struct obs_source_info s_sourceInfo;
} // namespace

bool obs_module_load()
{
	// Cubism SDK の初期化
#ifdef _DEBUG
	s_cubismOption.LogFunction = [](const csmChar *message) {
		blog(LOG_DEBUG, "%s", message);
	};
	s_cubismOption.LoggingLevel = CubismFramework::Option::LogLevel_Verbose;
#else
	s_cubismOption.LogFunction = [](const csmChar *message) {
		blog(LOG_INFO, "%s", message);
	};
	s_cubismOption.LoggingLevel = CubismFramework::Option::LogLevel_Info;
#endif
	if (!CubismFramework::StartUp(&s_cubismAllocator, &s_cubismOption)) {
		blog(LOG_ERROR, "can't start Cubism Framework");
		return false;
	}

	//Initialize cubism
	CubismFramework::Initialize();

	s_sourceInfo.id = "obs_live2d";
	s_sourceInfo.type = OBS_SOURCE_TYPE_INPUT;
	s_sourceInfo.icon_type = OBS_ICON_TYPE_CUSTOM;
	s_sourceInfo.output_flags = OBS_SOURCE_VIDEO;
	s_sourceInfo.get_name = live2d_getname;
	s_sourceInfo.create = live2d_create;
	s_sourceInfo.destroy = live2d_destroy;
	s_sourceInfo.video_render = live2d_video_render;
	s_sourceInfo.get_width = live2d_get_width;
	s_sourceInfo.get_height = live2d_get_height;
	s_sourceInfo.get_defaults = live2d_get_defaults;
	s_sourceInfo.get_properties = live2d_get_properties;
	s_sourceInfo.update = live2d_update;
	obs_register_source(&s_sourceInfo);

	return true;
}

void obs_module_unload()
{
	// リソースを解放
	LAppLive2DManager::ReleaseInstance();

	//Cubism SDK の解放
	CubismFramework::Dispose();

	LAppDelegate::ReleaseInstance();
}
