/*

███    ███ ███    █▄   ▄█          ▄█   ▄█▄    ▄████████ ███▄▄▄▄        ▀█████████▄     ▄████████  ▄████████    ▄█   ▄█▄    ▄████████ ███▄▄▄▄   ████████▄
███    ███ ███    ███ ███         ███ ▄███▀   ███    ███ ███▀▀▀██▄        ███    ███   ███    ███ ███    ███   ███ ▄███▀   ███    ███ ███▀▀▀██▄ ███   ▀███
███    ███ ███    ███ ███         ███▐██▀     ███    ███ ███   ███        ███    ███   ███    ███ ███    █▀    ███▐██▀     ███    █▀  ███   ███ ███    ███
███    ███ ███    ███ ███        ▄█████▀      ███    ███ ███   ███       ▄███▄▄▄██▀    ███    ███ ███         ▄█████▀     ▄███▄▄▄     ███   ███ ███    ███
███    ███ ███    ███ ███       ▀▀█████▄    ▀███████████ ███   ███      ▀▀███▀▀▀██▄  ▀███████████ ███        ▀▀█████▄    ▀▀███▀▀▀     ███   ███ ███    ███
███    ███ ███    ███ ███         ███▐██▄     ███    ███ ███   ███        ███    ██▄   ███    ███ ███    █▄    ███▐██▄     ███    █▄  ███   ███ ███    ███
 ▀█▄  ▄█▀  ███    ███ ███▌    ▄   ███ ▀███▄   ███    ███ ███   ███        ███    ███   ███    ███ ███    ███   ███ ▀███▄   ███    ███ ███   ███ ███   ▄███
  ▀████▀   ████████▀  █████▄▄██   ███   ▀█▀   ███    █▀   ▀█   █▀       ▄█████████▀    ███    █▀  ████████▀    ███   ▀█▀   ██████████  ▀█   █▀  ████████▀

*/
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#define VMA_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "VK_backEnd.h"
#include "VkBootstrap.h"
#include "VK_assetManager.h"
#include "vk_mem_alloc.h"
#include "VK_Renderer.h"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
#include "../../Game/Scene.h"
#include "../../ErrorChecking.h"

namespace VulkanBackEnd {

    struct UploadContext {
        VkFence _uploadFence;
        VkCommandPool _commandPool;
        VkCommandBuffer _commandBuffer;
    };

    VkDevice _device;
    VkInstance _instance;
    VmaAllocator _allocator;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat _swachainImageFormat;
    VkSampler _sampler;
    VkDescriptorPool _descriptorPool;
    VkExtent2D _renderTargetPresentExtent = { 768 , 432 };
    VkExtent2D _windowedModeExtent = { 2280, 1620 };
    VkExtent2D _currentWindowExtent = { };
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDeviceMemoryProperties _memoryProperties;
    VkPhysicalDevice _physicalDevice;
    VkPhysicalDeviceProperties _gpuProperties;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  _rayTracingPipelineProperties = {};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    UploadContext _uploadContext;
    FrameData _frames[FRAME_OVERLAP];
    int _frameNumber = { 0 };

    uint32_t g_allocatedSkinnedVertexBufferSize = 0;

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
    PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
    PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin;
    PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd;
    PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

    inline VkQueue _graphicsQueue;
    inline uint32_t _graphicsQueueFamily;
    vkb::Instance _bootstrapInstance;
    bool _frameBufferResized = false;
    bool _enableValidationLayers = true;
    bool _printAvaliableExtensions = false;
    bool _thereAreStillAssetsToLoad = true;
    bool _forceCloseWindow;

    void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

    RayTracingScratchBuffer CreateScratchBuffer(VkDeviceSize size);


   // void CreateTopLevelAccelerationStructure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS);


    //void UpdateStaticDescriptorSet();

    //void BuildRayTracingCommandBuffers(int swapchainIndex);

    inline uint32_t _mainIndexCount;
    //inline AllocatedBuffer _mousePickResultBuffer;
    //inline AllocatedBuffer _mousePickResultCPUBuffer; // for mouse picking
    //inline VkDeviceOrHostAddressConstKHR _vertexBufferDeviceAddress{};
    //inline VkDeviceOrHostAddressConstKHR _indexBufferDeviceAddress{};
    //inline VkDeviceOrHostAddressConstKHR _transformBufferDeviceAddress{};
    inline AllocatedBuffer _rtInstancesBuffer;
    inline Buffer _pointCloudBuffer;

