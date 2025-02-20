#include "ER_VolumetricFog.h"
#include "ER_ShadowMapper.h"
#include "ER_Core.h"
#include "ER_CoreException.h"
#include "ER_DirectionalLight.h"
#include "ER_Utility.h"
#include "ER_Camera.h"
#include "ER_QuadRenderer.h"

#define VOXEL_VOLUME_SIZE_Z 128

#define INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_SRV_INDEX 0
#define INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_UAV_INDEX 1
#define INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_CBV_INDEX 2

#define COMPOSITE_ROOT_DESCRIPTOR_TABLE_SRV_INDEX 0
#define COMPOSITE_ROOT_DESCRIPTOR_TABLE_CBV_INDEX 1

namespace EveryRay_Core {
	ER_VolumetricFog::ER_VolumetricFog(ER_Core& game, const ER_DirectionalLight& aLight, const ER_ShadowMapper& aShadowMapper, VolumetricFogQuality aQuality)
	    : ER_CoreComponent(game), mShadowMapper(aShadowMapper), mDirectionalLight(aLight), mCurrentQuality(aQuality), mPrevViewProj(XMMatrixIdentity())
	{	
	}
	
	ER_VolumetricFog::~ER_VolumetricFog()
	{
		if (mCurrentQuality == VolumetricFogQuality::VF_DISABLED)
			return;

		DeleteObject(mCameraFog);

		DeleteObject(mTempVoxelInjectionTexture3D[0]);
		DeleteObject(mTempVoxelInjectionTexture3D[1]);
		DeleteObject(mFinalVoxelAccumulationTexture3D);
		DeleteObject(mBlueNoiseTexture);
	
		DeleteObject(mInjectionCS);
		DeleteObject(mAccumulationCS);
		DeleteObject(mCompositePS);

		DeleteObject(mInjectionAccumulationPassesRootSignature);
		DeleteObject(mCompositePassRootSignature);

		mMainConstantBuffer.Release();
	}
    
