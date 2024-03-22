#include "VK_backEnd.h"
#include "VK_assetManager.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VK_Renderer.h"
#include "../../Core/AssetManager.h"
#include "../../BackEnd/BackEnd.h"

namespace VulkanBackEnd {

    /*struct VulkanRenderTargets {
        VulkanRenderTarget loadingScreen;
    } _renderTargets;
    */

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
    //GLFWwindow* _window;
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

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
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


    //void UploadUnsubmittedTextures();
    void CreateBuffers();
    void LoadShaders();
    void CreatePipelines();;
    void CreateRenderTargets();
    void CreateRayTracingBuffers();

    void UploadUnsubmittedMeshes();
    void UploadMesh(VulkanMesh& mesh);

    void FramebufferResizeCallback(GLFWwindow* window, int width, int height); 

    //void BlitRenderTargetIntoSwapChain(VkCommandBuffer commandBuffer, Vulkan::RenderTarget& renderTarget, uint32_t swapchainImageIndex);
    
    

    uint64_t GetBufferDeviceAddress(VkBuffer buffer);

    RayTracingScratchBuffer CreateScratchBuffer(VkDeviceSize size);
    AccelerationStructure CreateBottomLevelAccelerationStructure(VulkanMesh* mesh);
    void CreateAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);	
    void CreateTopLevelAccelerationStructure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS);
    void InitRayTracing();

    //void UpdateStaticDescriptorSet();

    //void BuildRayTracingCommandBuffers(int swapchainIndex);
    inline AllocatedBuffer _rtVertexBuffer;
    inline AllocatedBuffer _rtIndexBuffer;
    //inline AllocatedBuffer _mousePickResultBuffer;
    //inline AllocatedBuffer _mousePickResultCPUBuffer; // for mouse picking
    inline uint32_t _rtIndexCount;
    inline VkDeviceOrHostAddressConstKHR _vertexBufferDeviceAddress{};
    inline VkDeviceOrHostAddressConstKHR _indexBufferDeviceAddress{};
    inline VkDeviceOrHostAddressConstKHR _transformBufferDeviceAddress{};
    inline AllocatedBuffer _rtInstancesBuffer;

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
    VulkanRenderer::CreateMinimumRenderTargets();

    CreateCommandBuffers();
    CreateSyncStructures();
    CreateSampler();

    VulkanRenderer::CreateDescriptorSets();
    VulkanRenderer::CreatePipelinesMinimum();

    VulkanAssetManager::LoadFont(_device, _allocator);
    VulkanAssetManager::LoadHardCodedMesh();

    UploadUnsubmittedMeshes();

    VulkanRenderer::UpdateFixedDescriptorSetMinimum();

    CreateBuffers();
}

void Init() {

}

void VulkanBackEnd::LoadShaders() {

    //LoadShader(_device, "solid_color.vert", VK_SHADER_STAGE_VERTEX_BIT, &_solid_color_vertex_shader);
    //LoadShader(_device, "solid_color.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_solid_color_fragment_shader);

    //LoadShader(_device, "gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT, &_gbuffer_vertex_shader);
    //LoadShader(_device, "gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_gbuffer_fragment_shader);

    //LoadShader(_device, "composite.vert", VK_SHADER_STAGE_VERTEX_BIT, &_composite_vertex_shader);
    //LoadShader(_device, "composite.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_composite_fragment_shader);

    //_raytracer.LoadShaders(_device, "raygen.rgen", "miss.rmiss", "shadow.rmiss", "closesthit.rchit");
    //_raytracerPath.LoadShaders(_device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit");
    //_raytracerMousePick.LoadShaders(_device, "mousepick_raygen.rgen", "mousepick_miss.rmiss", "path_shadow.rmiss", "mousepick_closesthit.rchit");
}

