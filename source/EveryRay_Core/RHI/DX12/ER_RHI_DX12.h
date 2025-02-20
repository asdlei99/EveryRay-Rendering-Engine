#pragma once
#include "..\ER_RHI.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include "pix3.h"
//#include <dxc/dxcapi.h>
#if defined (_DEBUG) || (DEBUG)
#include <dxgidebug.h>
#endif
#include <d3dcompiler.h>

#include "imgui_impl_dx12.h"

#define DX12_MAX_BOUND_RENDER_TARGETS_VIEWS 8
#define DX12_MAX_BOUND_SHADER_RESOURCE_VIEWS 64 
#define DX12_MAX_BOUND_UNORDERED_ACCESS_VIEWS 8 
#define DX12_MAX_BOUND_CONSTANT_BUFFERS 8 
#define DX12_MAX_BOUND_SAMPLERS 8 
#define DX12_MAX_BOUND_ROOT_PARAMS 8 
#define DX12_MAX_BACK_BUFFER_COUNT 2

#define DX12_MAX_GENERATE_MIPS_TEXTURES_IN_POOL 2048 // max # of textures pending for GenerateMipsWithTextureReplacement();

namespace EveryRay_Core
{
	enum ER_RHI_DX12_PSO_STATE
	{
		UNSET = 0,
		GRAPHICS,
		COMPUTE
	};

	class ER_RHI_DX12_GraphicsPSO;
	class ER_RHI_DX12_ComputePSO;
	class ER_RHI_DX12_GPURootSignature;
	class ER_RHI_DX12_GPUDescriptorHeapManager;
	class ER_RHI_DX12_DescriptorHandle;

	class ER_RHI_DX12: public ER_RHI
	{
	public:
		ER_RHI_DX12();
		virtual ~ER_RHI_DX12();

		virtual bool Initialize(HWND windowHandle, UINT width, UINT height, bool isFullscreen, bool isReset) override;
		
		virtual void BeginGraphicsCommandList(int index = 0) override;
		virtual void EndGraphicsCommandList(int index = 0) override;

		virtual void BeginComputeCommandList(int index = 0) override {}; //TODO
		virtual void EndComputeCommandList(int index = 0) override {}; //TODO

		virtual void BeginCopyCommandList(int index = 0) override;
		virtual void EndCopyCommandList(int index = 0) override;

		virtual void ClearMainRenderTarget(float colors[4]) override;
		virtual void ClearMainDepthStencilTarget(float depth, UINT stencil = 0) override;
		virtual void ClearRenderTarget(ER_RHI_GPUTexture* aRenderTarget, float colors[4], int rtvArrayIndex = -1) override;
		virtual void ClearDepthStencilTarget(ER_RHI_GPUTexture* aDepthTarget, float depth, UINT stencil = -1) override;
		virtual void ClearUAV(ER_RHI_GPUResource* aRenderTarget, float colors[4]) override;
		virtual void ClearUAV(ER_RHI_GPUBuffer* aBuffer, UINT value) override;
		
		virtual ER_RHI_GPUShader* CreateGPUShader() override;
		virtual ER_RHI_GPUBuffer* CreateGPUBuffer(const std::string& aDebugName) override;
		virtual ER_RHI_GPUTexture* CreateGPUTexture(const std::wstring& aDebugName) override;
		virtual ER_RHI_GPURootSignature* CreateRootSignature(UINT NumRootParams = 0, UINT NumStaticSamplers = 0) override;
		virtual ER_RHI_InputLayout* CreateInputLayout(ER_RHI_INPUT_ELEMENT_DESC* inputElementDescriptions, UINT inputElementDescriptionCount) override;

		virtual void CreateTexture(ER_RHI_GPUTexture* aOutTexture, UINT width, UINT height, UINT samples, ER_RHI_FORMAT format, ER_RHI_BIND_FLAG bindFlags = ER_BIND_NONE,
			int mip = 1, int depth = -1, int arraySize = 1, bool isCubemap = false, int cubemapArraySize = -1) override;
		virtual void CreateTexture(ER_RHI_GPUTexture* aOutTexture, const std::string& aPath, bool isFullPath = false) override;
		virtual void CreateTexture(ER_RHI_GPUTexture* aOutTexture, const std::wstring& aPath, bool isFullPath = false) override;

