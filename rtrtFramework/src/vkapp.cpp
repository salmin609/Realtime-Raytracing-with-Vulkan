#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream


#ifdef WIN64
#else
#include <unistd.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vkapp.h"

#include "app.h"


template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
	return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>

VkApp::VkApp(App* _app) : app(_app)
{
	m_pcRay.rr = 0.8f;
	m_pcRay.alignmentTest = 1234;
	m_pcDenoise.normFactor = 0.003f;
	m_pcDenoise.depthFactor = 0.007f;
	m_pcDenoise.lumenFactor = 0.0f;

	createInstance(app->doApiDump);
	assert(m_instance);
	createPhysicalDevice(); // i.e. the GPU
	chooseQueueIndex();
	createDevice();
	getCommandQueue();

	loadExtensions();

	getSurface();
	createCommandPool();

	createSwapchain();
	createDepthResource();
	createPostRenderPass();
	createPostFrameBuffers();

	createScBuffer();
	createPostDescriptor();
	createPostPipeline();
	myloadModel("models/living_room.obj", glm::mat4(1.f));
	createMatrixBuffer();
	createObjDescriptionBuffer();
	createScanlineRenderPass();
	createScDescriptorSet();
	createScPipeline();

	createRtBuffers();
	initRayTracing();
	createRtAccelerationStructure();
	createRtDescriptorSet();
	createRtPipeline();
	createRtShaderBindingTable();

	createDenoiseBuffer();
	createDenoiseDescriptorSet();
	createDenoiseCompPipeline();

	#ifdef GUI
	initGUI();
	#endif
}


void VkApp::drawFrame()
{
	prepareFrame();

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	{ // Extra indent for recording commands into m_commandBuffer
		updateCameraBuffer();

		// Draw scene
		if (useRaytracer) {
			raytrace();

			if(doDenoise)
				denoise();
		}
		else {
			rasterize();
		}

		postProcess(); // tone mapper and output to swapchain image.

		vkEndCommandBuffer(m_commandBuffer);
	} // Done recording; Execute!

	submitFrame(); // Submit for display
}

VkCommandBuffer VkApp::createTempCmdBuffer()
{
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = m_cmdPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkCommandBuffer cmdBuffer;
	vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdBuffer);

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer, &beginInfo);
	return cmdBuffer;
}

void VkApp::submitTempCmdBuffer(VkCommandBuffer cmdBuffer)
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	vkQueueSubmit(m_queue, 1, &submitInfo, {});
	vkQueueWaitIdle(m_queue);
	vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdBuffer);
}

void VkApp::prepareFrame()
{
	// Acquire the next image from the swap chain --> m_swapchainIndex
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_readSemaphore,
		(VkFence)VK_NULL_HANDLE, &m_swapchainIndex);

	// Check if window has been resized -- or other(??) swapchain specific event
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSizedResources(windowSize);
	}

	// Use a fence to wait until the command buffer has finished execution before using it again
	while (VK_TIMEOUT == vkWaitForFences(m_device, 1, &m_waitFence, VK_TRUE, 1'000'000))
	{
	}
}

void VkApp::submitFrame()
{
	vkResetFences(m_device, 1, &m_waitFence);

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// The submit info structure specifies a command buffer queue submission batch
	VkSubmitInfo _si_{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	_si_.pNext = nullptr;
	_si_.pWaitDstStageMask = &waitStageMask; //  pipeline stages to wait for
	_si_.waitSemaphoreCount = 1;
	_si_.pWaitSemaphores = &m_readSemaphore;  // waited upon before execution
	_si_.signalSemaphoreCount = 1;
	_si_.pSignalSemaphores = &m_writtenSemaphore; // signaled when execution finishes
	_si_.commandBufferCount = 1;
	_si_.pCommandBuffers = &m_commandBuffer;
	if (vkQueueSubmit(m_queue, 1, &_si_, m_waitFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// Present frame
	VkPresentInfoKHR _i_{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	_i_.waitSemaphoreCount = 1;
	_i_.pWaitSemaphores = &m_writtenSemaphore;;
	_i_.swapchainCount = 1;
	_i_.pSwapchains = &m_swapchain;
	_i_.pImageIndices = &m_swapchainIndex;
	if (vkQueuePresentKHR(m_queue, &_i_) != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
}


VkShaderModule VkApp::createShaderModule(std::string code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*)code.data();

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		assert(0 && "failed to create shader module!");

	return shaderModule;
}

VkPipelineShaderStageCreateInfo VkApp::createShaderStageInfo(const std::string& code,
	VkShaderStageFlagBits stage,
	const char* entryPoint)
{
	VkPipelineShaderStageCreateInfo shaderStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStage.stage = stage;
	shaderStage.module = createShaderModule(code);
	shaderStage.pName = entryPoint;
	return shaderStage;
}
