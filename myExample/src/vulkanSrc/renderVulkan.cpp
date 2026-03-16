#include "renderVulkan.h"
#include "renderObject.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <atomic>
#include <cstdint>
#include <execution>
#include <fstream>

bool RenderVulkan::init() {
	bool rtn = false;

	do {
		if (!prepare())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

bool RenderVulkan::valid() {
	return false;
}

void RenderVulkan::destroy() {
	return;
}

bool RenderVulkan::prepare() {
	// 同步对象
	if (!createSyncObj())
		return false;

	if (!setupFrameBuffer())
		return false;

	// CommandPool（懒加载）

	// 顶点数据，搬运到 GPU 中
	if (!createVertexBufferRes())
		return false;

	// UniformBuffer
	if (!createUniformBufferRes())
		return false;

	if (!createIndirectBufferRes())
		return false;

	if (!createNormalMapSample())
		return false;

	// Descriptor 资源
	// Layout
	if (!createDescriptorSetLayout())
		return false;
	// Pool
	if (!createDescriptorPool())
		return false;
	// Set
	if (!createDescriptorSets())
		return false;

	// Pipeline
	if (!createPipeline())
		return false;

	return true;
}

VkCommandPool RenderVulkan::getCmdPool(const uint32_t qIndex) {
	const auto & it_QIndex2CmdPool = map_QIndex2CmdPool.find(qIndex);
	if (it_QIndex2CmdPool != map_QIndex2CmdPool.end())
		return it_QIndex2CmdPool->second;

	auto & cmdPool = map_QIndex2CmdPool.emplace(qIndex, VkCommandPool()).first->second;

	VkCommandPoolCreateInfo cmdPoolCI{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = qIndex,
	};

	CHECK_VK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolCI, nullptr, &cmdPool));

	return cmdPool;
}

VkCommandBuffer RenderVulkan::getCmdBuffer(
	const uint32_t qIndex,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {

	VkCommandBuffer rtnCmd;

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = getCmdPool(qIndex),
		.level = cmdBufferLevel,
		.commandBufferCount = 1,
	};

	CHECK_VK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufferAllocateInfo, &rtnCmd));

	return rtnCmd;
}

std::vector<VkCommandBuffer> RenderVulkan::getCmdBuffers(
	const uint32_t qIndex, const uint32_t count,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {

	std::vector<VkCommandBuffer> vec_RtnCmd;
	vec_RtnCmd.resize(count);

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = getCmdPool(qIndex),
		.level = cmdBufferLevel,
		.commandBufferCount = count,
	};

	CHECK_VK_RESULT(
		vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufferAllocateInfo, vec_RtnCmd.data()));

	return vec_RtnCmd;
}

bool RenderVulkan::freeCommandBuffer(const uint32_t qIndex, const VkCommandBuffer & cmdBuffer) {

	if (!map_QIndex2CmdPool.contains(qIndex))
		return false;

	vkFreeCommandBuffers(m_LogicalDevice, getCmdPool(qIndex), 1, &cmdBuffer);

	return true;
}

bool RenderVulkan::freeCommandBuffer(const uint32_t qIndex,
									 const std::vector<VkCommandBuffer> & vec_CmdBuffer) {

	if (!map_QIndex2CmdPool.contains(qIndex))
		return false;

	vkFreeCommandBuffers(m_LogicalDevice, getCmdPool(qIndex), vec_CmdBuffer.size(),
						 vec_CmdBuffer.data());

	return true;
}

VkCommandBuffer RenderVulkan::getFrameCmdBuffer(
	const uint32_t currFrameIndex,
	const VkCommandBufferLevel & cmdBufferLevel /*  = VK_COMMAND_BUFFER_LEVEL_PRIMARY */) {

	if (currFrameIndex >= m_MaxConcurrentFrames)
		return {};

	if (vec_FrameCmdBuffer.empty())
		vec_FrameCmdBuffer =
			std::move(getCmdBuffers(m_QueueIndex.m_Graphics, m_MaxConcurrentFrames));

	return vec_FrameCmdBuffer[currFrameIndex];
}