void VulkanBackEnd::CreatePipelines() {

    // Composite pipeline
    /*_pipelines.composite.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
    _pipelines.composite.PushDescriptorSetLayout(_staticDescriptorSet.layout);
    _pipelines.composite.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
    _pipelines.composite.CreatePipelineLayout(_device);
    _pipelines.composite.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
    _pipelines.composite.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.composite.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.composite.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.composite.SetColorBlending(false);
    _pipelines.composite.SetDepthTest(false);
    _pipelines.composite.SetDepthWrite(false);
    _pipelines.composite.Build(_device, _composite_vertex_shader, _composite_fragment_shader, 1);

    // Lines pipeline
    _pipelines.lines.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
    _pipelines.lines.PushDescriptorSetLayout(_staticDescriptorSet.layout);
    _pipelines.lines.SetPushConstantSize(sizeof(LineShaderPushConstants));
    _pipelines.lines.SetPushConstantCount(1);
    _pipelines.lines.CreatePipelineLayout(_device);
    _pipelines.lines.SetVertexDescription(VertexDescriptionType::POSITION);
    _pipelines.lines.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    _pipelines.lines.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.lines.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.lines.SetColorBlending(false);
    _pipelines.lines.SetDepthTest(false);
    _pipelines.lines.SetDepthWrite(false);
    _pipelines.lines.Build(_device, _solid_color_vertex_shader, _solid_color_fragment_shader, 1);

    // Denoise A pipeline
    _pipelines.denoisePassA.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
    _pipelines.denoisePassA.CreatePipelineLayout(_device);
    _pipelines.denoisePassA.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
    _pipelines.denoisePassA.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.denoisePassA.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.denoisePassA.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.denoisePassA.SetColorBlending(false);
    _pipelines.denoisePassA.SetDepthTest(false);
    _pipelines.denoisePassA.SetDepthWrite(false);
    _pipelines.denoisePassA.Build(_device, _denoise_pass_A_vertex_shader, _denoise_pass_A_fragment_shader, 1);

    // Denoise B pipeline
    _pipelines.denoisePassB.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
    _pipelines.denoisePassB.CreatePipelineLayout(_device);
    _pipelines.denoisePassB.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
    _pipelines.denoisePassB.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.denoisePassB.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.denoisePassB.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.denoisePassB.SetColorBlending(false);
    _pipelines.denoisePassB.SetDepthTest(false);
    _pipelines.denoisePassB.SetDepthWrite(false);
    _pipelines.denoisePassB.Build(_device, _denoise_pass_B_vertex_shader, _denoise_pass_B_fragment_shader, 1);

    // Denoise C pipeline
    _pipelines.denoisePassC.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
    _pipelines.denoisePassC.CreatePipelineLayout(_device);
    _pipelines.denoisePassC.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
    _pipelines.denoisePassC.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.denoisePassC.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.denoisePassC.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.denoisePassC.SetColorBlending(false);
    _pipelines.denoisePassC.SetDepthTest(false);
    _pipelines.denoisePassC.SetDepthWrite(false);
    _pipelines.denoisePassC.Build(_device, _denoise_pass_C_vertex_shader, _denoise_pass_C_fragment_shader, 1);

    // Blur horizontal
    _pipelines.denoiseBlurHorizontal.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
    _pipelines.denoiseBlurHorizontal.CreatePipelineLayout(_device);
    _pipelines.denoiseBlurHorizontal.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
    _pipelines.denoiseBlurHorizontal.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.denoiseBlurHorizontal.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.denoiseBlurHorizontal.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.denoiseBlurHorizontal.SetColorBlending(false);
    _pipelines.denoiseBlurHorizontal.SetDepthTest(false);
    _pipelines.denoiseBlurHorizontal.SetDepthWrite(false);
    _pipelines.denoiseBlurHorizontal.Build(_device, _blur_horizontal_vertex_shader, _blur_horizontal_fragment_shader, 1);

    // Blur vertical
    _pipelines.denoiseBlurVertical.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
    _pipelines.denoiseBlurVertical.CreatePipelineLayout(_device);
    _pipelines.denoiseBlurVertical.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
    _pipelines.denoiseBlurVertical.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.denoiseBlurVertical.SetPolygonMode(VK_POLYGON_MODE_FILL);
    _pipelines.denoiseBlurVertical.SetCullModeFlags(VK_CULL_MODE_NONE);
    _pipelines.denoiseBlurVertical.SetColorBlending(false);
    _pipelines.denoiseBlurVertical.SetDepthTest(false);
    _pipelines.denoiseBlurVertical.SetDepthWrite(false);
    _pipelines.denoiseBlurVertical.Build(_device, _blur_vertical_vertex_shader, _blur_vertical_fragment_shader, 1);
    */

    // Raster pipeline
    /* {
        // Text blitter pipeline layout
        _rasterPipeline.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
        _rasterPipeline.PushDescriptorSetLayout(_staticDescriptorSet.layout);
        _rasterPipeline.CreatePipelineLayout(_device);

        VertexInputDescription vertexDescription = Util::get_vertex_description();
        PipelineBuilder pipelineBuilder;
        pipelineBuilder._pipelineLayout = _rasterPipeline.layout;
        pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
        pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE); // VK_CULL_MODE_NONE
        pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
        pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(false);
        pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
        pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
        pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
        pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, _gbuffer_vertex_shader));
        pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, _gbuffer_fragment_shader));

        _rasterPipeline.handle = pipelineBuilder.build_dynamic_rendering_pipeline(_device, _depthFormat, 3);
    }*/
}