		virtual void CreateBuffer(ER_RHI_GPUBuffer* aOutBuffer, void* aData, UINT objectsCount, UINT byteStride, bool isDynamic = false, ER_RHI_BIND_FLAG bindFlags = ER_BIND_NONE, UINT cpuAccessFlags = 0, ER_RHI_RESOURCE_MISC_FLAG miscFlags = ER_RESOURCE_MISC_NONE, ER_RHI_FORMAT format = ER_FORMAT_UNKNOWN) override;
		virtual void CopyBuffer(ER_RHI_GPUBuffer* aDestBuffer, ER_RHI_GPUBuffer* aSrcBuffer, int cmdListIndex, bool isInCopyQueue = false) override;
		virtual void BeginBufferRead(ER_RHI_GPUBuffer* aBuffer, void** output) override;
		virtual void EndBufferRead(ER_RHI_GPUBuffer* aBuffer) override;

		virtual void CopyGPUTextureSubresourceRegion(ER_RHI_GPUResource* aDestBuffer, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ER_RHI_GPUResource* aSrcBuffer, UINT SrcSubresource, bool isInCopyQueueOrSkipTransitions = false) override;

		virtual void Draw(UINT VertexCount) override;
		virtual void DrawIndexed(UINT IndexCount) override;
		virtual void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) override;
		virtual void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) override;
		virtual void DrawIndexedInstancedIndirect(ER_RHI_GPUBuffer* anArgsBuffer, UINT alignedByteOffset) override;

		virtual void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;
		//TODO DispatchIndirect

		virtual void ExecuteCommandLists(int commandListIndex = 0, bool isCompute = false) override;
		virtual void ExecuteCopyCommandList() override;

		virtual void GenerateMips(ER_RHI_GPUTexture* aTexture, ER_RHI_GPUTexture* aSRGBTexture = nullptr) override;
		virtual void GenerateMipsWithTextureReplacement(ER_RHI_GPUTexture** aTexture, std::function<void(ER_RHI_GPUTexture**)> aReplacementCallback) override;
		virtual void ReplaceOriginalTexturesWithMipped() override;

		virtual void PresentGraphics() override;
		virtual void PresentCompute() override;
		
		virtual bool ProjectCubemapToSH(ER_RHI_GPUTexture* aTexture, UINT order, float* resultR, float* resultG, float* resultB) override;
		
		virtual void SaveGPUTextureToFile(ER_RHI_GPUTexture* aTexture, const std::wstring& aPathName) override;

		virtual void SetMainRenderTargets(int cmdListIndex = 0) override;
		virtual void SetRenderTargets(const std::vector<ER_RHI_GPUTexture*>& aRenderTargets, ER_RHI_GPUTexture* aDepthTarget = nullptr, ER_RHI_GPUTexture* aUAV = nullptr, int rtvArrayIndex = -1) override;
		virtual void SetDepthTarget(ER_RHI_GPUTexture* aDepthTarget) override;
		virtual void SetRenderTargetFormats(const std::vector<ER_RHI_GPUTexture*>& aRenderTargets, ER_RHI_GPUTexture* aDepthTarget = nullptr) override;
		virtual void SetMainRenderTargetFormats() override;

		virtual void SetDepthStencilState(ER_RHI_DEPTH_STENCIL_STATE aDS, UINT stencilRef = 0xffffffff) override;

		virtual void SetBlendState(ER_RHI_BLEND_STATE aBS, const float BlendFactor[4] = nullptr, UINT SampleMask = 0xffffffff) override;
		
		virtual void SetRasterizerState(ER_RHI_RASTERIZER_STATE aRS) override;
		
		virtual void SetViewport(const ER_RHI_Viewport& aViewport) override;
		
		virtual void SetRect(const ER_RHI_Rect& rect) override;
		
		virtual void SetShader(ER_RHI_GPUShader* aShader) override;
		
		virtual void SetShaderResources(ER_RHI_SHADER_TYPE aShaderType, const std::vector<ER_RHI_GPUResource*>& aSRVs, UINT startSlot = 0, 
			ER_RHI_GPURootSignature* rs = nullptr, int rootParamIndex = -1, bool isComputeRS = false, bool skipAutomaticTransition = false) override;
		virtual void SetUnorderedAccessResources(ER_RHI_SHADER_TYPE aShaderType, const std::vector<ER_RHI_GPUResource*>& aUAVs, UINT startSlot = 0,
			ER_RHI_GPURootSignature* rs = nullptr, int rootParamIndex = -1, bool isComputeRS = false, bool skipAutomaticTransition = false) override;
		virtual void SetConstantBuffers(ER_RHI_SHADER_TYPE aShaderType, const std::vector<ER_RHI_GPUBuffer*>& aCBs, UINT startSlot = 0,
			ER_RHI_GPURootSignature* rs = nullptr, int rootParamIndex = -1, bool isComputeRS = false) override;
		virtual void SetSamplers(ER_RHI_SHADER_TYPE aShaderType, const std::vector<ER_RHI_SAMPLER_STATE>& aSamplers, UINT startSlot = 0, ER_RHI_GPURootSignature* rs = nullptr) override;
		
		virtual void SetRootSignature(ER_RHI_GPURootSignature* rs, bool isCompute = false) override;
		virtual void SetRootConstant(UINT aConstant, UINT aRootIndex, UINT anOffset = 0, bool isCompute = false) override;

		virtual void SetInputLayout(ER_RHI_InputLayout* aIL) override;
		virtual void SetEmptyInputLayout() override;
		virtual void SetIndexBuffer(ER_RHI_GPUBuffer* aBuffer, UINT offset = 0) override;
		virtual void SetVertexBuffers(const std::vector<ER_RHI_GPUBuffer*>& aVertexBuffers) override;

		virtual void SetTopologyType(ER_RHI_PRIMITIVE_TYPE aType) override;
		virtual ER_RHI_PRIMITIVE_TYPE GetCurrentTopologyType() override;

		virtual void SetGPUDescriptorHeap(ER_RHI_DESCRIPTOR_HEAP_TYPE aType, bool aReset) override;
		virtual void SetGPUDescriptorHeapImGui(int cmdListIndex = 0) override;

		virtual void TransitionResources(const std::vector<ER_RHI_GPUResource*>& aResources, const std::vector<ER_RHI_RESOURCE_STATE>& aStates, int cmdListIndex = 0, bool isCopyQueue = false, int subresourceIndex = -1) override;
		virtual void TransitionResources(const std::vector<ER_RHI_GPUResource*>& aResources, ER_RHI_RESOURCE_STATE aState, int cmdListIndex = 0, bool isCopyQueue = false, int subresourceIndex = -1) override;
		virtual void TransitionMainRenderTargetToPresent(int cmdListIndex = 0) override;

		virtual bool IsPSOReady(const std::string& aName, bool isCompute = false) override;
		virtual void InitializePSO(const std::string& aName, bool isCompute = false) override;
		virtual void SetRootSignatureToPSO(const std::string& aName, ER_RHI_GPURootSignature* rs, bool isCompute = false) override;
		virtual void SetTopologyTypeToPSO(const std::string& aName, ER_RHI_PRIMITIVE_TYPE aType) override;
		virtual void FinalizePSO(const std::string& aName, bool isCompute = false) override;
		virtual void SetPSO(const std::string& aName, bool isCompute = false) override;
		virtual void UnsetPSO() override;

		virtual void UnbindRenderTargets() override;
		virtual void UnbindResourcesFromShader(ER_RHI_SHADER_TYPE aShaderType, bool unbindShader = true) override {}; //Not needed on DX12

		virtual void UpdateBuffer(ER_RHI_GPUBuffer* aBuffer, void* aData, int dataSize, bool updateForAllBackBuffers = false) override;
		
		virtual bool IsHardwareRaytracingSupported() override { return mIsRaytracingTierAvailable; }
		virtual bool IsRootConstantSupported()  override { return true; }

		virtual void InitImGui() override;
		virtual void StartNewImGuiFrame() override;
		virtual void RenderDrawDataImGui(int cmdListIndex = 0) override;
		virtual void ShutdownImGui() override;

		virtual void OnWindowSizeChanged(int width, int height) override;

		virtual void WaitForGpuOnGraphicsFence() override;
		virtual void WaitForGpuOnComputeFence() override;
		virtual void WaitForGpuOnCopyFence() override;

		virtual void ResetReplacementMippedTexturesPool() override;
		virtual void ResetDescriptorManager() override;
		virtual void ResetRHI(int width, int height, bool isFullscreen) override;

		virtual void BeginEventTag(const std::string& aName, bool isComputeQueue = false) override;
		virtual void EndEventTag(bool isComputeQueue = false) override;

		ID3D12Device* GetDevice() const { return mDevice.Get(); }
		ID3D12Device5* GetDeviceRaytracing() const { return (ID3D12Device5*)mDevice.Get(); }
		ID3D12GraphicsCommandList* GetGraphicsCommandList(int index) const { return mCommandListGraphics[index].Get(); }
		ID3D12GraphicsCommandList* GetComputeCommandList(int index) const { return mCommandListCompute[index].Get(); }
		ER_RHI_DX12_GPUDescriptorHeapManager* GetDescriptorHeapManager() const { return mDescriptorHeapManager; }

		const D3D12_SAMPLER_DESC& FindSamplerState(ER_RHI_SAMPLER_STATE aState);
		DXGI_FORMAT GetFormat(ER_RHI_FORMAT aFormat);
		ER_RHI_RESOURCE_STATE GetState(D3D12_RESOURCE_STATES aState);
		D3D12_RESOURCE_STATES GetState(ER_RHI_RESOURCE_STATE aState);
		D3D12_SHADER_VISIBILITY GetShaderVisibility(ER_RHI_SHADER_VISIBILITY aVis);
		D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeType(ER_RHI_DESCRIPTOR_RANGE_TYPE aDesc);

		ER_GRAPHICS_API GetAPI() { return mAPI; }
		static int mBackBufferIndex;
	private:
		inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetMainRenderTargetView() const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(mBackBufferIndex), mRTVDescriptorSize); }
		inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetMainDepthStencilView() const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }

		D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(ER_RHI_PRIMITIVE_TYPE aType);
		ER_RHI_PRIMITIVE_TYPE GetTopologyType(D3D12_PRIMITIVE_TOPOLOGY aType);
		D3D12_PRIMITIVE_TOPOLOGY GetTopology(ER_RHI_PRIMITIVE_TYPE aType);

		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType(ER_RHI_DESCRIPTOR_HEAP_TYPE aType);

		DXGI_FORMAT ChangeFormatToNonSRGB(DXGI_FORMAT aFormat);
		DXGI_FORMAT ChangeFormatToUncompressed(DXGI_FORMAT aFormat);
		bool IsFormatSRGB(DXGI_FORMAT aFormat);

		void CreateMainRenderTargetAndDepth(int width, int height);
		void CreateSamplerStates();
		void CreateBlendStates();
		void CreateRasterizerStates();
		void CreateDepthStencilStates();

		D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_12_1;
		
		ComPtr<IDXGIFactory4> mDXGIFactory;
		DWORD mDXGIFactoryFlags;
		ComPtr<IDXGISwapChain3> mSwapChain;
		ComPtr<ID3D12Device> mDevice;

		DXGI_FORMAT mMainRTBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
		DXGI_FORMAT mMainDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ComPtr<ID3D12Resource> mMainRenderTarget[DX12_MAX_BACK_BUFFER_COUNT];
		ComPtr<ID3D12Resource> mMainDepthStencilTarget;
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> mDSVDescriptorHeap;
		UINT mRTVDescriptorSize;

		ComPtr<ID3D12DescriptorHeap> mImGuiDescriptorHeap;

		// graphics
		ComPtr<ID3D12CommandQueue> mCommandQueueGraphics;
		ComPtr<ID3D12GraphicsCommandList> mCommandListGraphics[ER_RHI_MAX_GRAPHICS_COMMAND_LISTS];
		ComPtr<ID3D12CommandAllocator> mCommandAllocatorsGraphics[DX12_MAX_BACK_BUFFER_COUNT][ER_RHI_MAX_GRAPHICS_COMMAND_LISTS];
		
		ComPtr<ID3D12Fence> mFenceGraphics;
		UINT64 mFenceValuesGraphics[DX12_MAX_BACK_BUFFER_COUNT] = {};
		Wrappers::Event mFenceEventGraphics;
		
		// compute
		ComPtr<ID3D12CommandQueue> mCommandQueueCompute;
		ComPtr<ID3D12GraphicsCommandList> mCommandListCompute[ER_RHI_MAX_COMPUTE_COMMAND_LISTS];
		ComPtr<ID3D12CommandAllocator> mCommandAllocatorsCompute[DX12_MAX_BACK_BUFFER_COUNT][ER_RHI_MAX_COMPUTE_COMMAND_LISTS];

		ComPtr<ID3D12Fence> mFenceCompute;
		UINT64 mFenceValuesCompute;
		Wrappers::Event mFenceEventCompute;
		
		// copy
		ComPtr<ID3D12CommandQueue> mCommandQueueCopy;
		ComPtr<ID3D12GraphicsCommandList> mCommandListCopy;
		ComPtr<ID3D12CommandAllocator> mCommandAllocatorCopy;

		ComPtr<ID3D12Fence> mFenceCopy;
		UINT64 mFenceValuesCopy;
		Wrappers::Event mFenceEventCopy;

		std::map<ER_RHI_SAMPLER_STATE, D3D12_SAMPLER_DESC> mSamplerStates;
		std::map<ER_RHI_BLEND_STATE, D3D12_BLEND_DESC> mBlendStates;
		std::map<ER_RHI_RASTERIZER_STATE, D3D12_RASTERIZER_DESC> mRasterizerStates;
		std::map<ER_RHI_DEPTH_STENCIL_STATE, D3D12_DEPTH_STENCIL_DESC> mDepthStates;

		std::map<std::string, ER_RHI_DX12_GraphicsPSO> mGraphicsPSONames;
		std::map<std::string, ER_RHI_DX12_ComputePSO> mComputePSONames;
		std::string mCurrentGraphicsPSOName;
		std::string mCurrentComputePSOName;
		std::string mCurrentSetGraphicsPSOName; //which was set to command list already
		std::string mCurrentSetComputePSOName; //which was set to command list already
		ER_RHI_DX12_PSO_STATE mCurrentPSOState = ER_RHI_DX12_PSO_STATE::UNSET;
