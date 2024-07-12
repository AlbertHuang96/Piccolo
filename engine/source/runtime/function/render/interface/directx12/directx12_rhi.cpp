#include "runtime/function/render/interface/directx12/directx12_rhi.h"
//#include "runtime/function/render/interface/vulkan/vulkan_util.h"

#include "runtime/function/render/window_system.h"
#include "runtime/core/base/macro.h"

#define GLFW_EXPOSE_NATIVE_WIN32 1 //暴露出win32相关的原生接口
#include "GLFW/glfw3native.h"

#include <algorithm>
#include <cmath>

#if defined(__linux__)
#include <stdlib.h>
#elif defined(__MACH__)
// https://developer.apple.com/library/archive/documentation/Porting/Conceptual/PortingUnix/compiling/compiling.html
#include <stdlib.h>
#else
//#error Unknown Platform
#endif
//#elif defined(_MSC_VER)
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN 1
#define NOGDICAPMASKS 1
#define NOVIRTUALKEYCODES 1
#define NOWINMESSAGES 1
#define NOWINSTYLES 1
#define NOSYSMETRICS 1
#define NOMENUS 1
#define NOICONS 1
#define NOKEYSTATES 1
#define NOSYSCOMMANDS 1
#define NORASTEROPS 1
#define NOSHOWWINDOW 1
#define NOATOM 1
#define NOCLIPBOARD 1
#define NOCOLOR 1
#define NOCTLMGR 1
#define NODRAWTEXT 1
#define NOGDI 1
#define NOKERNEL 1
#define NOUSER 1
#define NONLS 1
#define NOMB 1
#define NOMEMMGR 1
#define NOMETAFILE 1
#define NOMINMAX 1
#define NOMSG 1
#define NOOPENFILE 1
#define NOSCROLL 1
#define NOSERVICE 1
#define NOSOUND 1
#define NOTEXTMETRIC 1
#define NOWH 1
#define NOWINOFFSETS 1
#define NOCOMM 1
#define NOKANJI 1
#define NOHELP 1
#define NOPROFILER 1
#define NODEFERWINDOWPOS 1
#define NOMCX 1
#include <Windows.h>
//#else
//#error Unknown Compiler
//#endif

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include "d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }

private:
    const HRESULT m_hr;
};

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

namespace Piccolo
{
    DirectX12RHI::~DirectX12RHI()
    {

    }

    void DirectX12RHI::initialize(RHIInitInfo init_info)
    {
        m_window = init_info.window_system->getWindow();
        mHWnd = glfwGetWin32Window(m_window);

        std::array<int, 2> window_size = init_info.window_system->getWindowSize();

        m_viewport = {0.0f, 0.0f, (float)window_size[0], (float)window_size[1], 0.0f, 1.0f};
        m_scissor  = {{0, 0}, {(uint32_t)window_size[0], (uint32_t)window_size[1]}};

        #if defined(_DEBUG)
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
            }
        }
