#pragma once

#include "runtime/function/render/interface/rhi.h"

#include <optional>

namespace Piccolo
{
    class DX12Buffer : public RHIBuffer
    {
    public:
        void setResource(VkBuffer res)
        {
            m_resource = res;
        }
        ID3D12Resource* operator->() 
        { 
            return m_resource.Get();
        }
        const ID3D12Resource* operator->() const 
        { 
            return m_resource.Get(); 
        }

        ID3D12Resource* GetResource() 
        { 
            return m_resource.Get(); 
        }
        const ID3D12Resource* GetResource() const 
        { 
            return m_resource.Get(); 
        }

        ID3D12Resource** GetAddressOf() 
        { 
            return m_resource.GetAddressOf(); 
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const 
        { 
            return m_GpuVirtualAddress; 
        }

        uint32_t GetVersionID() const { return m_VersionID; }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
        D3D12_RESOURCE_STATES m_UsageState;
        D3D12_RESOURCE_STATES m_TransitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

        // Used to identify when a resource changes so descriptors can be copied etc.
        uint32_t m_VersionID = 0;
    };

    class VulkanBufferView : public RHIBufferView
    {
    public:
        void setResource(VkBufferView res)
        {
            m_resource = res;
        }
        VkBufferView getResource() const
        {
            return m_resource;
        }
    private:
        VkBufferView m_resource;
    };

    class DX12GraphicsCommandList : public RHICommandBuffer
    {
    public:
        void setResource(ID3D12GraphicsCommandList* res)
        {
            m_resource = res;
        }
        const ID3D12GraphicsCommandList* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12GraphicsCommandList> m_resource;
    };

    class DX12CommandAllocator : public RHICommandPool
    {
    public:
        void setResource(ID3D12CommandAllocator* res)
        {
            m_resource = res;
        }
        ID3D12CommandAllocator* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12CommandAllocator> m_resource;
    };

    class DX12DescriptorHeap : public RHIDescriptorPool
    {
    public:
        void setResource(ID3D12DescriptorHeap* res)
        {
            m_resource = res;
        }
        ID3D12DescriptorHeap* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12DescriptorHeap> m_resource;
    };

    class VulkanDescriptorSet : public RHIDescriptorSet
    {
    public:
        void setResource(VkDescriptorSet res)
        {
            m_resource = res;
        }
        VkDescriptorSet getResource() const
        {
            return m_resource;
        }
    private:
        VkDescriptorSet m_resource;
    };

    class VulkanDescriptorSetLayout : public RHIDescriptorSetLayout
    {
    public:
        void setResource(VkDescriptorSetLayout res)
        {
            m_resource = res;
        }
        VkDescriptorSetLayout getResource() const
        {
            return m_resource;
        }
    private:
        VkDescriptorSetLayout m_resource;
    };

    class DX12Device : public RHIDevice
    {
    public:
        void setResource(ID3D12Device* res)
        {
            m_resource = res;
        }
        ID3D12Device* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12Device> m_resource;
    };

    class VulkanDeviceMemory : public RHIDeviceMemory
    {
    public:
        void setResource(VkDeviceMemory res)
        {
            m_resource = res;
        }
        VkDeviceMemory getResource() const
        {
            return m_resource;
        }
    private:
        VkDeviceMemory m_resource;
    };

    class VulkanEvent : public RHIEvent
    {
    public:
        void setResource(VkEvent res)
        {
            m_resource = res;
        }
        VkEvent getResource() const
        {
            return m_resource;
        }
    private:
        VkEvent m_resource;
    };

    class DX12Fence : public RHIFence
    {
    public:
        void setResource(ID3D12Fence* res)
        {
            m_resource = res;
        }
        ID3D12Fence* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12Fence> m_resource;
    };

    class VulkanFramebuffer : public RHIFramebuffer
    {
    public:
        void setResource(VkFramebuffer res)
        {
            m_resource = res;
        }
        VkFramebuffer getResource() const
        {
            return m_resource;
        }
    private:
        VkFramebuffer m_resource;
    };

    class VulkanImage : public RHIImage
    {
    public:
        void setResource(VkImage res)
        {
            m_resource = res;
        }
        VkImage &getResource()
        {
            return m_resource;
        }
    private:
        VkImage m_resource;
    };
    class VulkanImageView : public RHIImageView
    {
    public:
        void setResource(VkImageView res)
        {
            m_resource = res;
        }
        VkImageView getResource() const
        {
            return m_resource;
        }
    private:
        VkImageView m_resource;
    };
    class VulkanInstance : public RHIInstance
    {
    public:
        void setResource(VkInstance res)
        {
            m_resource = res;
        }
        VkInstance getResource() const
        {
            return m_resource;
        }
    private:
        VkInstance m_resource;
    };

    class DX3D12CommandQueue : public RHIQueue
    {
    public:
        void setResource(ID3D12CommandQueue* res)
        {
            m_resource = res;
        }
        ID3D12CommandQueue* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12CommandQueue> m_resource;
    };

    class VulkanPhysicalDevice : public RHIPhysicalDevice
    {
    public:
        void setResource(VkPhysicalDevice res)
        {
            m_resource = res;
        }
        VkPhysicalDevice getResource() const
        {
            return m_resource;
        }
    private:
        VkPhysicalDevice m_resource;
    };

    class DX12Pipeline : public RHIPipeline
    {
    public:
        void setResource(ID3D12PipelineState* res)
        {
            m_resource = res;
        }
        ID3D12PipelineState* getResource() const
        {
            return m_resource.Get();
        }
    private:
        ComPtr<ID3D12PipelineState> m_resource;
    };

    class VulkanPipelineCache : public RHIPipelineCache
    {
    public:
        void setResource(VkPipelineCache res)
        {
            m_resource = res;
        }
        VkPipelineCache getResource() const
        {
            return m_resource;
        }
    private:
        VkPipelineCache m_resource;
    };
    class VulkanPipelineLayout : public RHIPipelineLayout
    {
    public:
        void setResource(VkPipelineLayout res)
        {
            m_resource = res;
        }
        VkPipelineLayout getResource() const
        {
            return m_resource;
        }
    private:
        VkPipelineLayout m_resource;
    };
    class VulkanRenderPass : public RHIRenderPass
    {
    public:
        void setResource(VkRenderPass res)
        {
            m_resource = res;
        }
        VkRenderPass getResource() const
        {
            return m_resource;
        }
    private:
        VkRenderPass m_resource;
    };

    class DX12Sampler : public RHISampler
    {
    public:
        void setResource(D3D12_CPU_DESCRIPTOR_HANDLE res)
        {
            m_resource = res;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE getResource() const
        {
            return m_resource;
        }
    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_resource;
    };

    class VulkanSemaphore : public RHISemaphore
    {
    public:
        void setResource(VkSemaphore res)
        {
            m_resource = res;
        }
        VkSemaphore &getResource()
        {
            return m_resource;
        }
    private:
        VkSemaphore m_resource;
    };
    class VulkanShader : public RHIShader
    {
    public:
        void setResource(VkShaderModule res)
        {
            m_resource = res;
        }
        VkShaderModule getResource() const
        {
            return m_resource;
        }
    private:
        VkShaderModule m_resource;
    };
}