bool RenderVulkan::createVertexBufferRes() {
	if (!getBuffer(vec_Vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_VertexBufferRes.m_Buffer,
				   m_VertexBufferRes.m_Memory))
		return false;

	if (!getBuffer(vec_Index, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_IndexBufferRes.m_Buffer,
				   m_IndexBufferRes.m_Memory))
		return false;

	m_IndexBufferRes.m_Count = static_cast<uint32_t>(vec_Index.size());

	return true;
}

template <typename T>
bool RenderVulkan::getBuffer(const std::vector<T> & vec_Value,
							 const VkBufferUsageFlags & bufferUsageFlags, VkBuffer & buffer,
							 VkDeviceMemory & bufferMemory) {
	/// Host-Visible
	if (vec_Value.empty())
		return false;

	uint32_t bufferSize = static_cast<uint32_t>(vec_Value.size()) * sizeof(T);

	BufferRes stagingBufferRes;
	if (!allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						stagingBufferRes.m_Buffer, stagingBufferRes.m_Memory))
		return false;

	void * data;
	CHECK_VK_RESULT(
		vkMapMemory(m_LogicalDevice, stagingBufferRes.m_Memory, 0, bufferSize, 0, &data));

	memcpy(data, vec_Value.data(), bufferSize);

	vkUnmapMemory(m_LogicalDevice, stagingBufferRes.m_Memory);

	/// Device-Local
	if (!allocateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsageFlags,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory))
		return false;

	/// Copy command
	VkCommandBuffer copyCmd = getCmdBuffer(m_QueueIndex.m_Transfer);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	if (vkBeginCommandBuffer(copyCmd, &cmdBufferBeginInfo))
		return false;

	VkBufferCopy copyRegion{.size = bufferSize};
	vkCmdCopyBuffer(copyCmd, stagingBufferRes.m_Buffer, buffer, 1, &copyRegion);

	CHECK_VK_RESULT(vkEndCommandBuffer(copyCmd));

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &copyCmd,
	};

	VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = 0};
	VkFence fence;
	CHECK_VK_RESULT(vkCreateFence(m_LogicalDevice, &fenceCI, nullptr, &fence));

	CHECK_VK_RESULT(
		vkQueueSubmit(map_QIndex2Queue[m_QueueIndex.m_Transfer], 1, &submitInfo, fence));

	CHECK_VK_RESULT(vkWaitForFences(m_LogicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	// Destroy
	vkDestroyFence(m_LogicalDevice, fence, nullptr);
	freeCommandBuffer(m_QueueIndex.m_Transfer, copyCmd);

	vkDestroyBuffer(m_LogicalDevice, stagingBufferRes.m_Buffer, nullptr);
	vkFreeMemory(m_LogicalDevice, stagingBufferRes.m_Memory, nullptr);

	return true;
}

bool RenderVulkan::allocateBuffer(const uint32_t bufferSize,
								  const VkBufferUsageFlags & bufferUsageFlags,
								  const VkMemoryPropertyFlags & memPropertyFlags, VkBuffer & buffer,
								  VkDeviceMemory & bufferMemory) {

	VkBufferCreateInfo bufferInfoCI{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bufferSize,
		.usage = bufferUsageFlags,
	};

	CHECK_VK_RESULT(vkCreateBuffer(m_LogicalDevice, &bufferInfoCI, nullptr, &buffer));

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memReqs);

	VkMemoryAllocateInfo memAlloc{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memReqs.size,
		.memoryTypeIndex =
			getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits, memPropertyFlags),
	};

	CHECK_VK_RESULT(vkAllocateMemory(m_LogicalDevice, &memAlloc, nullptr, &bufferMemory));

	CHECK_VK_RESULT(vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0));

	return true;
}