/*
void VulkanBackEnd::SetWindowPointer(GLFWwindow* window) {
    _window = window;
}*/

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

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    enabledRayTracingPipelineFeatures.pNext = nullptr;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
    enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

    VkPhysicalDeviceFeatures features = {};
    features.samplerAnisotropy = true;
    features.shaderInt64 = true;
    selector.set_required_features(features);

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.shaderSampledImageArrayNonUniformIndexing = true;
    features12.runtimeDescriptorArray = true;
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorIndexing = true;
    features12.bufferDeviceAddress = true;
    selector.set_required_features_12(features12);

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.maintenance4 = true;
    features13.dynamicRendering = true;
    features13.pNext = &enabledAccelerationStructureFeatures;	// you probably want to confirm this chaining of pNexts works when shit goes wrong.
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
    bool maintenance4ExtensionSupported = false;
    for (const auto& extension : availableExtensions) {
        if (std::string(extension.extensionName) == VK_KHR_MAINTENANCE_4_EXTENSION_NAME) {
            maintenance4ExtensionSupported = true;
            //std::cout << "VK_KHR_maintenance4 is supported\n";
            break;
        }
    }
    // Specify the extensions to enable
    std::vector<const char*> enabledExtensions = {
        VK_KHR_MAINTENANCE_4_EXTENSION_NAME
    };
    if (!maintenance4ExtensionSupported) {
        throw std::runtime_error("Required extension not supported");
    }

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
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(_device, "vkCmdTraceRaysKHR"));
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

