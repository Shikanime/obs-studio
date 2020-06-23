/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppView.hpp"
#include <obs-module.h>
#include <math.h>
#include <string>
#include "LAppPal.hpp"
#include "LAppDelegate.hpp"
#include "LAppLive2DManager.hpp"
#include "LAppTextureManager.hpp"
#include "LAppDefine.hpp"
#include "LAppSprite.hpp"
#include "LAppModel.hpp"

using namespace std;
using namespace LAppDefine;

LAppView::LAppView()
	: _programId(0), _renderSprite(NULL), _renderTarget(SelectTarget_None)
{
	_clearColor[0] = 1.0f;
	_clearColor[1] = 1.0f;
	_clearColor[2] = 1.0f;
	_clearColor[3] = 0.0f;

	// デバイス座標からスクリーン座標に変換するための
	_deviceToScreen = new CubismMatrix44();

	// 画面の表示の拡大縮小や移動の変換を行う行列
	_viewMatrix = new CubismViewMatrix();
}

LAppView::~LAppView()
{
	_renderBuffer.DestroyOffscreenFrame();
	delete _renderSprite;
	delete _viewMatrix;
	delete _deviceToScreen;
}

void LAppView::Initialize()
{
	int width, height;
	glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width,
			  &height);

	if (width == 0 || height == 0) {
		return;
	}

	float ratio = static_cast<float>(height) / static_cast<float>(width);
	float left = ViewLogicalLeft;
	float right = ViewLogicalRight;
	float bottom = -ratio;
	float top = ratio;

	_viewMatrix->SetScreenRect(
		left, right, bottom,
		top); // デバイスに対応する画面の範囲。 Xの左端, Xの右端, Yの下端, Yの上端

	float screenW = fabsf(left - right);
	_deviceToScreen->LoadIdentity(); // サイズが変わった際などリセット必須
	_deviceToScreen->ScaleRelative(screenW / width, -screenW / width);
	_deviceToScreen->TranslateRelative(-width * 0.5f, -height * 0.5f);

	// 表示範囲の設定
	_viewMatrix->SetMaxScale(ViewMaxScale); // 限界拡大率
	_viewMatrix->SetMinScale(ViewMinScale); // 限界縮小率

	// 表示できる最大範囲
	_viewMatrix->SetMaxScreenRect(ViewLogicalMaxLeft, ViewLogicalMaxRight,
				      ViewLogicalMaxBottom, ViewLogicalMaxTop);
}

void LAppView::Render()
{
	LAppLive2DManager *Live2DManager = LAppLive2DManager::GetInstance();

	// Cubism更新・描画
	Live2DManager->OnUpdate();

	// 各モデルが持つ描画ターゲットをテクスチャとする場合
	if (_renderTarget == SelectTarget_ModelFrameBuffer && _renderSprite) {
		const GLfloat uvVertex[] = {
			1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		};

		for (csmUint32 i = 0; i < Live2DManager->GetModelNum(); i++) {
			float alpha = GetSpriteAlpha(
				i); // サンプルとしてαに適当な差をつける
			_renderSprite->SetColor(1.0f, 1.0f, 1.0f, alpha);

			LAppModel *model = Live2DManager->GetModel(i);
			if (model) {
				_renderSprite->RenderImmidiate(
					model->GetRenderBuffer()
						.GetColorBuffer(),
					uvVertex);
			}
		}
	}
}

void LAppView::InitializeSprite()
{
	_programId = LAppDelegate::GetInstance()->CreateShader();

	int width, height;
	glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width,
			  &height);

	LAppTextureManager *textureManager =
		LAppDelegate::GetInstance()->GetTextureManager();
	const string resourcesPath =
		obs_module_file("resources") + std::string("/");

	string imageName = BackImageName;
	LAppTextureManager::TextureInfo *backgroundTexture =
		textureManager->CreateTextureFromPngFile(resourcesPath +
							 imageName);

	// 画面全体を覆うサイズ
	float x = width * 0.5f;
	float y = height * 0.5f;
	_renderSprite = new LAppSprite(x, y, static_cast<float>(width),
				       static_cast<float>(height), 0,
				       _programId);
}