	void ER_VolumetricFog::Initialize()
	{
		switch (mCurrentQuality)
		{
		case VolumetricFogQuality::VF_DISABLED:
			return;
			break;
		case VolumetricFogQuality::VF_MEDIUM:
			mCurrentVoxelVolumeSizeX = 160;
			mCurrentVoxelVolumeSizeY = 90;
			break;
		case VolumetricFogQuality::VF_HIGH:
			mCurrentVoxelVolumeSizeX = 240;
			mCurrentVoxelVolumeSizeY = 135;
			break;
		}

		auto rhi = GetCore()->GetRHI();

		ER_Camera* cameraScene = (ER_Camera*)GetCore()->GetServices().FindService(ER_Camera::TypeIdClass());
		mCameraFog = new ER_Camera(*GetCore(), cameraScene->FieldOfView(), cameraScene->AspectRatio(), mCustomNearPlane, mCustomFarPlane);
		mCameraFog->Initialize();

		mTempVoxelInjectionTexture3D[0] = rhi->CreateGPUTexture(L"ER_RHI_GPUTexture: Volumetric Fog Temp Voxel Injection 3D #0");
		mTempVoxelInjectionTexture3D[0]->CreateGPUTextureResource(rhi, mCurrentVoxelVolumeSizeX, mCurrentVoxelVolumeSizeY, 1, ER_FORMAT_R16G16B16A16_FLOAT,
			ER_BIND_SHADER_RESOURCE | ER_BIND_UNORDERED_ACCESS, 1, VOXEL_VOLUME_SIZE_Z);

		mTempVoxelInjectionTexture3D[1] = rhi->CreateGPUTexture(L"ER_RHI_GPUTexture: Volumetric Fog Temp Voxel Injection 3D #1");
		mTempVoxelInjectionTexture3D[1]->CreateGPUTextureResource(rhi, mCurrentVoxelVolumeSizeX, mCurrentVoxelVolumeSizeY, 1, ER_FORMAT_R16G16B16A16_FLOAT,
			ER_BIND_SHADER_RESOURCE | ER_BIND_UNORDERED_ACCESS, 1, VOXEL_VOLUME_SIZE_Z);

		mFinalVoxelAccumulationTexture3D = rhi->CreateGPUTexture(L"ER_RHI_GPUTexture: Volumetric Fog Final Voxel Accumulation 3D");
		mFinalVoxelAccumulationTexture3D->CreateGPUTextureResource(rhi, mCurrentVoxelVolumeSizeX, mCurrentVoxelVolumeSizeY, 1, ER_FORMAT_R16G16B16A16_FLOAT,
			ER_BIND_SHADER_RESOURCE | ER_BIND_UNORDERED_ACCESS, 1, VOXEL_VOLUME_SIZE_Z);

		mBlueNoiseTexture = rhi->CreateGPUTexture(L"");
		mBlueNoiseTexture->CreateGPUTextureResource(rhi, "content\\textures\\blueNoise.dds");

		mInjectionCS = rhi->CreateGPUShader();
		mInjectionCS->CompileShader(rhi, "content\\shaders\\VolumetricFog\\VolumetricFogMain.hlsl", "CSInjection", ER_COMPUTE);
		mAccumulationCS = rhi->CreateGPUShader();
		mAccumulationCS->CompileShader(rhi, "content\\shaders\\VolumetricFog\\VolumetricFogMain.hlsl", "CSAccumulation", ER_COMPUTE);

		mInjectionAccumulationPassesRootSignature = rhi->CreateRootSignature(3, 2);
		if (mInjectionAccumulationPassesRootSignature)
		{
			mInjectionAccumulationPassesRootSignature->InitStaticSampler(rhi, 0, ER_RHI_SAMPLER_STATE::ER_TRILINEAR_WRAP);
			mInjectionAccumulationPassesRootSignature->InitStaticSampler(rhi, 1, ER_RHI_SAMPLER_STATE::ER_SHADOW_SS);
			mInjectionAccumulationPassesRootSignature->InitDescriptorTable(rhi, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_SRV_INDEX, { ER_RHI_DESCRIPTOR_RANGE_TYPE::ER_RHI_DESCRIPTOR_RANGE_TYPE_SRV }, { 0 }, { 3 });
			mInjectionAccumulationPassesRootSignature->InitDescriptorTable(rhi, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_UAV_INDEX, { ER_RHI_DESCRIPTOR_RANGE_TYPE::ER_RHI_DESCRIPTOR_RANGE_TYPE_UAV }, { 0 }, { 1 });
			mInjectionAccumulationPassesRootSignature->InitDescriptorTable(rhi, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_CBV_INDEX, { ER_RHI_DESCRIPTOR_RANGE_TYPE::ER_RHI_DESCRIPTOR_RANGE_TYPE_CBV }, { 0 }, { 1 });
			mInjectionAccumulationPassesRootSignature->Finalize(rhi, "ER_RHI_GPURootSignature: Volumetric Fog: Injection + Accumulation Passes");
		}
		
		mCompositePS = rhi->CreateGPUShader();
		mCompositePS->CompileShader(rhi, "content\\shaders\\VolumetricFog\\VolumetricFogComposite.hlsl", "PSComposite", ER_PIXEL);

		mCompositePassRootSignature = rhi->CreateRootSignature(2, 1);
		if (mCompositePassRootSignature)
		{
			mCompositePassRootSignature->InitStaticSampler(rhi, 0, ER_RHI_SAMPLER_STATE::ER_TRILINEAR_WRAP, ER_RHI_SHADER_VISIBILITY_PIXEL);
			mCompositePassRootSignature->InitDescriptorTable(rhi, COMPOSITE_ROOT_DESCRIPTOR_TABLE_SRV_INDEX, { ER_RHI_DESCRIPTOR_RANGE_TYPE::ER_RHI_DESCRIPTOR_RANGE_TYPE_SRV }, { 0 }, { 3 }, ER_RHI_SHADER_VISIBILITY_PIXEL);
			mCompositePassRootSignature->InitDescriptorTable(rhi, COMPOSITE_ROOT_DESCRIPTOR_TABLE_CBV_INDEX, { ER_RHI_DESCRIPTOR_RANGE_TYPE::ER_RHI_DESCRIPTOR_RANGE_TYPE_CBV }, { 0 }, { 1 }, ER_RHI_SHADER_VISIBILITY_PIXEL);
			mCompositePassRootSignature->Finalize(rhi, "ER_RHI_GPURootSignature: Volumetric Fog: Composite Pass", true);
		}

		mMainConstantBuffer.Initialize(rhi, "ER_RHI_GPUBuffer: Volumetric Fog Main CB");
		mCompositeConstantBuffer.Initialize(rhi, "ER_RHI_GPUBuffer: Volumetric Fog Composite CB");
	}

