#pragma once

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

#include "CommonTypes.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <VkBootstrap.h>

#include <shaderc/shaderc.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "AssetLoader.h"



#include "Camera.h"

#include <ktx.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>

const int MAX_TEXTURE_COUNT = 256;

const int MAX_OBJECTS = 100;

const int MAX_FRAMES = 2;

bool renderDebugQuad = false;

Camera camera;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void KeyboardInput()
{
    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    // adjust accordingly
    if (keystate[SDL_SCANCODE_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keystate[SDL_SCANCODE_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keystate[SDL_SCANCODE_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keystate[SDL_SCANCODE_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);

}

void MouseInput()
{
    int xPos, yPos;
    SDL_GetRelativeMouseState(&xPos, &yPos);

    camera.ProcessMouseMovement(xPos, -yPos);
}


class UEngine {
private:

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

    std::vector<FrameData> frames;

    uint32_t currentFrame = 0;

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

	AllocatedBuffer objectInstanceBuffer;

    VkSampler textureSampler;

    VkImage depthImage;
    
    VkImageView depthImageView;

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkFormat shadowDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkRenderPass shadowRenderPass;

    std::vector<VkImageView> images;

    uint32_t shadowMapResolution = 4096;

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

    AssetManager assetManager;

    VkExtent2D _windowExtent{ 1280 , 720 };

	SDL_Window* _window;

	bool bQuit = false;

    SDL_Event e;

public:
    void Run() {
        InitWindow();
        InitVulkan();

        CreateSwapChain();

        CreateRenderPass();
		
        CreateShadowRenderPass();

        CreateCommandPool();

        LoadAssets();

        CreateDepthResources();

        CreateTextureSampler();

        CreateDescriptorSetLayout();
        CreateShadowDescriptorSetLayout();
		CreateDebugQuadDescriptorSetLayout();

        CreateDescriptorPool();
        CreateShadowDescriptorPool();

        CreateShadowFrameBuffer();

        CreateDescriptorSets();
        CreateShadowDescriptorSets();
		CreateDebugQuadDescriptorSets();

        auto start = std::chrono::high_resolution_clock::now();

        CreateGraphicsPipeline();
        CreateDebugQuadPipeline();
        CreateShadowPipeline();

        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Pipeline creation time: " << duration << " milliseconds" << std::endl;

        glslang::FinalizeProcess();

        CreateFrameBuffers();

        CreateCommandBuffer();

        CreateSyncPrimitives();

        while (!bQuit)
        {
            MainLoop();
        }

        vkDeviceWaitIdle(vkb_device);
        
        Cleanup();
    }

private:
    void InitWindow()
    {
        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        _window = SDL_CreateWindow(
            "Vulkan Engine",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            _windowExtent.width,
            _windowExtent.height,
            window_flags);

        SDL_StartTextInput();

        camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f));

        glslang::InitializeProcess();
    }

    void InitVulkan() {

        vkb::InstanceBuilder instance_builder;
        auto instance_builder_return = instance_builder
            .request_validation_layers()
            .use_default_debug_messenger()
            .build();
        if (!instance_builder_return) {
            throw std::runtime_error("Failed to create instance!");
        }
        vkb_instance = instance_builder_return.value();

        deletionQueue.push_function([&]() {
            vkb::destroy_instance(vkb_instance);
            });

        if (SDL_Vulkan_CreateSurface(_window, vkb_instance.instance, &surface) != SDL_TRUE)
        {
            throw std::runtime_error("Failed to create surface!");
        }

        deletionQueue.push_function([&]() {
            vkb::destroy_surface(vkb_instance, surface);
            });

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        vkb::PhysicalDeviceSelector phys_device_selector(vkb_instance);
        auto physical_device_selector_return = phys_device_selector
            .set_required_features(deviceFeatures)
            .set_surface(surface)
            .select();
        if (!physical_device_selector_return) {
            throw std::runtime_error("Failed to select physical device!");
        }
        phys_device = physical_device_selector_return.value();

		std::cout << "Selected physical device: " << phys_device.name << std::endl;

        vkb::DeviceBuilder device_builder{ phys_device };
        auto dev_ret = device_builder.build();
        if (!dev_ret) {
            throw std::runtime_error("Failed to create logical device!");
        }
        vkb_device = dev_ret.value();

        deletionQueue.push_function([&]() {
            vkb::destroy_device(vkb_device);
            });       

        auto queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
        if (!queue_ret) {
            throw std::runtime_error("Failed to get graphics queue!");
        }
        graphicsQueue = queue_ret.value();

        queue_ret = vkb_device.get_queue(vkb::QueueType::present);
        if (!queue_ret) {
            throw std::runtime_error("Failed to get present queue!");
        }
        presentQueue = queue_ret.value();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = phys_device;
        allocatorInfo.device = vkb_device;
        allocatorInfo.instance = vkb_instance;
        //allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &allocator);

        deletionQueue.push_function([&]() {
            vmaDestroyAllocator(allocator);
            });
    }

    void OneTimeSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vkb_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        function(commandBuffer);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(vkb_device, commandPool, 1, &commandBuffer);
    }

    void LoadAssets()
    {
		//create cube asset
		Object cube;

		Mesh cubeMesh;

		assetManager.vertices = cubeVertices;
        assetManager.indices = cubeIndices;

		cubeMesh.startIndex = 0;
		cubeMesh.indexCount = static_cast<uint32_t>(assetManager.indices.size());

		cube.meshes.push_back(cubeMesh);

		assetManager.objectMap["Cube"] = 0;

		assetManager.objects.push_back(cube);

        //assetManager.CreateObjectInstance("Cube");

		//assetManager.CreateObjectInstance("Cube", glm::vec3(-0.5f, 1.0f, 0.0), glm::vec3(0), glm::vec3(0.5f));

		assetManager.CreateObjectInstance("Cube", glm::vec3(2.0f, 0.0f, 1.0), glm::vec3(0), glm::vec3(0.5f));

		assetManager.CreateObjectInstance("Cube", glm::vec3(-1.0f, 0.0f, 2.0), glm::vec3(60,0,60), glm::vec3(0.25f));

		assetManager.CreateObjectInstance("Cube", glm::vec3(0.0f, -1.0f, 0.0), glm::vec3(0), glm::vec3(10,1,10));

        //assetManager.LoadAsset("assets/Wolf.fbx", "Wolf");

		//assetManager.CreateObjectInstance("Wolf", glm::vec3(0.0f, 0.0f, 0.0), glm::vec3(0), glm::vec3(0.01f));

		//assetManager.PlayAnimation("Wolf", 0, 0);

		assetManager.LoadAsset("assets/Sword And Shield Run.dae", "Bot");

		assetManager.CreateObjectInstance("Bot");

		assetManager.PlayAnimation("Bot", 0, 0);

        for (size_t i = 0; i < assetManager.vertices.size(); ++i)
        {
            bool hasBone = false;

            // Check if any bone ID is not -1
            for (int j = 0; j < MAX_BONE_INFLUENCE; ++j)
            {
                if (assetManager.vertices[i].boneIDs[j] != -1)
                {
                    hasBone = true;
                    break;
                }
            }

            // Log a warning if the vertex has no valid bone ID
            if (!hasBone)
            {
                //std::cerr << "Warning: Vertex " << i << " has no bone IDs assigned (all -1)." << std::endl;
                // Optionally log additional info about the vertex
                // std::cerr << "Position: " << vertices[i].position.x << ", "
                //           << vertices[i].position.y << ", " << vertices[i].position.z << std::endl;
                continue; // Skip further checks for bone weights
            }

            // If there are bone IDs, check if all weights are zero
            bool hasWeight = false;
            for (int j = 0; j < MAX_BONE_INFLUENCE; ++j)
            {
                if (assetManager.vertices[i].boneWeights[j] > 0.0f)
                {
                    hasWeight = true;
                    break;
                }
            }

            // Log a warning if the vertex has no bone weights
            if (!hasWeight)
            {
                std::cerr << "Warning: Vertex " << i << " has bone IDs but no weights assigned." << std::endl;
                // Optionally log the vertex position for further inspection
                // std::cerr << "Position: " << vertices[i].position.x << ", "
                //           << vertices[i].position.y << ", " << vertices[i].position.z << std::endl;
                if (i > 0)
                {
                    assetManager.vertices[i].boneIDs = assetManager.vertices[i + 1].boneIDs;
                    assetManager.vertices[i].boneWeights = assetManager.vertices[i + 1].boneWeights;
                    std::cerr << "Assigned bone IDs and weights from Vertex " << (i + 1) << "." << std::endl;
                }
            }
        }

        assetManager.texturePaths.push_back("assets/image.jpg");

		const size_t objectInstanceBufferSize = MAX_OBJECTS * sizeof(ObjectInstance);
		objectInstanceBuffer = CreateBuffer(MAX_OBJECTS * objectInstanceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		deletionQueue.push_function([&]() {
			vmaDestroyBuffer(allocator, objectInstanceBuffer.buffer, objectInstanceBuffer.allocation);
			});

		//create vertex and index buffer
        const size_t vertexBufferSize = assetManager.vertices.size() * sizeof(Vertex);
        const size_t indexBufferSize = assetManager.indices.size() * sizeof(uint32_t);

        if (vertexBufferSize + indexBufferSize > 0)
        {
            vertexBuffer = CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

            indexBuffer = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

            deletionQueue.push_function([&]() {
                vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
                vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
                });
        }

        const size_t shadowUniformBufferSize = sizeof(ShadowData);

        shadowUniformBuffer = CreateBuffer(shadowUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        deletionQueue.push_function([&]() {
			vmaDestroyBuffer(allocator, shadowUniformBuffer.buffer, shadowUniformBuffer.allocation);
			});

        const size_t sceneDataUniformBufferSize = sizeof(SceneData);

        sceneDataUniformBuffer = CreateBuffer(sceneDataUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        deletionQueue.push_function([&]() {
            vmaDestroyBuffer(allocator, sceneDataUniformBuffer.buffer, sceneDataUniformBuffer.allocation);
            });


		//copy vertex and index data to GPU
        if (vertexBufferSize + indexBufferSize > 0)
        {
            AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

            void* data = staging.allocation->GetMappedData();

            // copy vertex buffer
            memcpy(data, assetManager.vertices.data(), vertexBufferSize);
            // copy index buffer
            memcpy((char*)data + vertexBufferSize, assetManager.indices.data(), indexBufferSize);


            // copy staging buffer to vertex and index buffer
            OneTimeSubmit([&](VkCommandBuffer cmd) {
                VkBufferCopy vertexCopy{ 0 };
                vertexCopy.dstOffset = 0;
                vertexCopy.srcOffset = 0;
                vertexCopy.size = vertexBufferSize;

                vkCmdCopyBuffer(cmd, staging.buffer, vertexBuffer.buffer, 1, &vertexCopy);

                VkBufferCopy indexCopy{ 0 };
                indexCopy.dstOffset = 0;
                indexCopy.srcOffset = vertexBufferSize;
                indexCopy.size = indexBufferSize;

                vkCmdCopyBuffer(cmd, staging.buffer, indexBuffer.buffer, 1, &indexCopy);
                });

            vmaDestroyBuffer(allocator, staging.buffer, staging.allocation);
        }

        for(std::string texturePath : assetManager.texturePaths)
		{
			CreateTextureImage(texturePath);
		}
        
    }

    void CreateDepthResources()
    {
    
		CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImage);

		depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        TransitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    void CreateTextureImage(std::string texturePath)
    {
        std::string extension = texturePath.substr(texturePath.find_last_of(".") + 1);

        //pngs must be compressed by this point
        if (extension == "png")
        {
            //extension = "ktx2";
            //texturePath = texturePath.substr(0, texturePath.find_last_of(".") + 1).append(extension);
        }

        if(extension == "ktx2")
		{
            ktxTexture2* ktx_texture;

            KTX_error_code result = ktxTexture_CreateFromNamedFile(texturePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture**)&ktx_texture);
            if (result != KTX_SUCCESS)
            {
                throw std::runtime_error("Could not load the requested image file.");
            }

            AllocatedBuffer stagingBuffer = CreateBuffer(ktx_texture->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

            void* data = stagingBuffer.allocation->GetMappedData();

            memcpy(data, ktx_texture->pData, ktx_texture->dataSize);

            VkImage textureImage;

            VkFormat format = (VkFormat)ktx_texture->vkFormat;

            CreateImage(ktx_texture->baseWidth, ktx_texture->baseHeight, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage);

            TransitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            CopyBufferToImage(stagingBuffer.buffer, textureImage, ktx_texture->baseWidth, ktx_texture->baseHeight);

            TransitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

            VkImageView textureImageView = CreateImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

            images.push_back(textureImageView);
		}

        else if (extension == "png" || extension == "jpg" || extension == "jpeg")
        {

            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!pixels) {
                throw std::runtime_error("Failed to load texture image!");
            }

            AllocatedBuffer stagingBuffer = CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

            void* data = stagingBuffer.allocation->GetMappedData();
            memcpy(data, pixels, static_cast<size_t>(imageSize));

            stbi_image_free(pixels);

            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

            VkImage textureImage;
            CreateImage(texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage);

            TransitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            CopyBufferToImage(stagingBuffer.buffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

            TransitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

            VkImageView textureImageView = CreateImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

            images.push_back(textureImageView);
        }
        else
        {
			throw std::runtime_error("Unsupported texture format!");
        }
        
    }

    void CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR; 
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; 
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(phys_device, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(vkb_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create texture sampler!");
		}

        deletionQueue.push_function([&]() {
            vkDestroySampler(vkb_device, textureSampler, nullptr);
            });
    }

    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) 
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(vkb_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image view!");
        }

        deletionQueue.push_function([=]() {
			vkDestroyImageView(vkb_device, imageView, nullptr);
			});

        return imageView;
    }

    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image)
    {
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        if (vkCreateImage(vkb_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vmaAllocateMemoryForImage(allocator, image, &allocInfo, &allocation, &allocationInfo);

        vmaBindImageMemory(allocator, allocation, image);

        deletionQueue.push_function([=]() {
			vmaDestroyImage(allocator, image, allocation);
			});
    }

    void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        OneTimeSubmit([&](VkCommandBuffer cmd) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0; // TODO
            barrier.dstAccessMask = 0; // TODO

            if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else 
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else 
            {
                throw std::invalid_argument("Unsupported layout transition!");
            }


            vkCmdPipelineBarrier(
                cmd,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

			});
    }

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        OneTimeSubmit([&](VkCommandBuffer cmd) {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                width,
                height,
                1
            };

            vkCmdCopyBufferToImage(
                cmd,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
            });
    }

    AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaallocInfo{};
        vmaallocInfo.usage = memoryUsage;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        AllocatedBuffer newBuffer;

        if(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate buffer!");
		}

        return newBuffer;
    }

    void CreateSwapChain()
    {
        int width, height;
       
        SDL_GetWindowSize(_window, &width, &height);

        while (width == 0 || height == 0) {
            SDL_WaitEvent(NULL);
            SDL_GetWindowSize(_window, &width, &height);
        }

        swapChainExtent.height = static_cast<uint32_t>(height);
        swapChainExtent.width = static_cast<uint32_t>(width);

        swapChainSurfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
        swapChainSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        vkb::SwapchainBuilder swapchain_builder{ vkb_device };
        auto swap_ret = swapchain_builder
            .set_desired_extent(swapChainExtent.width, swapChainExtent.height)
            .set_desired_format(swapChainSurfaceFormat)
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .build();
        if (!swap_ret) {
            throw std::runtime_error("Failed to create swapchain!");
        }
        vkb_swapchain = swap_ret.value();

        swapChainImages = vkb_swapchain.get_images().value();
        swapChainImageViews = vkb_swapchain.get_image_views().value();        
    }

    void CreateRenderPass()
    {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainSurfaceFormat.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        //subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        //subpass dependency
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        //render pass
        VkRenderPassCreateInfo renderPassInfo{};
        std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(vkb_device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyRenderPass(vkb_device, renderPass, nullptr);
            });
        
    }

    void CreateShadowRenderPass()
    {
        VkAttachmentDescription attachmentDescription{};
        attachmentDescription.format = shadowDepthFormat;
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end
        // Attachment will be transitioned to shader read at render pass end

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 0;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass
        // Attachment will be used as depth/stencil during render pass

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;													// No color attachments
        subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachmentDescription;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(vkb_device, &renderPassInfo, nullptr, &shadowRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        deletionQueue.push_function([&]() {
			vkDestroyRenderPass(vkb_device, shadowRenderPass, nullptr);
			});
    }

    void CreateDescriptorSetLayout()
    {
        // Vertex Storage Buffer
        VkDescriptorSetLayoutBinding vertexBufferLayoutBinding{};
        vertexBufferLayoutBinding.binding = 0;
        vertexBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertexBufferLayoutBinding.descriptorCount = 1;
        vertexBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = MAX_TEXTURE_COUNT;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding sceneDataLayoutBinding{};
        sceneDataLayoutBinding.binding = 2;
        sceneDataLayoutBinding.descriptorCount = 1;
        sceneDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding objectInstanceLayoutBinding{};
		objectInstanceLayoutBinding.binding = 3;
		objectInstanceLayoutBinding.descriptorCount = 1;
		objectInstanceLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		objectInstanceLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vertexBufferLayoutBinding, samplerLayoutBinding, sceneDataLayoutBinding, objectInstanceLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(vkb_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }

        deletionQueue.push_function([&]() {
			vkDestroyDescriptorSetLayout(vkb_device, descriptorSetLayout, nullptr);
			});
    }

    void CreateShadowDescriptorSetLayout()
	{
        // Vertex Storage Buffer
        VkDescriptorSetLayoutBinding vertexBufferLayoutBinding{};
        vertexBufferLayoutBinding.binding = 0;
        vertexBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertexBufferLayoutBinding.descriptorCount = 1;
        vertexBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding shadowDataLayoutBinding{};
        shadowDataLayoutBinding.binding = 1;
        shadowDataLayoutBinding.descriptorCount = 1;
        shadowDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shadowDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 2;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding objectInstanceLayoutBinding{};
        objectInstanceLayoutBinding.binding = 3;
        objectInstanceLayoutBinding.descriptorCount = 1;
        objectInstanceLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        objectInstanceLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vertexBufferLayoutBinding, shadowDataLayoutBinding, samplerLayoutBinding, objectInstanceLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(vkb_device, &layoutInfo, nullptr, &shadowDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyDescriptorSetLayout(vkb_device, shadowDescriptorSetLayout, nullptr);
            });
	}

    void CreateDebugQuadDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		if (vkCreateDescriptorSetLayout(vkb_device, &layoutInfo, nullptr, &debugQuadDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
		deletionQueue.push_function([&]() {
			vkDestroyDescriptorSetLayout(vkb_device, debugQuadDescriptorSetLayout, nullptr);
			});
    }

    void CreateGraphicsPipeline()
    {

        //auto vertShaderCode = ReadFile("shaders/shader.vert");
        //auto fragShaderCode = ReadFile("shaders/shader.frag");

        //VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, shaderc_glsl_vertex_shader);
        //VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode, shaderc_glsl_fragment_shader);

        
		auto vertShaderCode = ReadFileStr("shaders/shader.vert");
		auto fragShaderCode = ReadFileStr("shaders/shader.frag");

        std::vector<uint32_t> spirvCode = CompileGLSLtoSPV(vertShaderCode, EShLangVertex);
        VkShaderModule vertShaderModule = CreateShaderModule(spirvCode);

		spirvCode = CompileGLSLtoSPV(fragShaderCode, EShLangFragment);
		VkShaderModule fragShaderModule = CreateShaderModule(spirvCode);
        

        //vertex shader
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        //fragment shader
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        //shader stages
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        //vertex input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        //dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        //scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        //viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        //rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        //multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        //color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        //push constants
        /*
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(GPUPushConstants);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        */

        //depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional
        

        //pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        //pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
        //pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

        if (vkCreatePipelineLayout(vkb_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyPipelineLayout(vkb_device, pipelineLayout, nullptr);
            });
        

        //pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(vkb_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyPipeline(vkb_device, graphicsPipeline, nullptr);
            });

        vkDestroyShaderModule(vkb_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(vkb_device, vertShaderModule, nullptr);
    }

    void CreateDebugQuadPipeline()
    {
        //auto vertShaderCode = ReadFile("shaders/debugQuad.vert");
        //auto fragShaderCode = ReadFile("shaders/debugQuad.frag");

        //VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, shaderc_glsl_vertex_shader);
        //VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode, shaderc_glsl_fragment_shader);

        
        auto vertShaderCode = ReadFileStr("shaders/debugQuad.vert");
        auto fragShaderCode = ReadFileStr("shaders/debugQuad.frag");

        std::vector<uint32_t> spirvCode = CompileGLSLtoSPV(vertShaderCode, EShLangVertex);
        VkShaderModule vertShaderModule = CreateShaderModule(spirvCode);

        spirvCode = CompileGLSLtoSPV(fragShaderCode, EShLangFragment);
        VkShaderModule fragShaderModule = CreateShaderModule(spirvCode);
        

        //vertex shader
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        //fragment shader
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        //shader stages
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        //vertex input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        //dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        //scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        //viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        //rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        //multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        //color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        //push constants
        /*
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(GPUPushConstants);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        */

        //depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional


        //pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &debugQuadDescriptorSetLayout;
        //pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
        //pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

        if (vkCreatePipelineLayout(vkb_device, &pipelineLayoutInfo, nullptr, &debugQuadPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyPipelineLayout(vkb_device, debugQuadPipelineLayout, nullptr);
            });


        //pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = debugQuadPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(vkb_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &debugQuadPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyPipeline(vkb_device, debugQuadPipeline, nullptr);
            });

        vkDestroyShaderModule(vkb_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(vkb_device, vertShaderModule, nullptr);
    }

    void CreateShadowPipeline()
    {
        

        //auto vertShaderCode = ReadFile("shaders/shadow.vert");

        //VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, shaderc_glsl_vertex_shader);

        auto vertShaderCode = ReadFileStr("shaders/shadow.vert");
        std::vector<uint32_t> spirvCode = CompileGLSLtoSPV(vertShaderCode, EShLangVertex);
        VkShaderModule vertShaderModule = CreateShaderModule(spirvCode);

        //vertex shader
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        //fragment shader
        /*
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        */

        //shader stages
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo };

        //vertex input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        //dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        //scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent.height = (float)shadowMapResolution;
        scissor.extent.width = (float)shadowMapResolution;

        //viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)shadowMapResolution;
        viewport.height = (float)shadowMapResolution;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        //rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        //multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        //color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 0;
        colorBlending.pAttachments = nullptr;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        //push constants
        /*
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(GPUPushConstants);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        */

        //depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional


        //pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &shadowDescriptorSetLayout;
        //pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
        //pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

        if (vkCreatePipelineLayout(vkb_device, &pipelineLayoutInfo, nullptr, &shadowPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyPipelineLayout(vkb_device, shadowPipelineLayout, nullptr);
            });


        //pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 1;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = shadowPipelineLayout;
        pipelineInfo.renderPass = shadowRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(vkb_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shadowPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyPipeline(vkb_device, shadowPipeline, nullptr);
            });

       // vkDestroyShaderModule(vkb_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(vkb_device, vertShaderModule, nullptr);
    }

    void CreateFrameBuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) 
        {
            std::vector<VkImageView> attachments
            {
                swapChainImageViews[i],
                depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(vkb_device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }     
    }

    void CreateShadowFrameBuffer()
    {
       

        CreateImage(shadowMapResolution, shadowMapResolution, shadowDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImage);

        shadowImageView = CreateImageView(shadowImage, shadowDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        VkFilter shadowmap_filter = VK_FILTER_LINEAR;
        VkSamplerCreateInfo sampler{};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = shadowmap_filter;
        sampler.minFilter = shadowmap_filter;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = sampler.addressModeU;
        sampler.addressModeW = sampler.addressModeU;
        sampler.mipLodBias = 0.0f;
        sampler.maxAnisotropy = 1.0f;
        sampler.minLod = 0.0f;
        sampler.maxLod = 1.0f;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        if (vkCreateSampler(vkb_device, &sampler, nullptr, &shadowSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow sampler!");
        }

        deletionQueue.push_function([&]() {
            vkDestroySampler(vkb_device, shadowSampler, nullptr);
            });

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = shadowRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &shadowImageView;
        framebufferInfo.width = shadowMapResolution;
        framebufferInfo.height = shadowMapResolution;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkb_device, &framebufferInfo, nullptr, &shadowFramebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }

        deletionQueue.push_function([&]() {
			vkDestroyFramebuffer(vkb_device, shadowFramebuffer, nullptr);
			});

    }

    void RecreateSwapChain()
    {
        vkDeviceWaitIdle(vkb_device);

        std::cout << "Recreate Swapchain!" << std::endl;

        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(vkb_device, swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(vkb_device, swapChainImageViews[i], nullptr);
        }

        vkb::destroy_swapchain(vkb_swapchain);

        CreateSwapChain();
        CreateFrameBuffers();
    }

    void CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

        if (vkCreateCommandPool(vkb_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }

        deletionQueue.push_function([&]() {
            vkDestroyCommandPool(vkb_device, commandPool, nullptr);
            });   
    }

    void CreateDescriptorPool()
    {
        std::vector<VkDescriptorPoolSize> poolSizes(3);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES * 4);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES) * (MAX_TEXTURE_COUNT + 2);
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES * 2);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES * 3);

        if (vkCreateDescriptorPool(vkb_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }

        deletionQueue.push_function([&]() {
			vkDestroyDescriptorPool(vkb_device, descriptorPool, nullptr);
			});
    }

    void CreateShadowDescriptorPool()
    {
        std::vector<VkDescriptorPoolSize> poolSizes(3);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES);

		if (vkCreateDescriptorPool(vkb_device, &poolInfo, nullptr, &shadowDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool!");
		}

		deletionQueue.push_function([&]() {
			vkDestroyDescriptorPool(vkb_device, shadowDescriptorPool, nullptr);
			});
    }

    void CreateDescriptorSets()
    {
		
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES);
		allocInfo.pSetLayouts = layouts.data();

        //allocate descriptor sets
		if (vkAllocateDescriptorSets(vkb_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

        std::vector<VkDescriptorImageInfo> imageInfos;

        int i = 0;
        for (auto& imageView : images)
        {
            VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = imageView;
			imageInfo.sampler = textureSampler;

			imageInfos.push_back(imageInfo);
            i++;
        }

        for(i; i < 256; i++)
		{
			VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = images[0];
            imageInfo.sampler = textureSampler;
            
            imageInfos.push_back(imageInfo);
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfo.imageView = shadowImageView;
        imageInfo.sampler = shadowSampler;

        imageInfos[255] = imageInfo;

        VkDescriptorBufferInfo vertexBufferInfo{};
        vertexBufferInfo.buffer = vertexBuffer.buffer;
        vertexBufferInfo.offset = 0;
        vertexBufferInfo.range = assetManager.vertices.size() * sizeof(Vertex);

        VkDescriptorBufferInfo sceneBufferInfo{};
        sceneBufferInfo.buffer = sceneDataUniformBuffer.buffer;
        sceneBufferInfo.offset = 0;
        sceneBufferInfo.range = sizeof(SceneData);

		VkDescriptorBufferInfo objectInstanceBufferInfo{};
		objectInstanceBufferInfo.buffer = objectInstanceBuffer.buffer;
		objectInstanceBufferInfo.offset = 0;
		objectInstanceBufferInfo.range = MAX_OBJECTS * sizeof(ObjectInstance);

		for (size_t i = 0; i < MAX_FRAMES; i++) 
        {
			std::vector<VkWriteDescriptorSet> descriptorWrites(4);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &vertexBufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = MAX_TEXTURE_COUNT;
            descriptorWrites[1].pImageInfo = imageInfos.data();

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &sceneBufferInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &objectInstanceBufferInfo;

            vkUpdateDescriptorSets(vkb_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateShadowDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES, shadowDescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES);
		allocInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(vkb_device, &allocInfo, shadowDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

		VkDescriptorBufferInfo vertexBufferInfo{};
		vertexBufferInfo.buffer = vertexBuffer.buffer;
		vertexBufferInfo.offset = 0;
		vertexBufferInfo.range = assetManager.vertices.size() * sizeof(Vertex);

		VkDescriptorBufferInfo shadowBufferInfo{};
		shadowBufferInfo.buffer = shadowUniformBuffer.buffer;
		shadowBufferInfo.offset = 0;
		shadowBufferInfo.range = sizeof(ShadowData);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = shadowImageView;
		imageInfo.sampler = shadowSampler;

        VkDescriptorBufferInfo objectInstanceBufferInfo{};
        objectInstanceBufferInfo.buffer = objectInstanceBuffer.buffer;
        objectInstanceBufferInfo.offset = 0;
        objectInstanceBufferInfo.range = MAX_OBJECTS * sizeof(ObjectInstance);

        for (size_t i = 0; i < MAX_FRAMES; i++)
        {
            std::vector<VkWriteDescriptorSet> descriptorWrites(4);

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = shadowDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &vertexBufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = shadowDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &shadowBufferInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = shadowDescriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &imageInfo;

            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = shadowDescriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo = &objectInstanceBufferInfo;

            vkUpdateDescriptorSets(vkb_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        }



    }

    void CreateDebugQuadDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES, debugQuadDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(vkb_device, &allocInfo, debugQuadDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfo.imageView = shadowImageView;
        imageInfo.sampler = shadowSampler;

        for (size_t i = 0; i < MAX_FRAMES; i++)
        {
            std::vector<VkWriteDescriptorSet> descriptorWrites(1);

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = debugQuadDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(vkb_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateCommandBuffer()
    {
        frames.resize(MAX_FRAMES);

        //temp array to hold command buffers
        std::vector<VkCommandBuffer> commandBuffers(frames.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(vkb_device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        for (size_t i = 0; i < frames.size(); i++) {
            frames[i].commandBuffer = commandBuffers[i];
        }
    }

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
    {
        
        //assetManager.SetObjectInstanceTransform("Cube", 0, camera.Position + glm::vec3(0,1,0));

        ObjectInstanceData* objectInstanceData = (ObjectInstanceData*)objectInstanceBuffer.allocation->GetMappedData();

        uint32_t instanceIndex = 0;

        for (Object& object : assetManager.objects)
        {
            for (ObjectInstance& instance : object.instances)
            {
                std::copy(std::begin(instance.boneTransforms), std::end(instance.boneTransforms), std::begin(objectInstanceData[instanceIndex].boneTransforms));
				objectInstanceData[instanceIndex].model = instance.model;
				objectInstanceData[instanceIndex].currentAnimation = instance.currentAnimation;

                instanceIndex++;
            }
        }

        glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

        float near_plane = 1.0f, far_plane = 10.f;
        glm::mat4 lightProjection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, far_plane, near_plane);

		lightProjection[1][1] *= -1;

        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));

        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = shadowRenderPass;
            renderPassInfo.framebuffer = shadowFramebuffer;
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent.height = shadowMapResolution;
            renderPassInfo.renderArea.extent.width = shadowMapResolution;

            std::vector<VkClearValue> clearValues(1);
            clearValues[0].depthStencil = { 0.0f, 0 };

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(shadowMapResolution);
            viewport.height = static_cast<float>(shadowMapResolution);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent.height = shadowMapResolution;
            scissor.extent.width = shadowMapResolution;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


           

            ShadowData shadowData{};
            shadowData.lightSpaceMatrix = lightSpaceMatrix;
            shadowData.model = glm::mat4(1.0f);

            void* data = shadowUniformBuffer.allocation->GetMappedData();
            memcpy(data, &shadowData, sizeof(ShadowData));

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);

            vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            DrawQueue transparentQueue;

            instanceIndex = 0;

            for (Object& object : assetManager.objects)
            {
                uint32_t instanceCount = object.instances.size();

                if (instanceCount == 0)
                {
                    continue;
                }

                for (Mesh& mesh : object.meshes)
                {
                    if (mesh.isTransparent)
                    {
                        transparentQueue.push_function([=]() {
                            vkCmdDrawIndexed(commandBuffer, mesh.indexCount, instanceCount, mesh.startIndex, 0, instanceIndex);
                            });
                    }
                    else
                    {
                        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, instanceCount, mesh.startIndex, 0, instanceIndex);
                    }
                }

                instanceIndex += instanceCount;
            }

            transparentQueue.flush();

            vkCmdEndRenderPass(commandBuffer);
        }

        {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = swapChainExtent;

            std::vector<VkClearValue> clearValues(2);
            clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
            clearValues[1].depthStencil = { 1.0f, 0 };

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChainExtent.width);
            viewport.height = static_cast<float>(swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            

            glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);

            projection[1][1] *= -1;

            glm::mat4 view = camera.GetViewMatrix();

            glm::mat4 model = glm::mat4(1.0f);

            SceneData sceneData{};
            sceneData.projection = projection;
            sceneData.view = view;
            sceneData.model = model;
            sceneData.lightSpaceMatrix = lightSpaceMatrix;
            sceneData.lightPos = glm::vec4(lightPos, 0.f);

            void* data = sceneDataUniformBuffer.allocation->GetMappedData();
            memcpy(data, &sceneData, sizeof(SceneData));

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

            vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			DrawQueue transparentQueue;

			instanceIndex = 0;

            for (Object& object : assetManager.objects)
            {
				uint32_t instanceCount = object.instances.size();

				if (instanceCount == 0)
				{
					continue;
				}

				for (Mesh& mesh : object.meshes)
				{
					if (mesh.isTransparent)
					{
						transparentQueue.push_function([=]() {
							vkCmdDrawIndexed(commandBuffer, mesh.indexCount, instanceCount, mesh.startIndex, 0, instanceIndex);
							});
					}
					else
					{
						vkCmdDrawIndexed(commandBuffer, mesh.indexCount, instanceCount, mesh.startIndex, 0, instanceIndex);
					}
				}

				instanceIndex += instanceCount;
			}

			transparentQueue.flush();

            if(renderDebugQuad)
            { 
			//render debug quad
			    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debugQuadPipeline);

			    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debugQuadPipelineLayout, 0, 1, &debugQuadDescriptorSets[currentFrame], 0, nullptr);

			    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
			}

            vkCmdEndRenderPass(commandBuffer);

        }

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void CreateSyncPrimitives()
    {

        for (int i = 0; i < frames.size(); i++)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateSemaphore(vkb_device, &semaphoreInfo, nullptr, &frames[i].imageAvailableSemaphore) != VK_SUCCESS ||
                vkCreateSemaphore(vkb_device, &semaphoreInfo, nullptr, &frames[i].renderFinishedSemaphore) != VK_SUCCESS ||
                vkCreateFence(vkb_device, &fenceInfo, nullptr, &frames[i].renderFence) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create sync primitives!");

                
            }

            deletionQueue.push_function([this, i]() {
                vkDestroySemaphore(vkb_device, frames[i].imageAvailableSemaphore, nullptr);
                vkDestroySemaphore(vkb_device, frames[i].renderFinishedSemaphore, nullptr);
                vkDestroyFence(vkb_device, frames[i].renderFence, nullptr);
                });
        }     
    }

    void Draw()
    {
        //wait for previous frame
        vkWaitForFences(vkb_device, 1, &frames[currentFrame].renderFence, VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkb_device, vkb_swapchain, UINT64_MAX, frames[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        vkResetFences(vkb_device, 1, &frames[currentFrame].renderFence);

        vkResetCommandBuffer(frames[currentFrame].commandBuffer, 0);

        RecordCommandBuffer(frames[currentFrame].commandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { frames[currentFrame].imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &frames[currentFrame].commandBuffer;

        VkSemaphore signalSemaphores[] = { frames[currentFrame].renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frames[currentFrame].renderFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { vkb_swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            RecreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES;
    }

    void MainLoop() {
        float currentFrame = (float)SDL_GetTicks64() / 1000.0f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        KeyboardInput();
        MouseInput();

        while (SDL_PollEvent(&e) != 0) 
        {
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    bQuit = true;
                }
                else if (e.key.keysym.sym == SDLK_m)
                {
                    if (SDL_GetRelativeMouseMode() == SDL_TRUE)
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    else
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                }
                else if (e.key.keysym.sym == SDLK_o)
                {
					renderDebugQuad = !renderDebugQuad;
                }
            }

        }

		assetManager.UpdateAnimationSystem(deltaTime);

        Draw();
    }

    void Cleanup() {
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(vkb_device, swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(vkb_device, swapChainImageViews[i], nullptr);
        }

        vkb::destroy_swapchain(vkb_swapchain);

        deletionQueue.flush();

        SDL_DestroyWindow(_window);
    }

    VkShaderModule CreateShaderModule(const std::vector<char>& code, shaderc_shader_kind shader_kind)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        std::string str_code(code.begin(), code.end());
       
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            str_code, shader_kind, "shader.glsl", options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) 
        {
			std::cout << result.GetErrorMessage() << std::endl;
            throw std::runtime_error("Shader compilation failed!: " + std::string(result.GetErrorMessage()));
        }

        std::vector<uint32_t> compiledCode{ result.cbegin(), result.cend() };

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = compiledCode.size() * sizeof(uint32_t);
        createInfo.pCode = compiledCode.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vkb_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirvCode) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
        createInfo.pCode = spirvCode.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vkb_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    std::vector<uint32_t> CompileGLSLtoSPV(const std::string& sourceCode, EShLanguage shaderType) {
        const char* shaderStrings[1];
        shaderStrings[0] = sourceCode.c_str();

        glslang::TShader shader(shaderType);
        shader.setStrings(shaderStrings, 1);

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClient::EShClientOpenGL, glslang::EShTargetClientVersion::EShTargetOpenGL_450);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_3);

        // Set up default resource limits
        const TBuiltInResource* resources = GetDefaultResources();

        // Parse the GLSL shader
        if (!shader.parse(resources, 450, false, EShMsgDefault)) {
			std::cout << std::string(shader.getInfoLog()) << std::endl;
            throw std::runtime_error("GLSL Parsing Failed:\n" + std::string(shader.getInfoLog()));
        }

        // Link the shader into a program
        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(EShMsgDefault)) {
            throw std::runtime_error("GLSL Linking Failed:\n" + std::string(program.getInfoLog()));
        }

        // Convert the linked program to SPIR-V
        std::vector<uint32_t> spirv;
        glslang::GlslangToSpv(*program.getIntermediate(shaderType), spirv);

        return spirv;
    }


};