float LAppView::TransformViewX(float deviceX) const
{
	float screenX = _deviceToScreen->TransformX(
		deviceX); // 論理座標変換した座標を取得。
	return _viewMatrix->InvertTransformX(
		screenX); // 拡大、縮小、移動後の値。
}

float LAppView::TransformViewY(float deviceY) const
{
	float screenY = _deviceToScreen->TransformY(
		deviceY); // 論理座標変換した座標を取得。
	return _viewMatrix->InvertTransformY(
		screenY); // 拡大、縮小、移動後の値。
}

float LAppView::TransformScreenX(float deviceX) const
{
	return _deviceToScreen->TransformX(deviceX);
}

float LAppView::TransformScreenY(float deviceY) const
{
	return _deviceToScreen->TransformY(deviceY);
}

void LAppView::PreModelDraw(LAppModel &refModel)
{
	// 別のレンダリングターゲットへ向けて描画する場合の使用するフレームバッファ
	Csm::Rendering::CubismOffscreenFrame_OpenGLES2 *useTarget = NULL;

	if (_renderTarget !=
	    SelectTarget_None) { // 別のレンダリングターゲットへ向けて描画する場合

		// 使用するターゲット
		useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer)
				    ? &_renderBuffer
				    : &refModel.GetRenderBuffer();

		if (!useTarget->IsValid()) { // 描画ターゲット内部未作成の場合はここで作成
			int width, height;
			glfwGetWindowSize(
				LAppDelegate::GetInstance()->GetWindow(),
				&width, &height);
			if (width != 0 && height != 0) {
				// モデル描画キャンバス
				useTarget->CreateOffscreenFrame(
					static_cast<csmUint32>(width),
					static_cast<csmUint32>(height));
			}
		}

		// レンダリング開始
		useTarget->BeginDraw();
		useTarget->Clear(_clearColor[0], _clearColor[1], _clearColor[2],
				 _clearColor[3]); // 背景クリアカラー
	}
}

void LAppView::PostModelDraw(LAppModel &refModel)
{
	// 別のレンダリングターゲットへ向けて描画する場合の使用するフレームバッファ
	Csm::Rendering::CubismOffscreenFrame_OpenGLES2 *useTarget = NULL;

	if (_renderTarget !=
	    SelectTarget_None) { // 別のレンダリングターゲットへ向けて描画する場合

		// 使用するターゲット
		useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer)
				    ? &_renderBuffer
				    : &refModel.GetRenderBuffer();

		// レンダリング終了
		useTarget->EndDraw();

		// LAppViewの持つフレームバッファを使うなら、スプライトへの描画はここ
		if (_renderTarget == SelectTarget_ViewFrameBuffer &&
		    _renderSprite) {
			const GLfloat uvVertex[] = {
				1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			};

			_renderSprite->SetColor(1.0f, 1.0f, 1.0f,
						GetSpriteAlpha(0));
			_renderSprite->RenderImmidiate(
				useTarget->GetColorBuffer(), uvVertex);
		}
	}
}

void LAppView::SwitchRenderingTarget(SelectTarget targetType)
{
	_renderTarget = targetType;
}

void LAppView::SetRenderTargetClearColor(float r, float g, float b)
{
	_clearColor[0] = r;
	_clearColor[1] = g;
	_clearColor[2] = b;
}

float LAppView::GetSpriteAlpha(int assign) const
{
	// assignの数値に応じて適当に決定
	float alpha = 0.25f + static_cast<float>(assign) *
				      0.5f; // サンプルとしてαに適当な差をつける
	if (alpha > 1.0f) {
		alpha = 1.0f;
	}
	if (alpha < 0.1f) {
		alpha = 0.1f;
	}

	return alpha;
}
