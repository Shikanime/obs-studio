/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppPal.hpp"
#include <cstdio>
#include <stdarg.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <util/base.h>
#include <GLFW/glfw3.h>
#include <util/base.h>
#include <Model/CubismMoc.hpp>
#include "LAppDefine.hpp"

using std::endl;
using namespace Csm;
using namespace std;
using namespace LAppDefine;

double LAppPal::s_currentFrame = 0.0;
double LAppPal::s_lastFrame = 0.0;
double LAppPal::s_deltaTime = 0.0;

csmByte *LAppPal::LoadFileAsBytes(const string filePath, csmSizeInt *outSize)
{
	//filePath;//
	const char *path = filePath.c_str();

	int size = 0;
	struct stat statBuf;
	if (stat(path, &statBuf) == 0) {
		size = statBuf.st_size;
	}

	std::fstream file;
	char *buf = new char[size];

	file.open(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		blog(LOG_WARNING, "file open error");
		return NULL;
	}
	file.read(buf, size);
	file.close();

	*outSize = size;
	return reinterpret_cast<csmByte *>(buf);
}

void LAppPal::ReleaseBytes(csmByte *byteData)
{
	delete[] byteData;
}

csmFloat32 LAppPal::GetDeltaTime()
{
	return static_cast<csmFloat32>(s_deltaTime);
}

void LAppPal::UpdateTime()
{
	s_currentFrame = glfwGetTime();
	s_deltaTime = s_currentFrame - s_lastFrame;
	s_lastFrame = s_currentFrame;
}

void LAppPal::PrintMessage(const csmChar *message)
{
	blog(LOG_INFO, "%s", message);
}