    VkDevice GetDevice() {
        return _device;
    }
    VkSurfaceKHR GetSurface() {
        return _surface;
    }
    VkSwapchainKHR GetSwapchain() {
        return _swapchain;
    }
    int32_t GetFrameIndex() {
        return _frameNumber % FRAME_OVERLAP;
    }
    VkQueue GetGraphicsQueue() {
        return _graphicsQueue;
    }
    FrameData& GetCurrentFrame() {
        return _frames[GetFrameIndex()];
    }
    FrameData& GetFrameByIndex(int index) {
        return _frames[index];
    }
    VmaAllocator GetAllocator() {
        return _allocator;
    }
    VkDescriptorPool GetDescriptorPool() {
        return _descriptorPool;
    }
    VkSampler VulkanBackEnd::GetSampler() {
        return _sampler;
    }
    std::vector<VkImage>& GetSwapchainImages() {
        return _swapchainImages;
    }
    void VulkanBackEnd::AdvanceFrameIndex() {
        _frameNumber++;
    }
}

bool VulkanBackEnd::FrameBufferWasResized() {
    return _frameBufferResized;
}

void VulkanBackEnd::MarkFrameBufferAsResized() {
    _frameBufferResized = true;
}

void VulkanBackEnd::HandleFrameBufferResized() {
    RecreateDynamicSwapchain();
    _frameBufferResized = false;
}

void VulkanBackEnd::InitMinimum() {

    SetGLFWSurface();
    SelectPhysicalDevice();
    CreateSwapchain();

    VulkanRenderer::CreateMinimumShaders();
    VulkanRenderer::CreateRenderTargets();

    CreateCommandBuffers();
    CreateSyncStructures();
    CreateSampler();

    VulkanRenderer::CreateDescriptorSets();
    VulkanRenderer::CreatePipelinesMinimum();

    VulkanRenderer::CreateStorageBuffers();
}

void VulkanBackEnd::SetGLFWSurface() {
    glfwCreateWindowSurface(_instance, BackEnd::GetWindowPointer(), nullptr, &_surface);
}

void VulkanBackEnd::CreateVulkanInstance() {
    vkb::InstanceBuilder builder;
    builder.set_app_name("Unloved");
    builder.request_validation_layers(_enableValidationLayers);
    builder.use_default_debug_messenger();
    builder.require_api_version(1, 3, 0);
    _bootstrapInstance = builder.build().value();
    _instance = _bootstrapInstance.instance;
    _debugMessenger = _bootstrapInstance.debug_messenger;
}

