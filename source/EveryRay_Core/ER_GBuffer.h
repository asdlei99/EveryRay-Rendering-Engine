#pragma once

#include "Common.h"
#include "ER_CoreComponent.h"
#include "RHI/ER_RHI.h"

namespace EveryRay_Core
{
	class ER_Scene;
	class ER_Camera;

	enum GBufferDebugMode
	{
		GBUFFER_DEBUG_NONE = 0,
		GBUFFER_DEBUG_ALBEDO,
		GBUFFER_DEBUG_NORMALS,
		GBUFFER_DEBUG_ROUGHNESS,
		GBUFFER_DEBUG_METALNESS,

		GBUFFER_DEBUG_COUNT
	};

	class ER_GBuffer: public ER_CoreComponent
	{
	public:
		ER_GBuffer(ER_Core& game, ER_Camera& camera, int width, int height);
		~ER_GBuffer();
	
		void Initialize();
		void Update(const ER_CoreTime& time);

		void Start();
		void End();
		void Draw(const ER_Scene* scene);
		void Config() { mShowDebug = !mShowDebug; }
		bool IsEnabled() { return mIsEnabled; }

		ER_RHI_GPUTexture* GetAlbedo() { return mAlbedoBuffer; }
		ER_RHI_GPUTexture* GetNormals() { return mNormalBuffer; }
		ER_RHI_GPUTexture* GetPositions() { return mPositionsBuffer; }
		ER_RHI_GPUTexture* GetExtraBuffer() { return mExtraBuffer; } // [4 channels: extra mask value, roughness, metalness, height mask value]
		ER_RHI_GPUTexture* GetExtra2Buffer() { return mExtra2Buffer; } // [1 channel: "RenderingObjectFlags" bitmasks]
		ER_RHI_GPUTexture* GetDepth() { return mDepthBuffer; }

		GBufferDebugMode GetCurrentDebugMode() { return mCurrentDebugMode; }

	private:
		void UpdateImGui();

		ER_RHI_GPURootSignature* mRootSignature = nullptr;

		ER_RHI_GPUTexture* mDepthBuffer = nullptr;
		ER_RHI_GPUTexture* mAlbedoBuffer= nullptr;
		ER_RHI_GPUTexture* mNormalBuffer= nullptr;
		ER_RHI_GPUTexture* mPositionsBuffer = nullptr;
		ER_RHI_GPUTexture* mExtraBuffer = nullptr;
		ER_RHI_GPUTexture* mExtra2Buffer = nullptr;

		int mWidth;
		int mHeight;
		bool mIsEnabled = true;
		bool mShowDebug = false;

		GBufferDebugMode mCurrentDebugMode = GBUFFER_DEBUG_NONE;
	};
}