void VulkanBackEnd::UploadUnsubmittedMeshes() {
    for (VulkanMesh& mesh : VulkanAssetManager::GetMeshList()) {
        if (!mesh._uploadedToGPU) {
            UploadMesh(mesh);
            mesh._accelerationStructure = CreateBottomLevelAccelerationStructure(&mesh);
        }
    }
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

AccelerationStructure VulkanBackEnd::CreateBottomLevelAccelerationStructure(VulkanMesh* mesh) {
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(mesh->_vertexBuffer._buffer);
    indexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(mesh->_indexBuffer._buffer);
    transformBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(mesh->_transformBuffer._buffer);

    // Build
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
    accelerationStructureGeometry.geometry.triangles.maxVertex = mesh->_vertexCount;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
    accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

    // Get size info
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    const uint32_t numTriangles = mesh->_indexCount / 3;
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
}

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

void VulkanBackEnd::CreateBuffers() {

    // Dynamic
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        //_frames[i]._sceneCamDataBuffer.Create(_allocator, sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //_frames[i]._inventoryCamDataBuffer.Create(_allocator, sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _frames[i]._meshInstances2DBuffer.Create(_allocator, sizeof(RenderItem2D) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _frames[i]._meshInstancesSceneBuffer.Create(_allocator, sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //_frames[i]._meshInstancesInventoryBuffer.Create(_allocator, sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //_frames[i]._lightRenderInfoBuffer.Create(_allocator, sizeof(LightRenderInfo) * MAX_LIGHTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //_frames[i]._lightRenderInfoBufferInventory.Create(_allocator, sizeof(LightRenderInfo) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    // Static
    // none as of yet. besides TLAS but that is created elsewhere.
}

bool VulkanBackEnd::StillLoading() {
    return _thereAreStillAssetsToLoad;
}

void VulkanBackEnd::LoadNextItem() {

    // These return false if there is nothing to load
    // meaning it will work its way down this function each game loop until everything is loaded

    if (VulkanAssetManager::LoadNextTexture(_device, _allocator)) {
        return;
    }

    if (VulkanAssetManager::LoadNextModel()) {
        return;
    }
    else {
    }
    static bool shadersLoaded = false;
    static bool shadersLoadedMessageSeen = false;
    if (!shadersLoadedMessageSeen) {
        shadersLoadedMessageSeen = true;
        VulkanAssetManager::AddLoadingText("Compiling shaders...");
        return;
    }
    if (!shadersLoaded) {
        shadersLoaded = true;
        LoadShaders();
        UploadUnsubmittedMeshes();
        AssetManager::BuildMaterials();
    }

    static bool pipelinesCreated = false;
    static bool pipelinesCreatedMessageSeen = false;
    if (!pipelinesCreatedMessageSeen) {
        pipelinesCreatedMessageSeen = true;
        VulkanAssetManager::AddLoadingText("Creating pipelines...");
        return;
    }
    if (!pipelinesCreated) {
        pipelinesCreated = true;
        CreatePipelines();;
    }

    static bool renderTargetsCreated = false;
    static bool renderTargetsCreatedMessageSeen = false;
    if (!renderTargetsCreatedMessageSeen) {
        renderTargetsCreatedMessageSeen = true;
        VulkanAssetManager::AddLoadingText("Creating render targets...");
        return;
    }
    if (!renderTargetsCreated) {
        renderTargetsCreated = true;
        CreateRenderTargets();
    }

    static bool rtSetup = false;
    static bool rtSetupMSG = false;
    if (!rtSetupMSG) {
        rtSetupMSG = true;
        VulkanAssetManager::AddLoadingText("Initializing raytracing...");
        return;
    }
    if (!rtSetup) {
        rtSetup = true;
        //AssetManager::BuildMaterials();
        //Scene::Init();						// Scene::Init creates wall geometry, and thus must run before upload_meshes
        CreateRayTracingBuffers();
        //CreateTopLevelAccelerationStructure(Scene::GetMeshInstancesForSceneAccelerationStructure(), _frames[0]._sceneTLAS);
        //CreateTopLevelAccelerationStructure(Scene::GetMeshInstancesForInventoryAccelerationStructure(), _frames[0]._inventoryTLAS);
        InitRayTracing();
        //VulkanRenderer::UpdateStaticDescriptorSet();
        //Input::SetMousePos(_windowedModeExtent.width / 2, _windowedModeExtent.height / 2);
    }

    /*static bool meshUploaded = false;
    static bool meshUploadedMSG = false;
    if (!meshUploadedMSG) {
        meshUploadedMSG = true;
        AddLoadingText("Uploading mesh...");
        return;
    }
    if (!meshUploaded) {
        meshUploaded = true;
        upload_meshes();
    }*/

    _thereAreStillAssetsToLoad = false;
   // VulkanTextBlitter::ResetDebugText();
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

void VulkanBackEnd::CreateRayTracingBuffers()
{
    //std::vector<Mesh*> meshes = Scene::GetMeshList();
    //std::vector<Mesh>& meshes = AssetManager::GetMeshList();
    std::vector<VulkanVertex>& vertices = VulkanAssetManager::GetVertices_TEMPORARY();
    std::vector<uint32_t>& indices = VulkanAssetManager::GetIndices_TEMPORARY();

    /* you commented this out porting it
    {
        // RT Mesh Instance Index Buffer
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = sizeof(uint32_t) * 2;
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_mousePickResultBuffer._buffer, &_mousePickResultBuffer._allocation, nullptr));
        add_debug_name(_mousePickResultBuffer._buffer, "_mousePickResultBuffer");
        //vmaMapMemory(_allocator, _mousePickResultBuffer._allocation, &_mousePickResultBuffer._mapped);
        

        // Copy in inital values of 0
        uint32_t mousePickResult[2] = { 0, 0 };

        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = sizeof(uint32_t) * 2;
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_mousePickResultCPUBuffer._buffer, &_mousePickResultCPUBuffer._allocation, nullptr));
        add_debug_name(_mousePickResultCPUBuffer._buffer, "_mousePickResultCPUBuffer");
        vmaMapMemory(_allocator, _mousePickResultCPUBuffer._allocation, &_mousePickResultCPUBuffer._mapped);
    }*/

    // Vertices
    {
        const size_t bufferSize = vertices.size() * sizeof(VulkanVertex);
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
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &_rtVertexBuffer._buffer, &_rtVertexBuffer._allocation, nullptr));
        AddDebugName(_rtVertexBuffer._buffer, "_rtVertexBuffer");
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtVertexBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        int objectCount = 1;
    }

    // Indices
    {
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
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_rtIndexBuffer._buffer, &_rtIndexBuffer._allocation, nullptr));
        AddDebugName(_rtIndexBuffer._buffer, "_rtIndexBufferVertices");
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = bufferSize;
            vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtIndexBuffer._buffer, 1, &copy);
        });
        vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }

    _vertexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(_rtVertexBuffer._buffer);
    _indexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(_rtIndexBuffer._buffer);
}