#if defined(_DEBUG) || defined (DEBUG)
		UINT mCurrentPSOSwitchesCount = 0; // to debug how many times we switch our PSOs per frame
#endif

		ER_RHI_DX12_GPUDescriptorHeapManager* mDescriptorHeapManager = nullptr;

		ComPtr<ID3D12CommandSignature> mCommandSignature_DrawIndexed;

		D3D12_SAMPLER_DESC mEmptySampler;

		bool mIsRaytracingTierAvailable = false;
		bool mIsContextReadingBuffer = false;
		bool mIsInstancedBufferBound = false;

		ER_RHI_GPURootSignature* mClearUAV2DRS = nullptr;
		ER_RHI_GPUShader* mClearUAV2DCS = nullptr;
		std::string mClearUAV2DPSOName = "ER_RHI_GPUPipelineStateObject: Clear UAV 2D";

		ER_RHI_GPURootSignature* mClearUAV3DRS = nullptr;
		ER_RHI_GPUShader* mClearUAV3DCS = nullptr;
		std::string mClearUAV3DPSOName = "ER_RHI_GPUPipelineStateObject: Clear UAV 3D";

		ER_RHI_GPURootSignature* mGenerateMips2DRS = nullptr;
		ER_RHI_GPUShader* mGenerateMips2DCS = nullptr;
		std::string mGenerateMips2DPSOName = "ER_RHI_GPUPipelineStateObject: Generate Mips 2D";

		ER_RHI_GPURootSignature* mGenerateMips3DRS = nullptr;
		ER_RHI_GPUShader* mGenerateMips3DCS = nullptr;
		std::string mGenerateMips3DPSOName = "ER_RHI_GPUPipelineStateObject: Generate Mips 3D";

		ER_RHI_GPUTexture* mGenerateMipsWithReplacementReadyTexturesPool[DX12_MAX_GENERATE_MIPS_TEXTURES_IN_POOL] = { nullptr };
		std::function<void(ER_RHI_GPUTexture**)> mGenerateMipsWithReplacementCallbacks[DX12_MAX_GENERATE_MIPS_TEXTURES_IN_POOL];
		int mGenerateMipsWithReplacementCurrentTextureIndexInPool = 0; // should be atomic (in the future), at the moment we do not use multithreading for submitting mip generation commands
	};
}