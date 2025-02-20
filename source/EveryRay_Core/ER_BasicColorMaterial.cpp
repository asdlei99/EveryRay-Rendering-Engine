#include "ER_BasicColorMaterial.h"
#include "ER_Utility.h"
#include "ER_CoreException.h"
#include "ER_Core.h"
#include "ER_Camera.h"
#include "ER_RenderingObject.h"
#include "ER_Mesh.h"
#include "ER_MaterialsCallbacks.h"
#include "ER_Illumination.h"

static const std::string psoNameNonInstanced = "ER_RHI_GPUPipelineStateObject: BasicColorMaterial";
static const std::string psoNameInstanced = "ER_RHI_GPUPipelineStateObject: BasicColorMaterial w/ Instancing";

namespace EveryRay_Core
{
	ER_BasicColorMaterial::ER_BasicColorMaterial(ER_Core& game, const MaterialShaderEntries& entries, unsigned int shaderFlags, bool instanced)
		: ER_Material(game, entries, shaderFlags)
	{
		//TODO instanced support
		if (shaderFlags & HAS_VERTEX_SHADER)
		{
			ER_RHI_INPUT_ELEMENT_DESC inputElementDescriptions[] =
			{
				{ "POSITION", 0, ER_FORMAT_R32G32B32A32_FLOAT, 0, 0, true, 0 }
			};
			ER_Material::CreateVertexShader("content\\shaders\\BasicColor.hlsl", inputElementDescriptions, ARRAYSIZE(inputElementDescriptions));
		}
		
		if (shaderFlags & HAS_PIXEL_SHADER)
			ER_Material::CreatePixelShader("content\\shaders\\BasicColor.hlsl");

		mConstantBuffer.Initialize(ER_Material::GetCore()->GetRHI(), "ER_RHI_GPUBuffer: BasicColorMaterial CB");
	}

	ER_BasicColorMaterial::~ER_BasicColorMaterial()
	{
		mConstantBuffer.Release();
		ER_Material::~ER_Material();
	}

	// callback method for "ER_RenderingObject" that has this material
	void ER_BasicColorMaterial::PrepareResourcesForStandardMaterial(ER_MaterialSystems neededSystems, ER_RenderingObject* aObj, int meshIndex, ER_RHI_GPURootSignature* rs)
	{
		auto rhi = ER_Material::GetCore()->GetRHI();
		ER_Camera* camera = (ER_Camera*)(ER_Material::GetCore()->GetServices().FindService(ER_Camera::TypeIdClass()));
		
		assert(aObj);
		assert(camera);
		assert(neededSystems.mIllumination);

		mConstantBuffer.Data.World = XMMatrixTranspose(aObj->GetTransformationMatrix());
		mConstantBuffer.Data.ViewProjection = XMMatrixTranspose(camera->ViewMatrix() * camera->ProjectionMatrix());
		mConstantBuffer.Data.Color = XMFLOAT4{0.0, 0.0, 0.0, 0.0};
		mConstantBuffer.ApplyChanges(rhi);

		rhi->SetRootSignature(rs);
		rhi->SetTopologyType(ER_RHI_PRIMITIVE_TYPE::ER_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		const std::string& psoName = psoNameNonInstanced; //TODO add instancing support
		if (!rhi->IsPSOReady(psoName))
		{
			rhi->InitializePSO(psoName);
			PrepareShaders();
			rhi->SetRenderTargetFormats({ neededSystems.mIllumination->GetLocalIlluminationRT() }, neededSystems.mIllumination->GetGBufferDepth()); // we assume that we render in local RT (don't like it but idk how to properly pass RT atm)
			rhi->SetRasterizerState(ER_NO_CULLING);
			rhi->SetBlendState(ER_NO_BLEND);
			rhi->SetDepthStencilState(ER_DEPTH_ONLY_WRITE_COMPARISON_LESS_EQUAL);
			rhi->SetRootSignatureToPSO(psoName, rs);
			rhi->SetTopologyTypeToPSO(psoName, ER_RHI_PRIMITIVE_TYPE::ER_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			rhi->FinalizePSO(psoName);
		}
		rhi->SetPSO(psoName);
		rhi->SetConstantBuffers(ER_VERTEX, { mConstantBuffer.Buffer() }, 0, rs, BASICCOLOR_MAT_ROOT_DESCRIPTOR_TABLE_CBV_INDEX);
		rhi->SetConstantBuffers(ER_PIXEL,  { mConstantBuffer.Buffer() }, 0, rs, BASICCOLOR_MAT_ROOT_DESCRIPTOR_TABLE_CBV_INDEX);
	}

	// non-callback method for non-"RenderingObject" draws
	void ER_BasicColorMaterial::PrepareForRendering(const XMMATRIX& worldTransform, const XMFLOAT4& color, ER_RHI_GPURootSignature* rs)
	{
		auto rhi = ER_Material::GetCore()->GetRHI();
		ER_Camera* camera = (ER_Camera*)(ER_Material::GetCore()->GetServices().FindService(ER_Camera::TypeIdClass()));

		assert(camera);

		mConstantBuffer.Data.World = XMMatrixTranspose(worldTransform);
		mConstantBuffer.Data.ViewProjection = XMMatrixTranspose(camera->ViewMatrix() * camera->ProjectionMatrix());
		mConstantBuffer.Data.Color = color;
		mConstantBuffer.ApplyChanges(rhi);

		rhi->SetConstantBuffers(ER_VERTEX, { mConstantBuffer.Buffer() }, 0, rs, BASICCOLOR_MAT_ROOT_DESCRIPTOR_TABLE_CBV_INDEX);
		rhi->SetConstantBuffers(ER_PIXEL,  { mConstantBuffer.Buffer() }, 0, rs, BASICCOLOR_MAT_ROOT_DESCRIPTOR_TABLE_CBV_INDEX);
	}

	void ER_BasicColorMaterial::CreateVertexBuffer(const ER_Mesh& mesh, ER_RHI_GPUBuffer* vertexBuffer)
	{
		mesh.CreateVertexBuffer_Position(vertexBuffer);
	}

	int ER_BasicColorMaterial::VertexSize()
	{
		return sizeof(VertexPosition);
	}

}