void VulkanBackEnd::CreateTopLevelAccelerationStructure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS) {
    if (instances.size() == 0)
        return;

    // create the BLAS instances within TLAS, one for each game object
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
    //instanceDataDeviceAddresses.push_back(instanceDataDeviceAddress);

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
    //std::cout << "numInstances: " << numInstances << "\n";

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

    vkDeviceWaitIdle(_device);

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

uint32_t alignedSize(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanBackEnd::InitRayTracing() {

    /*
    std::vector<VkDescriptorSetLayout> rtDescriptorSetLayouts = { _dynamicDescriptorSet.layout, _staticDescriptorSet.layout, _samplerDescriptorSet.layout };

    _raytracer.CreatePipeline(_device, rtDescriptorSetLayouts, 2);
    _raytracer.CreateShaderBindingTable(_device, _allocator, _rayTracingPipelineProperties);

    _raytracerPath.CreatePipeline(_device, rtDescriptorSetLayouts, 2);
    _raytracerPath.CreateShaderBindingTable(_device, _allocator, _rayTracingPipelineProperties);

    _raytracerMousePick.CreatePipeline(_device, rtDescriptorSetLayouts, 2);
    _raytracerMousePick.CreateShaderBindingTable(_device, _allocator, _rayTracingPipelineProperties);*/
}



void VulkanBackEnd::CreateRenderTargets() {

    /*
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    //  Present Target
    {
        VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        _renderTargets.present = RenderTarget(_device, _allocator, format, _renderTargetPresentExtent.width, _renderTargetPresentExtent.height, usageFlags, "present render target");
    }
    int scale = 2;
    uint32_t width = _renderTargets.present._extent.width * scale;
    uint32_t height = _renderTargets.present._extent.height * scale;

    // RT image store 
    {
        VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;// VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        _renderTargets.rt_first_hit_color = RenderTarget(_device, _allocator, format, width, height, usage, "rt first hit color render target");
        _renderTargets.rt_first_hit_normals = RenderTarget(_device, _allocator, format, width, height, usage, "rt first hit normals render target");
        _renderTargets.rt_first_hit_base_color = RenderTarget(_device, _allocator, format, width, height, usage, "rt first hit base color render target");
        _renderTargets.rt_second_hit_color = RenderTarget(_device, _allocator, format, width, height, usage, "rt second hit color render target");
    }

    // GBuffer
    {
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        _renderTargets.gBufferBasecolor = RenderTarget(_device, _allocator, format, width, height, usage, "gbuffer base color render target");
        _renderTargets.gBufferNormal = RenderTarget(_device, _allocator, format, width, height, usage, "gbuffer base normal render target");
        _renderTargets.gBufferRMA = RenderTarget(_device, _allocator, format, width, height, usage, "gbuffer base rma render target");
    }
    // Laptop screen
    {
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        _renderTargets.laptopDisplay = RenderTarget(_device, _allocator, format, LAPTOP_DISPLAY_WIDTH, LAPTOP_DISPLAY_HEIGHT, usage, "laptop display render target");
    }

    //depth image size will match the window
    VkExtent3D depthImageExtent = {
        _renderTargets.present._extent.width,
        _renderTargets.present._extent.height,
        1
    };

    //hardcoding the depth format to 32 bit float
    _depthFormat = VK_FORMAT_D32_SFLOAT;

    _presentDepthTarget.Create(_device, _allocator, VK_FORMAT_D32_SFLOAT, _renderTargets.present._extent);
    _gbufferDepthTarget.Create(_device, _allocator, VK_FORMAT_D32_SFLOAT, _renderTargets.gBufferNormal._extent);

    // Blur target
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    _renderTargets.composite = RenderTarget(_device, _allocator, format, width, height, usage, "composite render target");

    float denoiseWidth = _renderTargets.rt_first_hit_color._extent.width;
    float denoiseHeight = _renderTargets.rt_first_hit_color._extent.height;
    _renderTargets.denoiseTextureA = RenderTarget(_device, _allocator, format, denoiseWidth, denoiseHeight, usage, "denoise A render target");
    _renderTargets.denoiseTextureB = RenderTarget(_device, _allocator, format, denoiseWidth, denoiseHeight, usage, "denoise B render target");
    _renderTargets.denoiseTextureC = RenderTarget(_device, _allocator, format, denoiseWidth, denoiseHeight, usage, "denoise C render target");*/
}

//////////////////////////
//                      //
//      CALLBACKS       //

void VulkanBackEnd::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    _frameBufferResized = true;
}