void RenderVulkan::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
										 VkImageLayout oldLayout, VkImageLayout newLayout,
										 uint32_t srcQueueFamilyIndex,
										 uint32_t dstQueueFamilyIndex) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	// 不同的同步节点
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

		// 如果发生了所有权转移（src != dst）
		if (srcQueueFamilyIndex != dstQueueFamilyIndex) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		} else {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
						 &barrier);
}

bool RenderVulkan::createNormalMapSample() {
	// 加载法线贴图数据
	int texWidth{0}, texHeight{0}, texChannels{0};
	BufferRes stagingBuffer;
	{
		stbi_uc * pixels = stbi_load((std::string(SADERPATH) + "triangle/NormalMap.png").c_str(),
									 &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!allocateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
								VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							stagingBuffer.m_Buffer, stagingBuffer.m_Memory))
			return false;

		// 拷贝
		void * data;
		vkMapMemory(m_LogicalDevice, stagingBuffer.m_Memory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_LogicalDevice, stagingBuffer.m_Memory);

		// 释放资源
		stbi_image_free(pixels);
	}

	VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM, // 法线贴图的标准格式
		.extent =
			{
				.width = static_cast<uint32_t>(texWidth),
				.height = static_cast<uint32_t>(texHeight),
				.depth = 1,
			},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL, // GPU 内部最优排布，CPU 不可读
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		// 独占模式
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	CHECK_VK_RESULT(
		vkCreateImage(m_LogicalDevice, &imageInfo, nullptr, &m_NormalMapImageRes.m_Image));

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(m_LogicalDevice, m_NormalMapImageRes.m_Image, &memReqs);

	VkMemoryAllocateInfo memAllloc{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memReqs.size,
		.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
											  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	if ((vkAllocateMemory(m_LogicalDevice, &memAllloc, nullptr, &m_NormalMapImageRes.m_Memory) !=
		 VK_SUCCESS) ||
		(vkBindImageMemory(m_LogicalDevice, m_NormalMapImageRes.m_Image,
						   m_NormalMapImageRes.m_Memory, 0) != VK_SUCCESS))
		return false;

	// cmd 操作
	{
		VkCommandBuffer cmd = getCmdBuffer(m_QueueIndex.m_Graphics);
		VkCommandBufferBeginInfo cmdBufferBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};
		CHECK_VK_RESULT(vkBeginCommandBuffer(cmd, &cmdBufferBeginInfo));

		transitionImageLayout(cmd, m_NormalMapImageRes.m_Image, VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy region{
			.bufferOffset = 0,
			.imageSubresource =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			.imageOffset =
				{
					0,
					0,
					0,
				},
			.imageExtent =
				{
					(uint32_t)texWidth,
					(uint32_t)texHeight,
					1,
				},
		};

		vkCmdCopyBufferToImage(cmd, stagingBuffer.m_Buffer, m_NormalMapImageRes.m_Image,
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		transitionImageLayout(cmd, m_NormalMapImageRes.m_Image,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CHECK_VK_RESULT(vkEndCommandBuffer(cmd));

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd,
		};

		vkQueueSubmit(map_QIndex2Queue[m_QueueIndex.m_Graphics], 1, &submitInfo, VK_NULL_HANDLE);

		vkQueueWaitIdle(map_QIndex2Queue[m_QueueIndex.m_Graphics]);

		// 资源释放
		freeCommandBuffer(m_QueueIndex.m_Graphics, cmd);
	}

	// 资源释放
	vkDestroyBuffer(m_LogicalDevice, stagingBuffer.m_Buffer, nullptr);
	vkFreeMemory(m_LogicalDevice, stagingBuffer.m_Memory, nullptr);

	VkImageViewCreateInfo viewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = m_NormalMapImageRes.m_Image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
	};

	vkCreateImageView(m_LogicalDevice, &viewInfo, nullptr, &m_NormalMapImageRes.m_ImageView);

	VkSamplerCreateInfo samplerInfo{
		.magFilter = VK_FILTER_LINEAR, // 线性过滤
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT, // 法线贴图通常需要重复平铺
	};

	vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, &m_NormalMapSample);

	return m_NormalMapSample != VK_NULL_HANDLE;
}