void VulkanBackEnd::SelectPhysicalDevice() {

    vkb::PhysicalDeviceSelector selector{ _bootstrapInstance };
    selector.add_required_extension(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);					// Hides shader warnings for unused variables
    selector.add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);				// Dynamic rendering
    selector.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);			// Ray tracing related extensions required by this sample
    selector.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);			// Ray tracing related extensions required by this sample
    selector.add_required_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);			// Required by VK_KHR_acceleration_structure
    selector.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);		// Required by VK_KHR_acceleration_structure
    selector.add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);				// Required by VK_KHR_acceleration_structure
    selector.add_required_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);						// Required for VK_KHR_ray_tracing_pipeline
    selector.add_required_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);			// Required by VK_KHR_spirv_1_4
    selector.add_required_extension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);	// aftermath
    selector.add_required_extension(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);		// aftermath

    selector.add_required_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME);

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    rayTracingPipelineFeatures.rayTraversalPrimitiveCulling = VK_TRUE;
    rayTracingPipelineFeatures.pNext = nullptr;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;
    rayQueryFeatures.pNext = &accelerationStructureFeatures;

    VkPhysicalDeviceFeatures features = {};
    features.samplerAnisotropy = true;
    features.shaderInt64 = true;
    features.multiDrawIndirect = true;
    selector.set_required_features(features);

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.shaderSampledImageArrayNonUniformIndexing = true;
    features12.runtimeDescriptorArray = true;
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorIndexing = true;
    features12.bufferDeviceAddress = true;
    features12.scalarBlockLayout = true;
    selector.set_required_features_12(features12);

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.maintenance4 = true;
    features13.dynamicRendering = true;
    features13.pNext = &rayQueryFeatures;	// you probably want to confirm this chaining of pNexts works when shit goes wrong.
    selector.set_required_features_13(features13);

    selector.set_minimum_version(1, 3);
    selector.set_surface(_surface);
    vkb::PhysicalDevice physicalDevice = selector.select().value();
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    // store these for some ray tracing stuff.
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &_memoryProperties);

    // Query available device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    // Check if extension is supported
    //bool maintenance4ExtensionSupported = false;
    for (const auto& extension : availableExtensions) {
        if (std::string(extension.extensionName) == VK_KHR_RAY_QUERY_EXTENSION_NAME) {
            //maintenance4ExtensionSupported = true;
            std::cout << "VK_KHR_RAY_QUERY_EXTENSION_NAME is supported\n";
            break;
        }
    }
    // Specify the extensions to enable
    //std::vector<const char*> enabledExtensions = {
    //    VK_KHR_MAINTENANCE_4_EXTENSION_NAME
    //};
    //if (!maintenance4ExtensionSupported) {
    //    throw std::runtime_error("Required extension not supported");
   // }

    vkb::Device vkbDevice = deviceBuilder.build().value();
    _device = vkbDevice.device;
    _physicalDevice = physicalDevice.physical_device;
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // Initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    vkGetPhysicalDeviceProperties(_physicalDevice, &_gpuProperties);

    auto instanceVersion = VK_API_VERSION_1_0;
    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersion) {
        vkEnumerateInstanceVersion(&instanceVersion);
    }

    uint32_t major = VK_VERSION_MAJOR(instanceVersion);
    uint32_t minor = VK_VERSION_MINOR(instanceVersion);
    uint32_t patch = VK_VERSION_PATCH(instanceVersion);
    std::cout << "Vulkan: " << major << "." << minor << "." << patch << "\n\n";

    if (_printAvaliableExtensions) {
        uint32_t deviceExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());
        std::cout << "Available device extensions:\n";
        for (const auto& extension : deviceExtensions) {
            std::cout << ' ' << extension.extensionName << "\n";
        }
        std::cout << "\n";
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "Available instance extensions:\n";
        for (const auto& extension : extensions) {
            std::cout << ' ' << extension.extensionName << "\n";
        }
        std::cout << "\n";
    }

    // Get the ray tracing and acceleration structure related function pointers required by this sample
    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(_device, "vkGetBufferDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device, "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCreateAccelerationStructureKHR"));
    //vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureDeviceAddressKHR"));
    //vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(_device, "vkCmdTraceRaysKHR"));
    VulkanRenderer::LoadRaytracingFunctionPointer();
    vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_device, "vkCreateRayTracingPipelinesKHR"));

    // Debug marker shit
    vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(_device, "vkSetDebugUtilsObjectNameEXT"));

    // Get ray tracing pipeline properties, which will be used later on in the sample
    _rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &_rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(_physicalDevice, &deviceProperties2);

    // Get acceleration structure properties, which will be used later on in the sample
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &accelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2(_physicalDevice, &deviceFeatures2);
}

void VulkanBackEnd::CreateSwapchain() {

    _currentWindowExtent.width = BackEnd::GetCurrentWindowWidth();
    _currentWindowExtent.height = BackEnd::GetCurrentWindowHeight();

    VkSurfaceFormatKHR format;
    format.colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    format.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

    vkb::SwapchainBuilder swapchainBuilder(_physicalDevice, _device, _surface);
    swapchainBuilder.set_desired_format(format);
    swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
    swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
    swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
    swapchainBuilder.set_desired_extent(_currentWindowExtent.width, _currentWindowExtent.height);
    swapchainBuilder.set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT); // added so you can blit into the swapchain

    vkb::Swapchain vkbSwapchain = swapchainBuilder.build().value();
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swachainImageFormat = vkbSwapchain.image_format;
}

void VulkanBackEnd::CreateCommandBuffers()
{
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.pNext = nullptr;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = _frames[i]._commandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.pNext = nullptr;
        VK_CHECK(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &_frames[i]._commandBuffer));
    }

    //create command pool for upload context
    VkCommandPoolCreateInfo uploadCommandPoolInfo = {};
    uploadCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    uploadCommandPoolInfo.flags = 0;
    uploadCommandPoolInfo.pNext = nullptr;

    //create command buffer for upload context
    VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = _uploadContext._commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.pNext = nullptr;
    VK_CHECK(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &_uploadContext._commandBuffer));
}