#endif 
    }

    IDXGIAdapter1* DirectX12RHI::getSupportedAdapter(ComPtr<IDXGIFactory4>&  dxgiFactory,
                                                   const D3D_FEATURE_LEVEL featureLevel)
    {
        IDXGIAdapter1* adapter = nullptr;
        for (std::uint32_t adapterIndex = 0U;; ++adapterIndex)
        {
            IDXGIAdapter1* currentAdapter = nullptr;
            if (DXGI_ERROR_NOT_FOUND == dxgiFactory->EnumAdapters1(adapterIndex, &currentAdapter))
            {
                break;
            }

            const HRESULT hres = D3D12CreateDevice(currentAdapter, featureLevel, _uuidof(ID3D12Device), nullptr);
            if (SUCCEEDED(hres))
            {
                adapter = currentAdapter;
                break;
            }

            currentAdapter->Release();
        }

        return adapter;
    }

    // command context  command list mgr
    void DirectX12RHI::prepareContext()
    {

    }

    void DirectX12RHI::createCommandPool()
    {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    }

    bool DirectX12RHI::allocateCommandBuffers(const RHICommandBufferAllocateInfo* pAllocateInfo, RHICommandBuffer*& pCommandBuffers)
    {
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    }

    void DirectX12RHI::createDevice()
    {
        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(mDxgiFactory.GetAddressOf())));

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        IDXGIAdapter1* adapter = nullptr;
        for (std::uint32_t i = 0U; i < _countof(featureLevels); ++i)
        {
            adapter = getSupportedAdapter(mDxgiFactory, featureLevels[i]);
            if (adapter != nullptr)
            {
                break;
            }
        }

        if (adapter != nullptr)
        {
            D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf()));
        }

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        
        ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mGraphicsCmdQueue)));
        mGraphicsQueue = new DX3D12CommandQueue();
        ((DX3D12CommandQueue*)mGraphicsQueue)->setResource(mGraphicsCmdQueue.Get());

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc2 = {};
        commandQueueDesc2.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        commandQueueDesc2.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc2, IID_PPV_ARGS(&mComputeCmdQueue)));
        mComputeCmdQueue = new DX3D12CommandQueue();
        ((DX3D12CommandQueue*)mComputeQueue)->setResource(mComputeCmdQueue.Get());

    }

    void DirectX12RHI::createWindowSurface()
    {
        /*WNDCLASSEX windowClass    = {0};
        windowClass.cbSize        = sizeof(WNDCLASSEX);
        windowClass.style         = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc   = WindowProc;
        windowClass.hInstance     = hInstance;
        windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        windowClass.lpszClassName = L"WinDX12Render";

        RegisterClassEx(&windowClass);

        hwnd = CreateWindow(windowClass.lpszClassName,
                            L"YSHRender",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            width,
                            height,
                            nullptr,
                            nullptr,
                            hInstance,
                            nullptr);*/
    }

    void DirectX12RHI::createSwapchain()
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount           = k_max_frames_in_flight;
        swapChainDesc.Width                 = mWidth;
        swapChainDesc.Height                = mHeight;
        swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count      = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(mDxgiFactory->CreateSwapChainForHwnd(
            commandQueue.Get(), mHWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

        ThrowIfFailed(swapChain1.As(&m_swapchain));
        m_current_frame_index = m_swapchain->GetCurrentBackBufferIndex();
    }

    // cmd buffer = dx12 cmd list
    RHICommandBuffer* DirectX12RHI::getCurrentCommandBuffer() const 
    { 
        return m_current_command_buffer; 
    }

    RHICommandBuffer* const* DirectX12RHI::getCommandBufferList() const
    {
        return m_command_buffers;
    }

    RHICommandPool* DirectX12RHI::getCommandPoor() const 
    { 
        return m_rhi_command_pool; 
    }
    RHIDescriptorPool* DirectX12RHI::getDescriptorPoor() const 
    { 
        return m_descriptor_pool; 
    }
    RHIFence* const*   DirectX12RHI::getFenceList() const 
    { 
        return m_rhi_is_frame_in_flight_fences; 
    }
    QueueFamilyIndices DirectX12RHI::getQueueFamilyIndices() const 
    { 
        return m_queue_indices; 
    }
    RHIQueue* DirectX12RHI::getGraphicsQueue() const 
    { 
        return mGraphicsQueue;
    }
    RHIQueue* DirectX12RHI::getComputeQueue() const 
    { 
        return mComputeQueue;
    }

    void DirectX12RHI::createFramebufferImageAndView()
    {
        // depth image view
    }

    void DirectX12RHI::createSwapchainImageViews()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT n = 0; n < k_max_frames_in_flight; n++)
        {
            ThrowIfFailed(m_swapchain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
            device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize);
        }
    }

    void DirectX12RHI::cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports)
    {
        D3D12_VIEWPORT Viewport;
        commandList->RSSetViewports(1, &Viewport);
    }
    void DirectX12RHI::cmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors)
    {
        D3D12_RECT Rect;
        commandList->RSSetScissorRects(1, &Rect);
    }

    void DirectX12RHI::cmdBindIndexBufferPFN(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset, RHIIndexType indexType)
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        //ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
        //ibv.Format = DXGI_FORMAT_R16_UINT;
        //ibv.SizeInBytes = ibByteSize;
        
        commandList->IASetIndexBuffer(&ibv);

    }

    void DirectX12RHI::cmdBindVertexBuffersPFN(RHICommandBuffer* commandBuffer,
        uint32_t firstBinding,
        uint32_t bindingCount,
        RHIBuffer* const* pBuffers,
        const RHIDeviceSize* pOffsets)
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        //vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
        //vbv.SizeInBytes = sizeof(Vertex) * 8;	//顶点缓冲区大小（所有顶点数据大小）
        //vbv.StrideInBytes = sizeof(Vertex);	//每个顶点元素所占用的字节数
        //设置顶点缓冲区
        commandList->IASetVertexBuffers(0, 1, &vbv);

    }

    void DirectX12RHI::createDescriptorHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors             = k_max_frames_in_flight;
        rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    bool DirectX12RHI::createSampler(const RHISamplerCreateInfo* pCreateInfo, RHISampler*& pSampler)
    {
        D3D12_SAMPLER_DESC SamplerDesc{};

        SamplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
        SamplerDesc.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)pCreateInfo->addressModeU; //D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        SamplerDesc.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)pCreateInfo->addressModeV;
        SamplerDesc.AddressW = (D3D12_TEXTURE_ADDRESS_MODE)pCreateInfo->addressModeW;
        SamplerDesc.MipLODBias = pCreateInfo->mipLodBias; //0.0f;
        SamplerDesc.MaxAnisotropy = pCreateInfo->maxAnisotropy; //16;
        SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)pCreateInfo->compareOp; //D3D12_COMPARISON_FUNC_LESS_EQUAL;
        SamplerDesc.BorderColor[0] = 1.0f;
        SamplerDesc.BorderColor[1] = 1.0f;
        SamplerDesc.BorderColor[2] = 1.0f;
        SamplerDesc.BorderColor[3] = 1.0f;
        SamplerDesc.MinLOD = pCreateInfo->minLod; //0.0f;
        SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX; //pCreateInfo->maxLod;

        pSampler = new DX12Sampler();
        D3D12_CPU_DESCRIPTOR_HANDLE dx12Sampler;// = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        device->CreateSampler(&SamplerDesc, dx12Sampler);
        ((DX12Sampler*)pSampler)->setResource(dx12Sampler);

    }

    uint8_t DirectX12RHI::getMaxFramesInFlight() const 
    { 
        return k_max_frames_in_flight; 
    }
    uint8_t DirectX12RHI::getCurrentFrameIndex() const 
    { 
        return m_current_frame_index; 
    }
    void    DirectX12RHI::setCurrentFrameIndex(uint8_t index) 
    { 
        m_current_frame_index = index; 
    }

    bool DirectX12RHI::createBufferVMA(VmaAllocator                   allocator,
        const RHIBufferCreateInfo* pBufferCreateInfo,
        const VmaAllocationCreateInfo* pAllocationCreateInfo,
        RHIBuffer*& pBuffer,
        VmaAllocation* pAllocation,
        VmaAllocationInfo* pAllocationInfo)
    {

    }
    bool DirectX12RHI::createBufferWithAlignmentVMA(VmaAllocator                   allocator,
        const RHIBufferCreateInfo* pBufferCreateInfo,
        const VmaAllocationCreateInfo* pAllocationCreateInfo,
        RHIDeviceSize                  minAlignment,
        RHIBuffer*& pBuffer,
        VmaAllocation* pAllocation,
        VmaAllocationInfo* pAllocationInfo)
    {

    }
}