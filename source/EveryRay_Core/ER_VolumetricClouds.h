#pragma once
#include "Common.h"
#include "ER_CoreComponent.h"
#include "RHI/ER_RHI.h"

namespace EveryRay_Core
{
	class ER_DirectionalLight;
	class ER_CoreTime;
	class ER_Camera;
	class ER_Skybox;

	enum VolumetricCloudsQuality
	{
		VC_DISABLED = 0,
		VC_LOW,
		VC_MEDIUM,
		VC_HIGH
	};

	namespace VolumetricCloudsCBufferData {
		struct ER_ALIGN_GPU_BUFFER FrameCB
		{
			XMMATRIX	InvProj;
			XMMATRIX	InvView;
			XMVECTOR	LightDir;
			XMVECTOR	LightCol;
			XMVECTOR	CameraPos;
			XMFLOAT2	UpsampleRatio;
		};

		struct ER_ALIGN_GPU_BUFFER CloudsCB
		{
			XMVECTOR AmbientColor;
			XMVECTOR WindDir;
			float WindSpeed;
			float Time;
			float Crispiness;
			float Curliness;
			float Coverage;
			float Absorption;
			float CloudsBottomHeight;
			float CloudsTopHeight;
			float DensityFactor;
			float DistanceToFadeFrom; // when to start fading
			float DistanceOfFade; // for how long to fade
		};

		struct ER_ALIGN_GPU_BUFFER UpsampleBlurCB
		{
			bool Upsample;
		};
	}

	class ER_VolumetricClouds : public ER_CoreComponent
	{
	public:
		ER_VolumetricClouds(ER_Core& game, ER_Camera& camera, ER_DirectionalLight& light, ER_Skybox& skybox, VolumetricCloudsQuality aQuality);
		~ER_VolumetricClouds();

		void Initialize(ER_RHI_GPUTexture* aIlluminationDepth);

		void Draw(const ER_CoreTime& gametime);
		void Update(const ER_CoreTime& gameTime);
		void Config() { mShowDebug = !mShowDebug; }
		void Composite(ER_RHI_GPUTexture* aRenderTarget);
		bool IsEnabled() { return mEnabled && (mCurrentQuality != VolumetricCloudsQuality::VC_DISABLED); }
		void SetDownscaleFactor(float val) { mDownscaleFactor = val; }
	private:
		void UpdateImGui();

		ER_Camera& mCamera;
		ER_DirectionalLight& mDirectionalLight;
		ER_Skybox& mSkybox;
		
		ER_RHI_GPUConstantBuffer<VolumetricCloudsCBufferData::FrameCB> mFrameConstantBuffer;
		ER_RHI_GPUConstantBuffer<VolumetricCloudsCBufferData::CloudsCB> mCloudsConstantBuffer;
		ER_RHI_GPUConstantBuffer<VolumetricCloudsCBufferData::UpsampleBlurCB> mUpsampleBlurConstantBuffer;

		ER_RHI_GPUTexture* mIlluminationResultDepthTarget = nullptr; // not allocated here, just a pointer
		ER_RHI_GPUTexture* mSkyRT = nullptr;
		ER_RHI_GPUTexture* mSkyAndSunRT = nullptr;
		ER_RHI_GPUTexture* mMainRT = nullptr;
		ER_RHI_GPUTexture* mUpsampleAndBlurRT = nullptr;
		ER_RHI_GPUTexture* mBlurRT = nullptr;
		ER_RHI_GPUTexture* mCloudTextureSRV = nullptr;
		ER_RHI_GPUTexture* mWeatherTextureSRV = nullptr;
		ER_RHI_GPUTexture* mWorleyTextureSRV = nullptr;

		ER_RHI_GPUShader* mMainCS = nullptr;
		ER_RHI_GPUShader* mCompositePS = nullptr;
		ER_RHI_GPUShader* mBlurPS = nullptr;
		ER_RHI_GPUShader* mUpsampleBlurCS = nullptr;

		ER_RHI_GPURootSignature* mMainPassRS = nullptr;
		ER_RHI_GPURootSignature* mUpsampleBlurPassRS = nullptr;
		ER_RHI_GPURootSignature* mCompositePassRS = nullptr;

		const std::string mMainPassPSOName = "ER_RHI_GPUPipelineStateObject: Volumetric Clouds - Main";
		const std::string mCompositePassPSOName = "ER_RHI_GPUPipelineStateObject: Volumetric Clouds - Composite";
		const std::string mBlurPassPSOName = "ER_RHI_GPUPipelineStateObject: Volumetric Clouds - Blur";
		const std::string mUpsampleBlurPSOName = "ER_RHI_GPUPipelineStateObject: Volumetric Clouds - Upsample & blur";

		float mCrispiness = 43.0f;
		float mCurliness = 1.1f;
		float mCoverage = 0.305f;
		float mAmbientColor[3] = { 102.0f / 255.0f, 104.0f / 255.0f, 105.0f / 255.0f };
		float mWindSpeedMultiplier = 175.0f;
		float mLightAbsorption = 0.0015f;
		float mCloudsBottomHeight = 2340.0f;
		float mCloudsTopHeight = 16400.0f;
		float mDensityFactor = 0.006f;
		float mDownscaleFactor = 0.5f;
		float mDistanceToFadeFrom = 58000.0f;
		float mDistanceOfFade = 10000.0f;

		bool mEnabled = true;
		bool mShowDebug = false;

		VolumetricCloudsQuality mCurrentQuality = VC_DISABLED;
	};
}