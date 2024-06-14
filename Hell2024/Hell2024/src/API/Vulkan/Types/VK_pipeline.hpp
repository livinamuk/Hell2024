#pragma once
#include <glm/glm.hpp>
#include <string>
#include "../VK_common.h"

struct Pipeline {

	VkPipeline _handle;
	VkPipelineLayout _layout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
	VertexInputDescription _vertexDescription;
	VkPrimitiveTopology _topology;
	VkPolygonMode _polygonMode;
	VkCullModeFlags _cullModeFlags;
	VkCompareOp _compareOp;
	VkFormat _depthFormat = VK_FORMAT_D32_SFLOAT;
	VkFormat _stencilFormat = VK_FORMAT_UNDEFINED;
	std::string _debugName = "NO_NAME";
	uint32_t m_pushConstantCount = 0;
	uint32_t m_pushConstantSize = 0;

	bool _colorBlendEnable = false;
	bool _depthWrite = false;
	bool _depthTest = false;

	void PushDescriptorSetLayout(VkDescriptorSetLayout layout) {
		_descriptorSetLayouts.push_back(layout);
	}

	void CreatePipelineLayout(VkDevice device) {

        if (_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, _layout, nullptr);
        }

        VkPushConstantRange pushConstantRange;
        pushConstantRange.offset = 0;
        pushConstantRange.size = m_pushConstantSize;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		/*VkPushConstantRange pushConstantRange;
		pushConstantRange.offset = 0;
		pushConstantRange.size = _pushConstantSize;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;*/

		VkPipelineLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = (uint32_t)_descriptorSetLayouts.size();
		createInfo.pSetLayouts = _descriptorSetLayouts.data();
		createInfo.pPushConstantRanges = &pushConstantRange;
		createInfo.pushConstantRangeCount = m_pushConstantCount;
		createInfo.pNext = nullptr;
 		VK_CHECK(vkCreatePipelineLayout(device, &createInfo, nullptr, &_layout));
	}

	void Cleanup(VkDevice device) {
		vkDestroyPipeline(device, _handle, nullptr);
		vkDestroyPipelineLayout(device, _layout, nullptr);
	}

    void SetVertexDescription(VertexDescriptionType type) {

	    // Main binding
	    _vertexDescription.Reset();
	    VkVertexInputBindingDescription mainBinding = {};
	    mainBinding.binding = 0;
        if (type != VertexDescriptionType::ALL_WEIGHTED) {
            mainBinding.stride = sizeof(Vertex);
        }
        else {
            mainBinding.stride = sizeof(WeightedVertex);
        }
	    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	    _vertexDescription.bindings.push_back(mainBinding);

        // Position
        VkVertexInputAttributeDescription positionAttribute = {};
        positionAttribute.binding = 0;
        positionAttribute.location = 0;
        positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        positionAttribute.offset = offsetof(Vertex, position);

        // Normals
        VkVertexInputAttributeDescription normalAttribute = {};
        normalAttribute.binding = 0;
        normalAttribute.location = 1;
        normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        normalAttribute.offset = offsetof(Vertex, normal);

        // UVs
        VkVertexInputAttributeDescription uvAttribute = {};
        uvAttribute.binding = 0;
        uvAttribute.location = 2;
        uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
        uvAttribute.offset = offsetof(Vertex, uv);

        // Tangent
        VkVertexInputAttributeDescription tangentAttribute = {};
        tangentAttribute.binding = 0;
        tangentAttribute.location = 3;
        tangentAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        tangentAttribute.offset = offsetof(Vertex, tangent);

        // BoneID
        VkVertexInputAttributeDescription boneIDAttribute = {};
        boneIDAttribute.binding = 0;
        boneIDAttribute.location = 4;
        boneIDAttribute.format = VK_FORMAT_R32G32B32A32_SINT;
        boneIDAttribute.offset = offsetof(WeightedVertex, boneID);

        // Weight
        VkVertexInputAttributeDescription weightAttribute = {};
        weightAttribute.binding = 0;
        weightAttribute.location = 5;
        weightAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        weightAttribute.offset = offsetof(WeightedVertex, weight);

		if (type == VertexDescriptionType::POSITION_NORMAL_TEXCOORD) {
			_vertexDescription.attributes.push_back(positionAttribute);
			_vertexDescription.attributes.push_back(normalAttribute);
			_vertexDescription.attributes.push_back(uvAttribute);
        }
        else if (type == VertexDescriptionType::POSITION_NORMAL) {
            _vertexDescription.attributes.push_back(positionAttribute);
            _vertexDescription.attributes.push_back(normalAttribute);
        }
        else if (type == VertexDescriptionType::POSITION_TEXCOORD) {
            _vertexDescription.attributes.push_back(positionAttribute);
            _vertexDescription.attributes.push_back(uvAttribute);
        }
        else if (type == VertexDescriptionType::POSITION) {
            _vertexDescription.attributes.push_back(positionAttribute);
        }
        else if (type == VertexDescriptionType::ALL) {
            _vertexDescription.attributes.push_back(positionAttribute);
            _vertexDescription.attributes.push_back(normalAttribute);
            _vertexDescription.attributes.push_back(uvAttribute);
            _vertexDescription.attributes.push_back(tangentAttribute);
        }
        else if (type == VertexDescriptionType::ALL_WEIGHTED) {
            _vertexDescription.attributes.push_back(positionAttribute);
            _vertexDescription.attributes.push_back(normalAttribute);
            _vertexDescription.attributes.push_back(uvAttribute);
            _vertexDescription.attributes.push_back(tangentAttribute);
            _vertexDescription.attributes.push_back(boneIDAttribute);
            _vertexDescription.attributes.push_back(weightAttribute);
        }
	}

	void SetTopology(VkPrimitiveTopology topology) {
		_topology = topology;
	}

	void SetCullModeFlags(VkCullModeFlags cullModeFlags) {
		_cullModeFlags = cullModeFlags;
	}

	void SetPolygonMode(VkPolygonMode polygonMode) {
		_polygonMode = polygonMode;
	}

	void SetColorBlending(bool enabled) {
		_colorBlendEnable = enabled;
	}

	void SetDepthWrite(bool enabled) {
		_depthWrite = enabled;
	}

	void SetDepthTest(bool enabled) {
		_depthTest = enabled;
	}

	void SetCompareOp(VkCompareOp op) {
		_compareOp = op;
	}

    void SetPushConstantSize(uint32_t bufferSize) {
        m_pushConstantSize = bufferSize;
        m_pushConstantCount = 1;
    }

    void Build(VkDevice device, VkShaderModule vertexShader, VkShaderModule fragmentShader, std::vector<VkFormat> colorAttachmentFormats) {

        int colorAttachmentCount = colorAttachmentFormats.size();

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.pNext = nullptr;
        vertexInputCreateInfo.pVertexAttributeDescriptions = _vertexDescription.attributes.data();
		vertexInputCreateInfo.vertexAttributeDescriptionCount = (uint32_t)_vertexDescription.attributes.size();
		vertexInputCreateInfo.pVertexBindingDescriptions = _vertexDescription.bindings.data();
		vertexInputCreateInfo.vertexBindingDescriptionCount = (uint32_t)_vertexDescription.bindings.size();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.pNext = nullptr;
		inputAssemblyCreateInfo.topology = _topology;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
		rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationCreateInfo.pNext = nullptr;
		rasterizationCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationCreateInfo.polygonMode = _polygonMode;
		rasterizationCreateInfo.lineWidth = 1.0f;
		rasterizationCreateInfo.cullMode = _cullModeFlags;
        rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;// VK_FRONT_FACE_COUNTER_CLOCKWISE VK_FRONT_FACE_CLOCKWISE
		rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationCreateInfo.depthBiasClamp = 0.0f;
		rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.pNext = nullptr;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.pSampleMask = nullptr;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.pNext = nullptr;
		depthStencilCreateInfo.depthTestEnable = _depthTest ? VK_TRUE : VK_FALSE;
		depthStencilCreateInfo.depthWriteEnable = _depthWrite ? VK_TRUE : VK_FALSE;
		depthStencilCreateInfo.depthCompareOp = _compareOp;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

		VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderCreateInfo.pNext = nullptr;
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderCreateInfo.module = vertexShader;
		vertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderCreateInfo.pNext = nullptr;
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderCreateInfo.module = fragmentShader;
		fragmentShaderCreateInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.push_back(vertexShaderCreateInfo);
		shaderStages.push_back(fragmentShaderCreateInfo);

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
		colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingCreateInfo.pNext = nullptr;
		colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendingCreateInfo.attachmentCount = colorAttachmentFormats.size();
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		for (int i = 0; i < colorAttachmentFormats.size(); i++) {
			VkPipelineColorBlendAttachmentState attachmentState = {};
			attachmentState.colorWriteMask = 0xF;
			//attachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachmentState.blendEnable = _colorBlendEnable;// VK_TRUE;
			attachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			attachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendAttachmentStates.push_back(attachmentState);
		}
		colorBlendingCreateInfo.pAttachments = blendAttachmentStates.data();

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };// , VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineInfo.pColorBlendState = &colorBlendingCreateInfo;
		pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineInfo.layout = _layout;
		pipelineInfo.renderPass = nullptr;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.pDynamicState = &dynamicState;

		// New create info to define color, depth and stencil attachments at pipeline create time
		std::vector<VkFormat> formats;
		for (int i = 0; i < colorAttachmentFormats.size(); i++) {
			formats.push_back(colorAttachmentFormats[i]);
		}
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipelineRenderingCreateInfo.colorAttachmentCount = colorAttachmentCount;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = formats.data();
		pipelineRenderingCreateInfo.depthAttachmentFormat = _depthFormat;
		pipelineRenderingCreateInfo.stencilAttachmentFormat = _stencilFormat;
		pipelineInfo.pNext = &pipelineRenderingCreateInfo;

		VkPipeline newPipeline;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
			std::cout << "failed to create pipeline\n";
			_handle = VK_NULL_HANDLE;
		}
		else {
			if (_handle != VK_NULL_HANDLE) {
				vkDestroyPipeline(device, _handle, nullptr);
			}
			_handle = newPipeline;
		}

		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		nameInfo.objectHandle = (uint64_t)_handle;
		nameInfo.pObjectName = _debugName.c_str();
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
		vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
	}
};