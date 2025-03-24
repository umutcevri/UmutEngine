#pragma once

#include <vector>
#include <deque>
#include <functional>

#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "VkBootstrap.h"

#include "CommonTypes.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>

const int MAX_TEXTURE_COUNT = 256;

const int MAX_ENTITIES = 1000;

const int MAX_ANIMATED_ENTITIES = 100;

const int MAX_FRAMES = 2;

struct SDL_Window;

class URenderer {
private:
    struct DeletionQueue
    {
        std::deque<std::function<void()>> deletors;

        void push_function(std::function<void()>&& function)
        {
            deletors.push_back(function);
        }

        void flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }

            deletors.clear();
        }
    };

    struct DrawQueue
    {
        std::deque<std::function<void()>> drawCalls;

        void push_function(std::function<void()>&& function)
        {
            drawCalls.push_back(function);
        }

        void flush()
        {
            for (auto it = drawCalls.rbegin(); it != drawCalls.rend(); it++)
            {
                (*it)();
            }
            drawCalls.clear();
        };

    };

    struct AllocatedBuffer {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    struct FrameData
    {
        VkCommandBuffer commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence renderFence;
    };

    struct GPUPushConstants
    {
        glm::mat4 transform;
        glm::mat4 lightSpaceMatrix;
    };

    struct ShadowData
    {
        glm::mat4 lightSpaceMatrix;
        glm::mat4 model;
    };

    struct SceneData
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model;
        glm::mat4 lightSpaceMatrix;
        glm::vec4 lightPos;
    };

    DeletionQueue deletionQueue;

    vkb::Instance vkb_instance;

    vkb::Device vkb_device;

    vkb::PhysicalDevice phys_device;

    VkQueue graphicsQueue;

    VkQueue presentQueue;

    VkSurfaceKHR surface;

    VmaAllocator allocator;

    vkb::Swapchain vkb_swapchain;

    VkExtent2D swapChainExtent;

    VkSurfaceFormatKHR swapChainSurfaceFormat;

    std::vector<VkImage> swapChainImages;

    std::vector<VkImageView> swapChainImageViews;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;

    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorPool descriptorPool;

    std::vector<VkDescriptorSet> descriptorSets = std::vector<VkDescriptorSet>(MAX_FRAMES);

    std::vector<VkDescriptorSet> shadowDescriptorSets = std::vector<VkDescriptorSet>(MAX_FRAMES);

    std::vector<VkDescriptorSet> debugQuadDescriptorSets = std::vector<VkDescriptorSet>(MAX_FRAMES);

    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;

    std::vector<FrameData> frames;

    uint32_t currentFrame = 0;

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

    AllocatedBuffer entityInstanceBuffer;

    AllocatedBuffer boneTransformBuffer;

    VkSampler textureSampler;

    VkImage depthImage;

    VkImageView depthImageView;

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkFormat shadowDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkRenderPass shadowRenderPass;

    std::vector<VkImageView> images;

    uint32_t shadowMapResolution = 16384;

    VkSampler shadowSampler;

    VkFramebuffer shadowFramebuffer;

    VkPipeline shadowPipeline;

    VkPipelineLayout shadowPipelineLayout;

    VkDescriptorSetLayout shadowDescriptorSetLayout;

    VkPipeline debugQuadPipeline;

    VkPipelineLayout debugQuadPipelineLayout;

    VkDescriptorSetLayout debugQuadDescriptorSetLayout;

    VkImage shadowImage;

    VkImageView shadowImageView;

    AllocatedBuffer shadowUniformBuffer;

    AllocatedBuffer sceneDataUniformBuffer;

    VkDescriptorPool shadowDescriptorPool;

    SDL_Window* _window;

    AllocatedBuffer entityInstanceStaging;

	AllocatedBuffer boneTransformStaging;

    size_t entityInstanceBufferSize;

    size_t boneTransformBufferSize;

	float deltaTime = 0.0f;

	class Camera* defaultCamera;

public:
    bool renderDebugQuad = false;

	int cameraIndex = 0;

    void Init();

	void Draw(float deltaTime);

	void Cleanup();

    void SetWindow(SDL_Window* window);

private:

    void InitVulkan();

    void OneTimeSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    void LoadAssets();

    void CreateDepthResources();

    void CreateTextureImage(std::string texturePath);

    void CreateTextureSampler();

    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image);

    void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    void CreateSwapChain();

    void CreateRenderPass();

    void CreateShadowRenderPass();

    void CreateDescriptorSetLayout();

    void CreateShadowDescriptorSetLayout();

    void CreateDebugQuadDescriptorSetLayout();

    void CreateGraphicsPipeline();

    void CreateDebugQuadPipeline();

    void CreateShadowPipeline();

    void CreateFrameBuffers();

    void CreateShadowFrameBuffer();

    void RecreateSwapChain();

    void CreateCommandPool();

    void CreateDescriptorPool();

    void CreateShadowDescriptorPool();

    void CreateDescriptorSets();

    void CreateShadowDescriptorSets();

    void CreateDebugQuadDescriptorSets();

    void CreateCommandBuffer();

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void CreateSyncPrimitives();

    std::vector<uint32_t> CompileGLSLtoSPV(const std::string& sourceCode, EShLanguage shaderType);

    VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirvCode);
};
