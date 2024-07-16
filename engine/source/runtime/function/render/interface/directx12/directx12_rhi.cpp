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
        mWindow = init_info.window_system->getWindow();
        mHWnd = glfwGetWin32Window(mWindow);

        std::array<int, 2> window_size = init_info.window_system->getWindowSize();

        mViewport = {0.0f, 0.0f, (float)window_size[0], (float)window_size[1], 0.0f, 1.0f};
        mScissor = {{0, 0}, {(uint32_t)window_size[0], (uint32_t)window_size[1]}};

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

    void DirectX12RHI::resetCommandPool()
    {
        ThrowIfFailed(commandAllocator->Reset());
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
        swapChainDesc.BufferCount           = kMaxFramesInFlight;
        swapChainDesc.Width                 = mWidth;
        swapChainDesc.Height                = mHeight;
        swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count      = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(mDxgiFactory->CreateSwapChainForHwnd(
            commandQueue.Get(), mHWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

        ThrowIfFailed(swapChain1.As(&mSwapchain));
        mCurrentFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
    }

    void DirectX12RHI::recreateSwapchain()
    {
        for (int i = 0; i < kMaxFramesInFlight; ++i)
            frameBuffers[i].Reset();
        depthStencilBuffer.Reset();

        // 根据窗口大小重新分配缓冲区大小
        ThrowIfFailed(mSwapchain->ResizeBuffers(
            kMaxFramesInFlight, 
            mWidth, mHeight, 
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    }

    bool DirectX12RHI::createDescriptorPool(const RHIDescriptorPoolCreateInfo* pCreateInfo, RHIDescriptorPool*& pDescriptorPool)
    {

    }

    bool DirectX12RHI::allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* pAllocateInfo, RHIDescriptorSet*& pDescriptorSets)
    {

    }

    bool DirectX12RHI::beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        ThrowIfFailed(commandAllocator->Reset());

        // 命令列表在提交之后就可以重置了，不需要等到命令执行完。
        // 重用命令列表的内存块。
        ThrowIfFailed(((DX12GraphicsCommandList*)commandBuffer)->getResource()->Reset(commandAllocator.Get(), nullptr));

        // 把后缓冲区的资源状态切换成Render Target。
            // ResourceBarrier函数创建了一个通知驱动同步资源访问性的命令，简单地来说就是切换资源的状态。
        commandList->ResourceBarrier(1,
            // D3D12_RESOURCE_BARRIER是资源栅栏（暂时这么翻译），用来表示对资源的操作。
            // CD3DX12_RESOURCE_BARRIER类是D3D12_RESOURCE_BARRIER结构的辅助类，提供更便利的
            // 使用接口。Transition的作用正如其函数名一样，创建资源状态转换的操作，其返回值
            // 是CD3DX12_RESOURCE_BARRIER 。
            &CD3DX12_RESOURCE_BARRIER::Transition(frameBuffers[mCurrentFrameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET));
    }

    bool DirectX12RHI::endCommandBuffer(RHICommandBuffer* commandBuffer)
    {
        ThrowIfFailed(((DX12GraphicsCommandList*)commandBuffer)->getResource()->Close());
    }

    RHICommandBuffer* DirectX12RHI::beginSingleTimeCommands()
    {
        ComPtr<ID3D12GraphicsCommandList> oneTimeCommandList;
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&oneTimeCommandList)));

        ThrowIfFailed(commandAllocator->Reset());

        ThrowIfFailed(oneTimeCommandList->Reset(commandAllocator.Get(), nullptr));

        // 把后缓冲区的资源状态切换成Render Target。
            // ResourceBarrier函数创建了一个通知驱动同步资源访问性的命令，简单地来说就是切换资源的状态。
        oneTimeCommandList->ResourceBarrier(1,
            // D3D12_RESOURCE_BARRIER是资源栅栏（暂时这么翻译），用来表示对资源的操作。
            // CD3DX12_RESOURCE_BARRIER类是D3D12_RESOURCE_BARRIER结构的辅助类，提供更便利的
            // 使用接口。Transition的作用正如其函数名一样，创建资源状态转换的操作，其返回值
            // 是CD3DX12_RESOURCE_BARRIER 。
            &CD3DX12_RESOURCE_BARRIER::Transition(frameBuffers[mCurrentFrameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        RHICommandBuffer* commandBuffer = new DX12GraphicsCommandList();
        ((DX12GraphicsCommandList*)commandBuffer)->setResource(oneTimeCommandList.Get());
        return commandBuffer;
    }

    // need to sync queue wait vkQueueWaitIdle
    void DirectX12RHI::endSingleTimeCommands(RHICommandBuffer* commandBuffer)
    {
        ThrowIfFailed(((DX12GraphicsCommandList*)commandBuffer)->getResource()->Close());
        
        
        ID3D12CommandList* cmdLists[] = { ((DX12GraphicsCommandList*)commandBuffer)->getResource() };
        commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

        /*VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vk_command_buffer;

        vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(((VulkanQueue*)m_graphics_queue)->getResource());

        vkFreeCommandBuffers(m_device, ((VulkanCommandPool*)m_rhi_command_pool)->getResource(), 1, &vk_command_buffer);*/
        //delete(command_buffer);

        delete(commandBuffer);
    }

    bool DirectX12RHI::createCommandPool(const RHICommandPoolCreateInfo* pCreateInfo, RHICommandPool*& pCommandPool)
    {
        //((DX12CommandAllocator*)pCommandPool)->getResource()
        ID3D12CommandAllocator* addr = ((DX12CommandAllocator*)pCommandPool)->getResource();
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&addr)));
    }

    // cmd buffer = dx12 cmd list
    RHICommandBuffer* DirectX12RHI::getCurrentCommandBuffer() const 
    { 
        return mCurrentCommandBuffer; 
    }

    RHICommandBuffer* const* DirectX12RHI::getCommandBufferList() const
    {
        return mCommandBuffers;
    }

    RHICommandPool* DirectX12RHI::getCommandPoor() const 
    { 
        return mRhiCommandPool; 
    }

    RHIDescriptorPool* DirectX12RHI::getDescriptorPoor() const 
    { 
        return mDescriptorPool; 
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

    bool DirectX12RHI::queueSubmit(RHIQueue* queue, uint32_t submitCount, const RHISubmitInfo* pSubmits, RHIFence* fence)
    {
        ThrowIfFailed(commandList->Close());
        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        ((DX3D12CommandQueue*)queue)->getResource()->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        //queue->getResource->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    }

    bool DirectX12RHI::createFence(const RHIFenceCreateInfo* pCreateInfo, RHIFence*& pFence)
    {

        //ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    }

    void DirectX12RHI::createFramebufferImageAndView()
    {
        // = DirectX12RHI::createSwapchainImageViews()
        // rtvs and dsv
    }

    bool DirectX12RHI::createFramebuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer)
    {
        //ComPtr<ID3D12Resource>
    }

    void DirectX12RHI::createSwapchainImageViews()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT n = 0; n < kMaxFramesInFlight; n++)
        {
            ThrowIfFailed(mSwapchain->GetBuffer(n, IID_PPV_ARGS(&frameBuffers[n])));
            device->CreateRenderTargetView(frameBuffers[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize);
        }

        D3D12_RESOURCE_DESC depthStencilDesc;
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 缓冲区维数（1维纹理还是2维纹理？）
        depthStencilDesc.Alignment = 0; // 对齐
        depthStencilDesc.Width = mWidth; 
        depthStencilDesc.Height = mHeight; 
        depthStencilDesc.DepthOrArraySize = 1; // 纹理的深度，以纹素为单位；或者是纹理数组尺寸
        depthStencilDesc.MipLevels = 1; 
        depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // 纹素格式
        depthStencilDesc.SampleDesc.Count = 1; 
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 纹理布局，暂时不用管。
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 其他资源标志。允许深度/模板视图作为资源创建，
        // 并且允许资源转换成D3D12_RESOURCE_STATE_DEPTH_WRITE或D3D12_RESOURCE_STATE_DEPTH_READ状态

        D3D12_CLEAR_VALUE optClear;
        optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 缓存数据格式
        optClear.DepthStencil.Depth = 1.0f; 
        optClear.DepthStencil.Stencil = 0; 
        // 创建，提交一个资源到特定的堆，这个资源属性是我们指定的。
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // 特定的堆的属性。这里表示堆是被GPU访问的。
            D3D12_HEAP_FLAG_NONE, // 堆选项，指明堆是否包含纹理，资源是否被多个适配器共享。NONE表示不需要额外的功能。
            &depthStencilDesc, // 资源描述符。生成这种资源。
            D3D12_RESOURCE_STATE_COMMON, // 资源的初始状态。要如何使用这个资源？
            // 这个枚举的官方解释是要跨越图形引擎类型访问资源的话必须转换到这个状态。但是并没有给出详细的说明。
            // 还指出了，纹理必须是COMMON状态，为了让CPU进行访问。这里是否跟第一个参数冲突？
            &optClear, // 清除值
            IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));

        //创建DSV(必须填充DSV属性结构体，和创建RTV不同，RTV是通过句柄)
    //D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    //dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    //dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    //dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    //dsvDesc.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(depthStencilBuffer.Get(),
            nullptr,	//D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
            //由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
            dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV句柄
    }

    void DirectX12RHI::cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports)
    {
        D3D12_VIEWPORT Viewport;
        Viewport.Width = pViewports->width;
        Viewport.Height = pViewports->height;
        Viewport.MinDepth = pViewports->minDepth;
        Viewport.MaxDepth = pViewports->maxDepth;
        Viewport.TopLeftX = pViewports->x;
        Viewport.TopLeftY = pViewports->y;
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
        // rtv heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors             = kMaxFramesInFlight;
        rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // dsv heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; 
        dsvHeapDesc.NodeMask = 0;
        
        ThrowIfFailed(device->CreateDescriptorHeap(
            &dsvHeapDesc,
            IID_PPV_ARGS(dsvHeap.GetAddressOf())));

        dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    void DirectX12RHI::createBuffer(RHIDeviceSize size, RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory)
    {
        //UINT64 RHIDeviceSize
        ComPtr<ID3D12Resource> uploadBuffer = nullptr;
            //创建上传堆，作用是：写入CPU内存数据，并传输给默认堆
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), //创建上传堆类型的堆
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(size),//变体的构造函数，传入byteSize，其他均为默认值，简化书写
                D3D12_RESOURCE_STATE_GENERIC_READ,	//上传堆里的资源需要复制给默认堆，所以是可读状态
                nullptr,	//不是深度模板资源，不用指定优化值
                IID_PPV_ARGS(&uploadBuffer)));

        //创建默认堆，作为上传堆的数据传输对象
        //ComPtr<ID3D12Resource> defaultBuffer;
        ComPtr<ID3D12Resource> defaultBuffer = ((DX12Buffer*)buffer)->GetResource();
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//创建默认堆类型的堆
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(size),
            D3D12_RESOURCE_STATE_COMMON,//默认堆为最终存储数据的地方，所以暂时初始化为普通状态
            nullptr,
            IID_PPV_ARGS(&defaultBuffer)));

        //将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
        commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST));

        //将数据从CPU内存拷贝到GPU缓存
        D3D12_SUBRESOURCE_DATA subResourceData;
        //subResourceData.pData = initData; // it is vertices/indexes data buffer
        subResourceData.RowPitch = size;
        subResourceData.SlicePitch = subResourceData.RowPitch;
        //核心函数UpdateSubresources，将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆。1是最大的子资源的下标（模板中定义，意为有2个子资源）
        UpdateSubresources<1>(commandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

        //再次将资源从COPY_DEST状态转换到GENERIC_READ状态(现在只提供给着色器访问)
        commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_GENERIC_READ));

        //return defaultBuffer;
    }

    void DirectX12RHI::createImage(uint32_t image_width, uint32_t image_height, RHIFormat format, RHIImageTiling image_tiling, RHIImageUsageFlags image_usage_flags, RHIMemoryPropertyFlags memory_property_flags,
        RHIImage*& image, RHIDeviceMemory*& memory, RHIImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels)
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Alignment = 0;
        Desc.DepthOrArraySize = (UINT16)array_layers;
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        //Desc.Flags = (D3D12_RESOURCE_FLAGS)Flags;
        //Desc.Format = GetBaseFormat(Format);
        Desc.Height = (UINT)image_height;
        Desc.Width = (UINT64)image_width;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Desc.MipLevels = (UINT16)miplevels;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;

        
        D3D12_CLEAR_VALUE ClearValue;
        CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
            &Desc, D3D12_RESOURCE_STATE_COMMON, &ClearValue, IID_PPV_ARGS(((DX12Image*)image)->GetAddressOf())));
        
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
        return kMaxFramesInFlight;
    }
    uint8_t DirectX12RHI::getCurrentFrameIndex() const 
    { 
        return mCurrentFrameIndex; 
    }
    void    DirectX12RHI::setCurrentFrameIndex(uint8_t index) 
    { 
        mCurrentFrameIndex = index;
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