	void ER_VolumetricFog::Draw()
	{
		if (mCurrentQuality == VolumetricFogQuality::VF_DISABLED || !mEnabled)
			return;

		auto rhi = GetCore()->GetRHI();
		rhi->SetRootSignature(mInjectionAccumulationPassesRootSignature, true);

		rhi->BeginEventTag("EveryRay: Volumetric Fog (injection)");
		ComputeInjection();
		rhi->EndEventTag();

		rhi->BeginEventTag("EveryRay: Volumetric Fog (accumulation)");
		ComputeAccumulation();
		rhi->EndEventTag();
	}

	void ER_VolumetricFog::Update(const ER_CoreTime& gameTime)
	{
		if (mCurrentQuality == VolumetricFogQuality::VF_DISABLED)
			return;

		UpdateImGui();

		if (!mEnabled)
			return;

		ER_Camera* cameraScene = (ER_Camera*)GetCore()->GetServices().FindService(ER_Camera::TypeIdClass());
		ER_Camera* camera = mCameraFog;
		camera->SetPosition(cameraScene->Position());
		camera->SetDirection(cameraScene->Direction());
		camera->SetUp(cameraScene->Up());
		camera->SetFovNearFarPlanesDistance(cameraScene->FieldOfView(), mCustomNearPlane, mCustomFarPlane);
		camera->Update(gameTime);

		auto rhi = GetCore()->GetRHI();

		mMainConstantBuffer.Data.InvViewProj = XMMatrixTranspose(XMMatrixInverse(nullptr, camera->ViewMatrix() * camera->ProjectionMatrix()));
		mMainConstantBuffer.Data.PrevViewProj = mPrevViewProj;
		mMainConstantBuffer.Data.ShadowMatrix = mShadowMapper.GetViewMatrix(0) * mShadowMapper.GetProjectionMatrix(0) /** XMLoadFloat4x4(&MatrixHelper::GetProjectionShadowMatrix())*/;
		mMainConstantBuffer.Data.SunDirection = XMFLOAT4{ -mDirectionalLight.Direction().x, -mDirectionalLight.Direction().y, -mDirectionalLight.Direction().z, 1.0f };
		mMainConstantBuffer.Data.SunColor = XMFLOAT4{ mDirectionalLight.GetColor().x, mDirectionalLight.GetColor().y, mDirectionalLight.GetColor().z, mDirectionalLight.mLightIntensity };
		mMainConstantBuffer.Data.CameraPosition = XMFLOAT4{ camera->Position().x, camera->Position().y, camera->Position().z, 1.0f };
		mMainConstantBuffer.Data.CameraNearFar_FrameIndex_PreviousFrameBlend = XMFLOAT4{ mCustomNearPlane, mCustomFarPlane, static_cast<float>(GetCore()->GetFrameIndex()), mPreviousFrameBlendFactor };
		mMainConstantBuffer.Data.VolumeSize = XMFLOAT4{ static_cast<float>(mCurrentVoxelVolumeSizeX), static_cast<float>(mCurrentVoxelVolumeSizeY), VOXEL_VOLUME_SIZE_Z, 0.0f };
		mMainConstantBuffer.Data.Anisotropy = mAnisotropy;
		mMainConstantBuffer.Data.Density = mDensity;
		mMainConstantBuffer.Data.Strength = mStrength;
		mMainConstantBuffer.Data.ThicknessFactor = mThicknessFactor;
		mMainConstantBuffer.ApplyChanges(rhi);

		mCompositeConstantBuffer.Data.ViewProj = XMMatrixTranspose(camera->ViewMatrix() * camera->ProjectionMatrix());
		mCompositeConstantBuffer.Data.CameraNearFar = XMFLOAT4{ mCustomNearPlane, mCustomFarPlane, 0.0f, 0.0f };
		mCompositeConstantBuffer.Data.VolumeSize = mMainConstantBuffer.Data.VolumeSize;
		mCompositeConstantBuffer.Data.BlendingWithSceneColorFactor = mBlendingWithSceneColorFactor;
		mCompositeConstantBuffer.ApplyChanges(rhi);
		
		mPrevViewProj = mCompositeConstantBuffer.Data.ViewProj;
	}