bool RenderVulkan::createUniformBufferRes() {
	VkMemoryAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = 0,
		.memoryTypeIndex = 0,
	};

	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = sizeof(ShaderData),
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	};

	vec_UniformRes.resize(m_MaxConcurrentFrames);

	VkMemoryRequirements memReqs;

	for (uint32_t i = 0; i < m_MaxConcurrentFrames; i++) {
		auto & uniformRes = vec_UniformRes[i];

		CHECK_VK_RESULT(vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr,
									   &uniformRes.m_BufferRes.m_Buffer));

		vkGetBufferMemoryRequirements(m_LogicalDevice, uniformRes.m_BufferRes.m_Buffer, &memReqs);

		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
														   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		CHECK_VK_RESULT(vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr,
										 &uniformRes.m_BufferRes.m_Memory));

		CHECK_VK_RESULT(vkBindBufferMemory(m_LogicalDevice, uniformRes.m_BufferRes.m_Buffer,
										   uniformRes.m_BufferRes.m_Memory, 0));

		CHECK_VK_RESULT(vkMapMemory(m_LogicalDevice, uniformRes.m_BufferRes.m_Memory, 0,
									memReqs.size, 0, (void **)&uniformRes.m_PMapped));
	}

	return !vec_UniformRes.empty();
}

bool RenderVulkan::createIndirectBufferRes() {
	const VkDeviceSize & bufferSize = sizeof(VkDrawIndexedIndirectCommand) *
									  RenderObjectManager::getInstance().getRenderObjCount();

	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkMemoryAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = 0,
		.memoryTypeIndex = 0,
	};

	VkMemoryRequirements memReqs;

	vec_IndirectRes.resize(m_MaxConcurrentFrames);

	for (uint32_t i = 0; i < m_MaxConcurrentFrames; i++) {
		auto & indirctRes = vec_IndirectRes[i];

		CHECK_VK_RESULT(vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr,
									   &indirctRes.m_BufferRes.m_Buffer));

		vkGetBufferMemoryRequirements(m_LogicalDevice, indirctRes.m_BufferRes.m_Buffer, &memReqs);

		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = getMemoryTypeIndex(m_MemoryProperty, memReqs.memoryTypeBits,
													   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
														   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		CHECK_VK_RESULT(vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr,
										 &indirctRes.m_BufferRes.m_Memory));

		CHECK_VK_RESULT(vkBindBufferMemory(m_LogicalDevice, indirctRes.m_BufferRes.m_Buffer,
										   indirctRes.m_BufferRes.m_Memory, 0));

		CHECK_VK_RESULT(vkMapMemory(m_LogicalDevice, indirctRes.m_BufferRes.m_Memory, 0,
									memReqs.size, 0, (void **)&indirctRes.m_PMapped));

		// test, 初始化时写死
		/* std::vector<VkDrawIndexedIndirectCommand> vec_IndirectInfo;
		uint32_t drawCount{0};
		{
			const auto & vec_pRenderObj = RenderObjectManager::getInstance().getVecRenderObj(
				[this](const GPoint & pt) -> bool { return m_pCamera->isPtIn(pt); });

			vec_IndirectInfo.reserve(vec_pRenderObj.size());
			drawCount = vec_pRenderObj.size();

			for (const auto & pRenderObj : vec_pRenderObj) {
				vec_IndirectInfo.emplace_back(VkDrawIndexedIndirectCommand{
					.indexCount = pRenderObj->getIndexCount(),
					.instanceCount = 1,
					.firstIndex = pRenderObj->getIndexOffset(),
					.vertexOffset = static_cast<int32_t>(pRenderObj->getVertexOffset()),
				});
			}
		}

		memcpy(indirctRes.m_PMapped, vec_IndirectInfo.data(),
			   sizeof(VkDrawIndexedIndirectCommand) * drawCount); */
	}

	return !vec_IndirectRes.empty();
}