void VulkanBackEnd::CreateSyncStructures() {

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    fenceCreateInfo.pNext = nullptr;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.flags = 0;
    semaphoreCreateInfo.pNext = nullptr;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
    }

    VkFenceCreateInfo uploadFenceCreateInfo = {};
    uploadFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    uploadFenceCreateInfo.flags = 0;
    uploadFenceCreateInfo.pNext = nullptr;

    VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
}

void VulkanBackEnd::CreateSampler() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 12.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.maxAnisotropy = _gpuProperties.limits.maxSamplerAnisotropy;;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.pNext = nullptr;
    vkCreateSampler(_device, &samplerInfo, nullptr, &_sampler);
}

void VulkanBackEnd::AddDebugName(VkBuffer buffer, const char* name) {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
    nameInfo.objectHandle = (uint64_t)buffer;
    nameInfo.pObjectName = name;
    vkSetDebugUtilsObjectNameEXT(_device, &nameInfo);
}

void VulkanBackEnd::AddDebugName(VkDescriptorSetLayout descriptorSetLayout, const char* name) {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
    nameInfo.objectHandle = (uint64_t)descriptorSetLayout;
    nameInfo.pObjectName = name;
    vkSetDebugUtilsObjectNameEXT(_device, &nameInfo);
}

uint64_t VulkanBackEnd::GetBufferDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(_device, &bufferDeviceAI);
}

void VulkanBackEnd::CreateAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferCreateInfo, &vmaallocInfo, &accelerationStructure.buffer._buffer, &accelerationStructure.buffer._allocation, nullptr));
    AddDebugName(accelerationStructure.buffer._buffer, "Acceleration Structure Buffer");
}


RayTracingScratchBuffer VulkanBackEnd::CreateScratchBuffer(VkDeviceSize size) {
    RayTracingScratchBuffer scratchBuffer{};

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VK_CHECK(vmaCreateBuffer(_allocator, &bufferCreateInfo, &vmaallocInfo, &scratchBuffer.handle._buffer, &scratchBuffer.handle._allocation, nullptr));

    VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.handle._buffer;
    scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(_device, &bufferDeviceAddressInfo);

    return scratchBuffer;
}

AccelerationStructure VulkanBackEnd::CreateBottomLevelAccelerationStructure(Mesh& mesh) {

    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

    uint32_t vertexAddressOffset = mesh.baseVertex * sizeof(Vertex);
    uint32_t indexAddressOffset = mesh.baseIndex * sizeof(uint32_t);

    vertexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(_mainVertexBuffer._buffer) + vertexAddressOffset;
    indexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(_mainIndexBuffer._buffer) + indexAddressOffset;

    // Build
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry {};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
    accelerationStructureGeometry.geometry.triangles.maxVertex = mesh.vertexCount;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

    // Get size info
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    const uint32_t numTriangles = mesh.indexCount / 3;
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        _device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &numTriangles,
        &accelerationStructureBuildSizesInfo);

    AccelerationStructure bottomLevelAS{};
    CreateAccelerationStructureBuffer(bottomLevelAS, accelerationStructureBuildSizesInfo);

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer._buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(_device, &accelerationStructureCreateInfo, nullptr, &bottomLevelAS.handle);

    // Create a small scratch buffer used during build of the bottom level acceleration structure
    RayTracingScratchBuffer scratchBuffer = CreateScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    ImmediateSubmit([=](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(
            cmd,
            1,
            &accelerationBuildGeometryInfo,
            accelerationBuildStructureRangeInfos.data());
    });

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.handle;
    bottomLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(_device, &accelerationDeviceAddressInfo);

    vmaDestroyBuffer(_allocator, scratchBuffer.handle._buffer, scratchBuffer.handle._allocation);

    return bottomLevelAS;
}