	void ER_VolumetricFog::UpdateImGui()
	{
		if (!mShowDebug)
			return;

		ImGui::Begin("Volumetric Fog System");
		ImGui::Checkbox("Enabled", &mEnabled);
		ImGui::SliderFloat("Anisotropy", &mAnisotropy, 0.0f, 1.0f);
		ImGui::SliderFloat("Density", &mDensity, 0.1f, 10.0f);
		ImGui::SliderFloat("Strength", &mStrength, 0.0f, 50.0f);
		//ImGui::SliderFloat("Thickness", &mThicknessFactor, 0.0f, 0.1f);
		ImGui::SliderFloat("Blending with scene", &mBlendingWithSceneColorFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Blending with previous frame", &mPreviousFrameBlendFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Custom near plane", &mCustomNearPlane, 0.01f, 10.0f);
		ImGui::SliderFloat("Custom far plane", &mCustomFarPlane, 10.0f, 10000.0f);
		ImGui::End();
	}

	void ER_VolumetricFog::ComputeInjection()
	{
		auto rhi = GetCore()->GetRHI();

		int readIndex = mCurrentTexture3DRead;
		int writeIndex = !mCurrentTexture3DRead;

		if (!rhi->IsPSOReady(mInjectionPassPSOName, true))
		{
			rhi->InitializePSO(mInjectionPassPSOName, true);
			rhi->SetRootSignatureToPSO(mInjectionPassPSOName, mInjectionAccumulationPassesRootSignature, true);
			rhi->SetShader(mInjectionCS);
			rhi->FinalizePSO(mInjectionPassPSOName, true);
		}
		rhi->SetPSO(mInjectionPassPSOName, true);
		// we set common injection/accumulation root signature in ER_VolumetricFog::Draw()
		rhi->SetShaderResources(ER_COMPUTE, { mShadowMapper.GetShadowTexture(0), mBlueNoiseTexture, mTempVoxelInjectionTexture3D[readIndex] }, 0, 
			mInjectionAccumulationPassesRootSignature, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_SRV_INDEX, true);
		rhi->SetUnorderedAccessResources(ER_COMPUTE, { mTempVoxelInjectionTexture3D[writeIndex] }, 0, 
			mInjectionAccumulationPassesRootSignature, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_UAV_INDEX, true);
		rhi->SetConstantBuffers(ER_COMPUTE, { mMainConstantBuffer.Buffer() }, 0, 
			mInjectionAccumulationPassesRootSignature, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_CBV_INDEX, true);
		rhi->SetSamplers(ER_COMPUTE, { ER_RHI_SAMPLER_STATE::ER_TRILINEAR_CLAMP, ER_RHI_SAMPLER_STATE::ER_SHADOW_SS });
		rhi->Dispatch(ER_CEIL(mCurrentVoxelVolumeSizeX, 8), ER_CEIL(mCurrentVoxelVolumeSizeY, 8), ER_CEIL(VOXEL_VOLUME_SIZE_Z, 1));
		rhi->UnsetPSO();
		rhi->UnbindResourcesFromShader(ER_COMPUTE);

		mCurrentTexture3DRead = !mCurrentTexture3DRead;
	}