bool RenderVulkan::createDescriptorSetLayout() {
	std::vector<VkDescriptorSetLayoutBinding> vec_DescriptorSetLayout{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr,
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		},
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.bindingCount = static_cast<uint32_t>(vec_DescriptorSetLayout.size()),
		.pBindings = vec_DescriptorSetLayout.data(),
	};

	CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_LogicalDevice, &descriptorLayoutCI, nullptr,
												&m_DescriptorSetLayout));

	return true;
}

bool RenderVulkan::createDescriptorPool() {
	VkDescriptorPoolSize descriptorTypeCounts[2];
	descriptorTypeCounts[0] = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = m_MaxConcurrentFrames,
	};
	descriptorTypeCounts[1] = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = m_MaxConcurrentFrames,
	};

	VkDescriptorPoolCreateInfo descriptorPoolCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.maxSets = m_MaxConcurrentFrames,
		.poolSizeCount = 2,
		.pPoolSizes = descriptorTypeCounts,
	};

	CHECK_VK_RESULT(
		vkCreateDescriptorPool(m_LogicalDevice, &descriptorPoolCI, nullptr, &m_DescriptorPool));

	return true;
}

bool RenderVulkan::createDescriptorSets() {
	for (uint32_t i = 0; i < m_MaxConcurrentFrames; i++) {
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &m_DescriptorSetLayout,
		};

		CHECK_VK_RESULT(vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo,
												 &vec_UniformRes[i].m_DescriptorSet));

		VkWriteDescriptorSet descriptorWrites[2];

		// UBO
		VkDescriptorBufferInfo bufferInfo{
			.buffer = vec_UniformRes[i].m_BufferRes.m_Buffer,
			.offset = 0,
			.range = sizeof(ShaderData),
		};
		descriptorWrites[0] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vec_UniformRes[i].m_DescriptorSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo,
		};

		// NormalMap
		VkDescriptorImageInfo imageInfo{
			.sampler = m_NormalMapSample,
			.imageView = m_NormalMapImageRes.m_ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		descriptorWrites[1] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vec_UniformRes[i].m_DescriptorSet,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo,
		};

		vkUpdateDescriptorSets(m_LogicalDevice, 2, descriptorWrites, 0, nullptr);
	}

	return true;
}