/*
void VulkanBackEnd::UploadMesh(VulkanMesh& mesh) {

    // Vertices
    {
        const size_t bufferSize = mesh._vertexCount * sizeof(Vertex);
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
        memcpy(data, VulkanAssetManager::GetMeshVertexPointer(mesh._vertexOffset), mesh._vertexCount * sizeof(Vertex));
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);

        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.pNext = nullptr;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &mesh._vertexBuffer._buffer, &mesh._vertexBuffer._allocation, nullptr));

        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy);
        });

        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }

    // Indices
    if (mesh._indexCount > 0)
    {
        const size_t bufferSize = mesh._indexCount * sizeof(uint32_t);
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);

        memcpy(data, VulkanAssetManager::GetMeshIndicePointer(mesh._indexOffset), mesh._indexCount * sizeof(uint32_t));
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);

        VkBufferCreateInfo indexBufferInfo = {};
        indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferInfo.pNext = nullptr;
        indexBufferInfo.size = bufferSize;
        indexBufferInfo.usage =
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateBuffer(_allocator, &indexBufferInfo, &vmaallocInfo, &mesh._indexBuffer._buffer, &mesh._indexBuffer._allocation, nullptr));

        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._indexBuffer._buffer, 1, &copy);
        });

        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }
    // Transforms
    {
        VkTransformMatrixKHR transformMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        };

        const size_t bufferSize = sizeof(transformMatrix);;
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
        memcpy(data, &transformMatrix, bufferSize);
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;// VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &mesh._transformBuffer._buffer, &mesh._transformBuffer._allocation, nullptr));
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._transformBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }
    mesh._uploadedToGPU = true;
}*/

void VulkanBackEnd::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VkCommandBuffer cmd = _uploadContext._commandBuffer;

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    commandBufferBeginInfo.pNext = nullptr;

    VK_CHECK(vkBeginCommandBuffer(cmd, &commandBufferBeginInfo));

    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.pNext = nullptr;

    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _uploadContext._uploadFence));
    vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
    vkResetFences(_device, 1, &_uploadContext._uploadFence);
    vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

bool VulkanBackEnd::StillLoading() {
    return _thereAreStillAssetsToLoad;
}

void VulkanBackEnd::PrepareSwapchainForPresent(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex) {
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    VkImageMemoryBarrier swapChainBarrier = {};
    swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapChainBarrier.image = _swapchainImages[swapchainImageIndex];
    swapChainBarrier.subresourceRange = range;
    swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    swapChainBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
}

void VulkanBackEnd::RecreateDynamicSwapchain() {
    while (_currentWindowExtent.width == 0 || _currentWindowExtent.height == 0) {
        _currentWindowExtent.width = BackEnd::GetCurrentWindowWidth();
        _currentWindowExtent.height = BackEnd::GetCurrentWindowHeight();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(_device);
    for (int i = 0; i < _swapchainImages.size(); i++) {
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    CreateSwapchain();
}

uint32_t alignedSize(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanBackEnd::UploadVertexData(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

    if (_mainVertexBuffer._buffer != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);                                                                  // This feels fucked
        vmaDestroyBuffer(_allocator, _mainVertexBuffer._buffer, _mainVertexBuffer._allocation);
    }
    if (_mainIndexBuffer._buffer != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);                                                                  // This feels fucked
        vmaDestroyBuffer(_allocator, _mainIndexBuffer._buffer, _mainIndexBuffer._allocation);
    }

    /* Vertices */ {

        const size_t bufferSize = vertices.size() * sizeof(Vertex);
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
        AddDebugName(stagingBuffer._buffer, "stagingBuffer");
        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
        memcpy(data, vertices.data(), bufferSize);
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);
        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.pNext = nullptr;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &_mainVertexBuffer._buffer, &_mainVertexBuffer._allocation, nullptr));
        AddDebugName(_mainVertexBuffer._buffer, "Main Vertex Buffer");
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _mainVertexBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        int objectCount = 1;
    }

    /* Indices */ {

        const size_t bufferSize = indices.size() * sizeof(uint32_t);
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
        AddDebugName(stagingBuffer._buffer, "stagingBufferIndices");
        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
        memcpy(data, indices.data(), bufferSize);
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = bufferSize;
        bufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_mainIndexBuffer._buffer, &_mainIndexBuffer._allocation, nullptr));
        AddDebugName(_mainIndexBuffer._buffer, "Main Index Buffer");
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _mainIndexBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }
}