	void ER_VolumetricFog::ComputeAccumulation()
	{
		auto rhi = GetCore()->GetRHI();

		int readIndex = mCurrentTexture3DRead;

		if (!rhi->IsPSOReady(mAccumulationPassPSOName, true))
		{
			rhi->InitializePSO(mAccumulationPassPSOName, true);
			rhi->SetRootSignatureToPSO(mAccumulationPassPSOName, mInjectionAccumulationPassesRootSignature, true);
			rhi->SetShader(mAccumulationCS);
			rhi->FinalizePSO(mAccumulationPassPSOName, true);
		}
		rhi->SetPSO(mAccumulationPassPSOName, true);
		// we set common injection/accumulation root signature in ER_VolumetricFog::Draw()
		rhi->SetShaderResources(ER_COMPUTE, { mShadowMapper.GetShadowTexture(0), mBlueNoiseTexture, mTempVoxelInjectionTexture3D[readIndex] }, 0, 
			mInjectionAccumulationPassesRootSignature, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_SRV_INDEX, true);
		rhi->SetUnorderedAccessResources(ER_COMPUTE, { mFinalVoxelAccumulationTexture3D }, 0, 
			mInjectionAccumulationPassesRootSignature, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_UAV_INDEX, true);
		rhi->SetConstantBuffers(ER_COMPUTE, { mMainConstantBuffer.Buffer() }, 0,
			mInjectionAccumulationPassesRootSignature, INJECTION_ACCUMULATION_ROOT_DESCRIPTOR_TABLE_CBV_INDEX, true);
		rhi->SetSamplers(ER_COMPUTE, { ER_RHI_SAMPLER_STATE::ER_TRILINEAR_WRAP, ER_RHI_SAMPLER_STATE::ER_SHADOW_SS });
		rhi->Dispatch(ER_CEIL(mCurrentVoxelVolumeSizeX, 8), ER_CEIL(mCurrentVoxelVolumeSizeY, 8), 1);
		rhi->UnsetPSO();
		rhi->UnbindResourcesFromShader(ER_COMPUTE);
	}

	void ER_VolumetricFog::Composite(ER_RHI_GPUTexture* aRT, ER_RHI_GPUTexture* aInputColorTexture, ER_RHI_GPUTexture* aGbufferWorldPos)
	{
		assert(aGbufferWorldPos && aInputColorTexture && aRT);

		ER_QuadRenderer* quadRenderer = (ER_QuadRenderer*)mCore->GetServices().FindService(ER_QuadRenderer::TypeIdClass());
		assert(quadRenderer);

		auto rhi = GetCore()->GetRHI();
		rhi->SetRootSignature(mCompositePassRootSignature);
		rhi->SetTopologyType(ER_RHI_PRIMITIVE_TYPE::ER_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		if (!rhi->IsPSOReady(mCompositePassPSOName))
		{
			rhi->InitializePSO(mCompositePassPSOName);
			rhi->SetShader(mCompositePS);
			rhi->SetRenderTargetFormats({ aRT });
			rhi->SetBlendState(ER_NO_BLEND);
			rhi->SetRasterizerState(ER_NO_CULLING);
			rhi->SetRootSignatureToPSO(mCompositePassPSOName, mCompositePassRootSignature);
			rhi->SetTopologyTypeToPSO(mCompositePassPSOName, ER_RHI_PRIMITIVE_TYPE::ER_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			quadRenderer->PrepareDraw(rhi);
			rhi->FinalizePSO(mCompositePassPSOName);
		}
		rhi->SetPSO(mCompositePassPSOName);
		rhi->SetShaderResources(ER_PIXEL, { aInputColorTexture, aGbufferWorldPos, mFinalVoxelAccumulationTexture3D }, 0, 
			mCompositePassRootSignature, COMPOSITE_ROOT_DESCRIPTOR_TABLE_SRV_INDEX);
		rhi->SetConstantBuffers(ER_PIXEL, { mCompositeConstantBuffer.Buffer() }, 0,
			mCompositePassRootSignature, COMPOSITE_ROOT_DESCRIPTOR_TABLE_CBV_INDEX);
		rhi->SetSamplers(ER_PIXEL, { ER_RHI_SAMPLER_STATE::ER_TRILINEAR_WRAP });
		quadRenderer->Draw(rhi);
		rhi->UnsetPSO();

		rhi->UnbindResourcesFromShader(ER_PIXEL);
	}
}