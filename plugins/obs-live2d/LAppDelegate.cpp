/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDelegate.hpp"
#include <util/base.h>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <util/base.h>
#include <graphics/graphics.h>
#include <obs.h>
#include "LAppView.hpp"
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"

using namespace Csm;
using namespace std;
using namespace LAppDefine;

namespace {
LAppDelegate *s_instance = NULL;
}

LAppDelegate *LAppDelegate::GetInstance()
{
	if (s_instance == NULL) {
		s_instance = new LAppDelegate();
	}

	return s_instance;
}

void LAppDelegate::ReleaseInstance()
{
	if (s_instance != NULL) {
		delete s_instance;
	}

	s_instance = NULL;
}

void LAppDelegate::Release()
{
	// Windowの削除
	glfwDestroyWindow(_window);

	delete _textureManager;
	delete _view;
}

void LAppDelegate::Render()
{
	if (_isInit) {
		// Windowの生成_
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
		_window = glfwCreateWindow(RenderTargetWidth,
					   RenderTargetHeight,
					   "Live2D Model Scene", NULL, NULL);
		if (_window == NULL) {
			blog(LOG_ERROR, "Can't create GLFW window");
			glfwTerminate();
			return;
		}

		// Windowのコンテキストをカレントに設定
		glfwMakeContextCurrent(_window);

		if (glewInit() != GLEW_OK) {
			blog(LOG_ERROR, "Can't initialize GLEW");
			glfwTerminate();
			return;
		}

		//テクスチャサンプリング設定
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR);

		//透過設定
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//load model
		LAppLive2DManager::GetInstance();

		//default proj
		CubismMatrix44 projection;

		LAppPal::UpdateTime();

		_view->InitializeSprite();

		_isInit = false;
	}

	int width, height;
	glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width,
			  &height);
	if ((_windowWidth != width || _windowHeight != height) && width > 0 &&
	    height > 0) {
		//AppViewの初期化
		_view->Initialize();
		// サイズを保存しておく
		_windowWidth = width;
		_windowHeight = height;

		// ビューポート変更
		glViewport(0, 0, width, height);
	}

	// 時間更新
	LAppPal::UpdateTime();

	// 画面の初期化
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.0);

	// Draw graphic texture
	gs_texture_t *texture =
		gs_texture_create(width, height, GS_RGBA, 1, NULL, GS_DYNAMIC);

	uint8_t *buffer;
	uint32_t linesize;
	if (gs_texture_map(texture, &buffer, &linesize)) {
		//描画更新
		_view->Render();

		glReadPixels(0, 0, _windowWidth, _windowHeight, GL_RGBA,
			     GL_UNSIGNED_BYTE, buffer);
	}
	gs_texture_unmap(texture);
	obs_source_draw(texture, 0, 0, 0, 0, true);
	gs_texture_destroy(texture);
}

LAppDelegate::LAppDelegate()
	: _window(NULL),
	  _captured(false),
	  _mouseX(0.0f),
	  _mouseY(0.0f),
	  _isInit(true),
	  _windowWidth(0),
	  _windowHeight(0)
{
	_view = new LAppView();
	_textureManager = new LAppTextureManager();
}

LAppDelegate::~LAppDelegate() {}

GLuint LAppDelegate::CreateShader()
{
	//バーテックスシェーダのコンパイル
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	const char *vertexShader = "#version 120\n"
				   "attribute vec3 position;"
				   "attribute vec2 uv;"
				   "varying vec2 vuv;"
				   "void main(void){"
				   "    gl_Position = vec4(position, 1.0);"
				   "    vuv = uv;"
				   "}";
	glShaderSource(vertexShaderId, 1, &vertexShader, NULL);
	glCompileShader(vertexShaderId);
	if (!CheckShader(vertexShaderId)) {
		return 0;
	}

	//フラグメントシェーダのコンパイル
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fragmentShader =
		"#version 120\n"
		"varying vec2 vuv;"
		"uniform sampler2D texture;"
		"uniform vec4 baseColor;"
		"void main(void){"
		"    gl_FragColor = texture2D(texture, vuv) * baseColor;"
		"}";
	glShaderSource(fragmentShaderId, 1, &fragmentShader, NULL);
	glCompileShader(fragmentShaderId);
	if (!CheckShader(fragmentShaderId)) {
		return 0;
	}

	//プログラムオブジェクトの作成
	GLuint programId = glCreateProgram();
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	// リンク
	glLinkProgram(programId);

	glUseProgram(programId);

	return programId;
}

bool LAppDelegate::CheckShader(GLuint shaderId)
{
	GLint status;
	GLint logLength;
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		GLchar *log = reinterpret_cast<GLchar *>(CSM_MALLOC(logLength));
		glGetShaderInfoLog(shaderId, logLength, &logLength, log);
		CubismLogError("Shader compile log: %s", log);
		CSM_FREE(log);
	}

	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		glDeleteShader(shaderId);
		return false;
	}

	return true;
}