void VulkanBackEnd::UploadWeightedVertexData(std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices) {

    /*
    if (_mainSkinnedVertexBuffer._buffer != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);                                                                  // This feels fucked
        vmaDestroyBuffer(_allocator, _mainSkinnedVertexBuffer._buffer, _mainSkinnedVertexBuffer._allocation);
    }
    if (_mainSkinnedIndexBuffer._buffer != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);                                                                  // This feels fucked
        vmaDestroyBuffer(_allocator, _mainSkinnedIndexBuffer._buffer, _mainSkinnedIndexBuffer._allocation);
    }*/

    /* Vertices */ {

        const size_t bufferSize = vertices.size() * sizeof(WeightedVertex);
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
        AddDebugName(stagingBuffer._buffer, "stagingBuffer");
        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
        memcpy(data, vertices.data(), bufferSize);
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);
        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.pNext = nullptr;
        vertexBufferInfo.size = bufferSize;
        vertexBufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &_mainWeightedVertexBuffer._buffer, &_mainWeightedVertexBuffer._allocation, nullptr));
        AddDebugName(_mainWeightedVertexBuffer._buffer, "Main Weighted Vertex Buffer");
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _mainWeightedVertexBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        int objectCount = 1;
    }

    /* Indices */ {

        const size_t bufferSize = indices.size() * sizeof(uint32_t);
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
        AddDebugName(stagingBuffer._buffer, "stagingBufferIndices");
        void* data;
        vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
        memcpy(data, indices.data(), bufferSize);
        vmaUnmapMemory(_allocator, stagingBuffer._allocation);
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = bufferSize;
        bufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_mainWeightedIndexBuffer._buffer, &_mainWeightedIndexBuffer._allocation, nullptr));
        AddDebugName(_mainWeightedIndexBuffer._buffer, "Main Weighted Index Buffer");
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _mainWeightedIndexBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }
/*
    std::cout << "Weighted vertex uploaded to Vulkan GPU brain\n\n\n";


    std::cout << "\n\nVERTICES\n";

    for (int i = 0; i < 50; i++) {
        std::cout << i << ": " << Util::Vec3ToString(vertices[i].position) << "\n";
    }

    std::cout << "\n\INDICES\n";

    for (int i = 0; i < 50; i++) {
        std::cout << i << ": " << indices[i] << "\n";
    }
    std::cout << "\n\\n";*/

}


//                      //
//      Raytracing      //
//                      //

void VulkanBackEnd::InitRayTracing() {

    std::cout << "Raytracing init successful\n";

    AssetManager::CreateMeshBLAS();

    std::vector<VkDescriptorSetLayout> rtDescriptorSetLayouts = {
        VulkanRenderer::GetDynamicDescriptorSet().layout,
        VulkanRenderer::GetAllTexturesDescriptorSet().layout,
        VulkanRenderer::GetRenderTargetsDescriptorSet().layout,
        VulkanRenderer::GetRaytracingDescriptorSet().layout
    };

    VulkanRenderer::GetRaytracer().CreateRaytracingPipeline(_device, rtDescriptorSetLayouts, 1);
    VulkanRenderer::GetRaytracer().CreateShaderBindingTable(_device, _allocator, _rayTracingPipelineProperties);
    
    std::cout << "init raytracing\n";
}

VkTransformMatrixKHR GlmMat4ToVkTransformMatrix(glm::mat4 matrix) {
    matrix = glm::transpose(matrix);
    VkTransformMatrixKHR vkTransformMatrix;
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            vkTransformMatrix.matrix[x][y] = matrix[x][y];
        }
    }
    return vkTransformMatrix;
}

std::vector<VkAccelerationStructureInstanceKHR> VulkanBackEnd::CreateTLASInstancesFromRenderItems(std::vector<RenderItem3D>& renderItems) {

    int instanceCustomIndex = 0;
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    for (RenderItem3D& renderItem : renderItems) {

        // Replace this with a bit flag maybe? cause castShadow is not even what this checking here.
        if (!renderItem.castShadow) {
            continue;
        }

        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        VkAccelerationStructureInstanceKHR& instance = instances.emplace_back(VkAccelerationStructureInstanceKHR());
        instance.transform = GlmMat4ToVkTransformMatrix(renderItem.modelMatrix);
        instance.instanceCustomIndex = instanceCustomIndex;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
        instance.accelerationStructureReference = mesh->accelerationStructure.deviceAddress;
        instanceCustomIndex++;
    }
    return instances;
}

