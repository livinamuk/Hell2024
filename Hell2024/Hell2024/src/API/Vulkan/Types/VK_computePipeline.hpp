#pragma once
#include "../VK_common.h"
#include <vector>
#include <string>
#include <iostream>

struct ComputePipeline {

private:
    VkPipeline m_handle;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
    uint32_t m_pushConstantCount = 0;
    uint32_t m_pushConstantSize = 0;

public:

    void Build(VkDevice device, VkShaderModule computeShaderModule) {

        VkPushConstantRange pushConstantRange;
        pushConstantRange.offset = 0;
        pushConstantRange.size = m_pushConstantSize;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = m_descriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        pipelineLayoutInfo.pushConstantRangeCount = m_pushConstantCount;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
            std::cout << "Failed to create compute pipeline layout!";
        }

        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = computeShaderModule;
        computeShaderStageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = m_layout;
        pipelineInfo.stage = computeShaderStageInfo;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handle) != VK_SUCCESS) {
            std::cout << "Failed to create compute pipeline!";
        }
    }

    void PushDescriptorSetLayout(VkDescriptorSetLayout layout) {
        m_descriptorSetLayouts.push_back(layout);
    }

    VkPipeline GetHandle() {
        return m_handle;
    }

    VkPipelineLayout GetLayout() {
        return m_layout;
    }

    void SetPushConstantSize(uint32_t bufferSize) {
        m_pushConstantSize = bufferSize;
        m_pushConstantCount = 1;
    }

};