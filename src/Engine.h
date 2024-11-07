#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class UEngine {
private:
    GLFWwindow* window;

    vkb::Instance vkb_instance;

    vkb::Device vkb_device;

    VkQueue graphicsQueue;

    VkQueue presentQueue;

    VkSurfaceKHR surface;

    vkb::Swapchain swapchain;

    std::vector<VkImage> swapChainImages;

    std::vector<VkImageView> swapChainImageViews;


public:
    void Run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Engine", nullptr, nullptr);
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

        if (glfwCreateWindowSurface(vkb_instance.instance, window, NULL, &surface) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to create surface!");
        }

        vkb::PhysicalDeviceSelector phys_device_selector(vkb_instance);
        auto physical_device_selector_return = phys_device_selector
            .set_surface(surface)
            .select();
        if (!physical_device_selector_return) {
            throw std::runtime_error("Failed to select physical device!");
        }
        auto phys_device = physical_device_selector_return.value();

        vkb::DeviceBuilder device_builder{ phys_device };
        auto dev_ret = device_builder.build();
        if (!dev_ret) {
            throw std::runtime_error("Failed to create logical device!");
        }
        vkb_device = dev_ret.value();

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

        vkb::SwapchainBuilder swapchain_builder{ vkb_device };
        auto swap_ret = swapchain_builder
            .set_desired_extent(WIDTH, HEIGHT)
            .build();
        if (!swap_ret) {
            throw std::runtime_error("Failed to create swapchain!");
        }
        swapchain = swap_ret.value();

        swapChainImages = swapchain.get_images().value();
        swapChainImageViews = swapchain.get_image_views().value();
    }

    void MainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void Cleanup() {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(vkb_device, imageView, nullptr);
        }

        vkb::destroy_swapchain(swapchain);

        vkb::destroy_device(vkb_device);

        vkb::destroy_surface(vkb_instance, surface);

        vkb::destroy_instance(vkb_instance);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