void VulkanBackEnd::CreateTopLevelAccelerationStructure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS) {

    if (instances.size() == 0) {
        return;
    }

    const size_t bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.pNext = nullptr;
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    AllocatedBuffer stagingBuffer;

    VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
    AddDebugName(stagingBuffer._buffer, "stagingBufferTLAS");
    void* data;
    vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
    memcpy(data, instances.data(), bufferSize);
    vmaUnmapMemory(_allocator, stagingBuffer._allocation);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_rtInstancesBuffer._buffer, &_rtInstancesBuffer._allocation, nullptr));
    AddDebugName(_rtInstancesBuffer._buffer, "_rtInstancesBuffer");

    ImmediateSubmit([=](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = bufferSize;
        vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtInstancesBuffer._buffer, 1, &copy);
    });
    vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(_rtInstancesBuffer._buffer);

    VkAccelerationStructureGeometryInstancesDataKHR tlasInstancesInfo = {};
    tlasInstancesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasInstancesInfo.data.deviceAddress = GetBufferDeviceAddress(_rtInstancesBuffer._buffer);

    VkAccelerationStructureGeometryKHR  accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances = tlasInstancesInfo;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &accelerationStructureGeometry;

    const uint32_t numInstances = static_cast<uint32_t>(instances.size());

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        _device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &numInstances,
        &accelerationStructureBuildSizesInfo);

    CreateAccelerationStructureBuffer(outTLAS, accelerationStructureBuildSizesInfo);

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = outTLAS.buffer._buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(_device, &accelerationStructureCreateInfo, nullptr, &outTLAS.handle);

    // Create a small scratch buffer used during build of the top level acceleration structure
    RayTracingScratchBuffer scratchBuffer = CreateScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = outTLAS.handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numInstances;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    // This seems suspect AF
    // This seems suspect AF
    // This seems suspect AF
    vkDeviceWaitIdle(_device);
    // This seems suspect AF
    // This seems suspect AF
    // This seems suspect AF

    // Build the acceleration structure on the device via a one-time command buffer submission
    // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
    ImmediateSubmit([=](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(
            cmd,
            1,
            &accelerationBuildGeometryInfo,
            accelerationBuildStructureRangeInfos.data());
    });

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = outTLAS.handle;
    outTLAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(_device, &accelerationDeviceAddressInfo);

    vmaDestroyBuffer(_allocator, scratchBuffer.handle._buffer, scratchBuffer.handle._allocation);

    // deal with this
    vmaDestroyBuffer(_allocator, _rtInstancesBuffer._buffer, _rtInstancesBuffer._allocation);
}


//                       //
//      Point Cloud      //
//                       //

void VulkanBackEnd::CreatePointCloudVertexBuffer(std::vector<CloudPoint>& pointCloud) {
    if (pointCloud.empty()) {
        return;
    }
    const size_t bufferSize = sizeof(CloudPoint) * pointCloud.size();
    _pointCloudBuffer.Create(_allocator, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.pNext = nullptr;
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    AllocatedBuffer stagingBuffer;
    VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
    AddDebugName(stagingBuffer._buffer, "stagingBuffer");

    void* data;
    vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
    memcpy(data, pointCloud.data(), bufferSize);
    vmaUnmapMemory(_allocator, stagingBuffer._allocation);

    ImmediateSubmit([=](VkCommandBuffer cmd) {
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = bufferSize;
        vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _pointCloudBuffer.buffer, 1, &copy);
    });

}

Buffer* VulkanBackEnd::GetPointCloudBuffer() {
    return &_pointCloudBuffer;
}

void VulkanBackEnd::DestroyPointCloudBuffer() {
    _pointCloudBuffer.Destroy(_allocator);
}


//                     //
//      Callbacks      //
//                     //

void VulkanBackEnd::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    _frameBufferResized = true;
}




void VulkanBackEnd::AllocateSkinnedVertexBufferSpace(int vertexCount) {

    // Check if there is enough space
    if (g_allocatedSkinnedVertexBufferSize < vertexCount * sizeof(Vertex)) {

        // Destroy old buffer
        if (g_mainSkinnedVertexBuffer._buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(_allocator, g_mainSkinnedVertexBuffer._buffer, g_mainSkinnedVertexBuffer._allocation);
        }
        // Create new one
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.pNext = nullptr;
        vertexBufferInfo.size = vertexCount * sizeof(Vertex);
        vertexBufferInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(VulkanBackEnd::GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &g_mainSkinnedVertexBuffer._buffer, &g_mainSkinnedVertexBuffer._allocation, nullptr));
        VulkanBackEnd::AddDebugName(g_mainSkinnedVertexBuffer._buffer, "Detached Vertex Buffer");
        g_allocatedSkinnedVertexBufferSize = vertexCount * sizeof(Vertex);
    }
}