bool RenderVulkan::createPipeline() {

	VkPipelineLayoutCreateInfo pipelineLayoutCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 1,
		.pSetLayouts = &m_DescriptorSetLayout,
	};

	CHECK_VK_RESULT(
		vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutCI, nullptr, &m_PipelineLayout));

	VkPipelineCacheCreateInfo pipelineCacheCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
	};

	CHECK_VK_RESULT(
		vkCreatePipelineCache(m_LogicalDevice, &pipelineCacheCI, nullptr, &m_PipelineCache));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		//.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	};

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		//.cullMode = VK_CULL_MODE_NONE,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f,
	};

	VkPipelineColorBlendAttachmentState blendAttachmentState{
		.blendEnable = VK_FALSE,
		.colorWriteMask = 0xf,
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &blendAttachmentState,
	};

	VkPipelineViewportStateCreateInfo viewportStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	std::vector<VkDynamicState> dynamicStateEnables{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size()),
		.pDynamicStates = dynamicStateEnables.data(),
	};

	VkStencilOpState stencilState{
		.failOp = VK_STENCIL_OP_KEEP,
		.passOp = VK_STENCIL_OP_KEEP,
		.compareOp = VK_COMPARE_OP_ALWAYS,
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = stencilState,
		.back = stencilState,
	};

	VkPipelineMultisampleStateCreateInfo multisampleStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.pSampleMask = nullptr,
	};

	// 绑定顶点数据的步长
	VkVertexInputBindingDescription vertexInputBinding{
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	// 对应 GLSL 中的 location
	// 描述内存布局
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributs{
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, pos),
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, normal),
		},
		{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, color),
		},
		{
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, posApex),
		},
		{
			.location = 4,
			.binding = 0,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = offsetof(Vertex, heightRatio),
		},
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBinding,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributs.size()),
		.pVertexAttributeDescriptions = vertexInputAttributs.data(),
	};

	// Shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{

		// Vertex shader
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = loadSPIRVShader(std::string(SADERPATH) + "triangle/triangle.vert.spv"),
			.pName = "main",
		},

		// Fragment shader
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = loadSPIRVShader(std::string(SADERPATH) + "triangle/triangle.frag.spv"),
			.pName = "main",
		},
	};

	assert(shaderStages[0].module != VK_NULL_HANDLE);
	assert(shaderStages[1].module != VK_NULL_HANDLE);

	VkGraphicsPipelineCreateInfo pipelineCI{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputStateCI,
		.pInputAssemblyState = &inputAssemblyStateCI,
		.pViewportState = &viewportStateCI,
		.pRasterizationState = &rasterizationStateCI,
		.pMultisampleState = &multisampleStateCI,
		.pDepthStencilState = &depthStencilStateCI,
		.pColorBlendState = &colorBlendStateCI,
		.pDynamicState = &dynamicStateCI,
		.layout = m_PipelineLayout,
		.renderPass = m_renderPass,
	};

	CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_LogicalDevice, m_PipelineCache, 1, &pipelineCI,
											  nullptr, &m_Pipeline));

	vkDestroyShaderModule(m_LogicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_LogicalDevice, shaderStages[1].module, nullptr);

	return true;
}

VkShaderModule RenderVulkan::loadSPIRVShader(const std::string & filePath) {
	if (filePath.empty())
		return VK_NULL_HANDLE;

	std::ifstream is(filePath, std::ios::binary | std::ios::in | std::ios::ate);

	if (!is.is_open()) {
		std::cout << "Can not Open file: " << filePath << std::endl;
		return VK_NULL_HANDLE;
	}

	size_t shaderSize = is.tellg();
	assert(shaderSize > 0);
	is.seekg(0, std::ios::beg);
	char * shaderCode = new char[shaderSize];
	is.read(shaderCode, shaderSize);
	is.close();

	VkShaderModuleCreateInfo shaderModuleCI{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shaderSize,
		.pCode = (uint32_t *)shaderCode,
	};

	VkShaderModule shaderModule;
	CHECK_VK_RESULT(vkCreateShaderModule(m_LogicalDevice, &shaderModuleCI, nullptr, &shaderModule));

	delete[] shaderCode;

	return shaderModule;
}

bool RenderVulkan::createSyncObj() {
	VkFenceCreateInfo fenceCI{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	VkSemaphoreCreateInfo semaphoreCI{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	vec_Fence.resize(m_MaxConcurrentFrames);
	vec_PresentSmph.resize(m_MaxConcurrentFrames);
	for (uint32_t i = 0; i < m_MaxConcurrentFrames; ++i) {
		CHECK_VK_RESULT(vkCreateFence(m_LogicalDevice, &fenceCI, nullptr, &vec_Fence[i]));
		CHECK_VK_RESULT(
			vkCreateSemaphore(m_LogicalDevice, &semaphoreCI, nullptr, &vec_PresentSmph[i]));
	}

	vec_RenderSmph.resize(vec_ImageView.size());
	for (size_t i = 0; i < vec_ImageView.size(); ++i)
		CHECK_VK_RESULT(
			vkCreateSemaphore(m_LogicalDevice, &semaphoreCI, nullptr, &vec_RenderSmph[i]));

	vec_ImagesInFlight.resize(vec_ImageView.size(), VK_NULL_HANDLE);

	return !vec_Fence.empty() && !vec_PresentSmph.empty() && !vec_RenderSmph.empty();
}

bool RenderVulkan::setupFrameBuffer() {
	vec_FrameBuffer.resize(vec_ImageView.size());
	for (size_t i = 0; i < vec_FrameBuffer.size(); i++) {
		std::vector<VkImageView> attachments{
			vec_ImageView[i],
			m_DepthStencilRes.m_ImageView,
		};

		VkFramebufferCreateInfo frameBufferCI{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_renderPass,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.width = static_cast<uint32_t>(m_WindowWidth),
			.height = static_cast<uint32_t>(m_WindowHeight),
			.layers = 1,
		};

		CHECK_VK_RESULT(
			vkCreateFramebuffer(m_LogicalDevice, &frameBufferCI, nullptr, &vec_FrameBuffer[i]));
	}

	return !vec_FrameBuffer.empty();
}

bool RenderVulkan::renderNext() {
	vkWaitForFences(m_LogicalDevice, 1, &vec_Fence[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
	CHECK_VK_RESULT(vkResetFences(m_LogicalDevice, 1, &vec_Fence[m_CurrentFrameIndex]));

	// 此处并不能代表 imageIndex 所在的 VkImage 可以直接使用了，submit 命令中还需要等待
	// vec_PresentSmph[m_CurrentFrameIndex] 这个信号量
	uint32_t imageIndex{0};
	VkResult result =
		vkAcquireNextImageKHR(m_LogicalDevice, m_Swapchain, UINT64_MAX,
							  vec_PresentSmph[m_CurrentFrameIndex], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		// window resize
		return false;
	} else if (result != VK_SUCCESS) {
		std::cout << "Could not acquire the next swap chain image!" << std::endl;
		fflush(stdout);
		exit(2);
	}

	ShaderData shaderData{
		.projectionMatrix = m_pCamera->getPerspectiveMtx(),
		.modelMatrix = glm::mat4(1.0f),
		.viewMatrix = m_pCamera->getViewMtx(),
	};

	memcpy(vec_UniformRes[m_CurrentFrameIndex].m_PMapped, &shaderData, sizeof(ShaderData));

	// 批量绘制
	// 直接对 map 的内存进行写入
	VkDrawIndexedIndirectCommand * pDest =
		static_cast<VkDrawIndexedIndirectCommand *>(vec_IndirectRes[m_CurrentFrameIndex].m_PMapped);
	std::atomic<uint32_t> drawCount{0};
	{
		const auto & vec_pRenderObjAll = RenderObjectManager::getInstance().getVecRenderObjectAll();

		std::for_each(std::execution::par_unseq, vec_pRenderObjAll.begin(), vec_pRenderObjAll.end(),
					  [&](const RenderObject * const pRenderObj) {
						  if (m_pCamera->isPtIn(pRenderObj->getFeaturePt())) {
							  const auto & writeIndex =
								  drawCount.fetch_add(1, std::memory_order_relaxed);

							  pDest[writeIndex].indexCount = pRenderObj->getIndexCount();
							  pDest[writeIndex].instanceCount = 1;
							  pDest[writeIndex].firstIndex = pRenderObj->getIndexOffset();
							  pDest[writeIndex].vertexOffset =
								  static_cast<int32_t>(pRenderObj->getVertexOffset());
							  pDest[writeIndex].firstInstance = 0;
						  }
					  });
	}

	//
	const auto & currentCmdBuffer = getFrameCmdBuffer(m_CurrentFrameIndex);

	vkResetCommandBuffer(currentCmdBuffer, 0);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	CHECK_VK_RESULT(vkBeginCommandBuffer(currentCmdBuffer, &cmdBufferBeginInfo));

	VkClearValue clearValues[2]{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_renderPass,
		.framebuffer = vec_FrameBuffer[imageIndex],
		.renderArea =
			{
				.offset =
					{
						.x = 0,
						.y = 0,
					},
				.extent =
					{
						.width = static_cast<uint32_t>(m_WindowWidth),
						.height = static_cast<uint32_t>(m_WindowHeight),
					},
			},
		.clearValueCount = 2,
		.pClearValues = clearValues,
	};

	vkCmdBeginRenderPass(currentCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.height = static_cast<float>(m_WindowHeight);
	viewport.width = static_cast<float>(m_WindowWidth);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(currentCmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = m_WindowWidth;
	scissor.extent.height = m_WindowHeight;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(currentCmdBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0,
							1, &vec_UniformRes[m_CurrentFrameIndex].m_DescriptorSet, 0, nullptr);

	vkCmdBindPipeline(currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	// 此处可以使用 Storage Buffer， 同时通过 offsets 访问不同块
	VkDeviceSize offsets[1]{0};
	vkCmdBindVertexBuffers(currentCmdBuffer, 0, 1, &m_VertexBufferRes.m_Buffer, offsets);

	vkCmdBindIndexBuffer(currentCmdBuffer, m_IndexBufferRes.m_Buffer, 0, VK_INDEX_TYPE_UINT32);

	// vkCmdDrawIndexed(currentCmdBuffer, m_IndexBufferRes.m_Count, 1, 0, 0, 0);

	vkCmdDrawIndexedIndirect(currentCmdBuffer,
							 vec_IndirectRes[m_CurrentFrameIndex].m_BufferRes.m_Buffer, 0,
							 drawCount.load(), sizeof(VkDrawIndexedIndirectCommand));

	// 一次性绘制测试
	/* vkCmdDrawIndexedIndirect(currentCmdBuffer,
							 vec_IndirectRes[m_CurrentFrameIndex].m_BufferRes.m_Buffer, 0,
							 80000, sizeof(VkDrawIndexedIndirectCommand)); */

	vkCmdEndRenderPass(currentCmdBuffer);

	CHECK_VK_RESULT(vkEndCommandBuffer(currentCmdBuffer));

	// 表示 颜色输出阶段
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		// 需要等待此信号量，才可以继续执行，此信号量表示这一个 VkImage 是否可用
		.pWaitSemaphores = &vec_PresentSmph[m_CurrentFrameIndex],
		// 表示要在 COLOR_ATTACHMENT_OUTPUT 这个阶段执行之前，等待上述信号量，
		// 也就是等待这个VkImage是否可用
		.pWaitDstStageMask = &waitStageMask, // 可以设置多个阶段的等待
		.commandBufferCount = 1,
		.pCommandBuffers = &currentCmdBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vec_RenderSmph[imageIndex],
	};

	CHECK_VK_RESULT(vkQueueSubmit(map_QIndex2Queue[m_QueueIndex.m_Graphics], 1, &submitInfo,
								  vec_Fence[m_CurrentFrameIndex]));

	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vec_RenderSmph[imageIndex],
		.swapchainCount = 1,
		.pSwapchains = &m_Swapchain,
		.pImageIndices = &imageIndex,
	};

	result = vkQueuePresentKHR(map_QIndex2Queue[m_QueueIndex.m_Graphics], &presentInfo);

	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		// windows resize
	} else if (result != VK_SUCCESS) {
		std::cout << "Could not present the image to the swap chain!" << std::endl;
		fflush(stdout);
		exit(2);
	}

	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxConcurrentFrames;

	return true;
}

bool RenderVulkan::isReady() {
	if (vec_Vertex.empty() || vec_Index.empty() || (m_renderPass == VK_NULL_HANDLE) ||
		(m_LogicalDevice == VK_NULL_HANDLE) || map_QIndex2Queue.empty() || vec_ImageView.empty() ||
		(m_Swapchain == VK_NULL_HANDLE) || (m_pCamera != nullptr))
		return false;

	return true;
}