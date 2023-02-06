
#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

#include <unordered_set>
#include <unordered_map>

#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"
#include "backends/imgui_impl_vulkan.h"


void VkApp::destroyAllVulkanResources()
{
    // @@
    vkDeviceWaitIdle(m_device);  // Uncomment this when you have an m_device created.

    // Destroy all vulkan objects.
    // ...  All objects created on m_device must be destroyed before m_device.
    //vkDestroyDevice(m_device, nullptr);
    //vkDestroyInstance(m_instance, nullptr);

    //vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    destroySwapchain();
    m_depthImage.destroy(m_device);
    vkDestroyRenderPass(m_device, m_postRenderPass, nullptr);

    const int size = m_framebuffers.size();

    for(int i = 0; i < size; ++i)
		vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);

    vkDestroyPipelineLayout(m_device, m_postPipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_postPipeline, nullptr);

    m_scImageBuffer.destroy(m_device);
    m_postDesc.destroy(m_device);

    
    int textureSize = m_objText.size();
    for(int i = 0; i < textureSize; ++i)
        m_objText[i].destroy(m_device);



    //for (int i = 0; i < buffersSize; ++i)
    //    m_objDesc[i].destroy(m_device);
    m_matrixBW.destroy(m_device);
    m_objDescriptionBW.destroy(m_device);
    vkDestroyRenderPass(m_device, m_scanlineRenderPass, nullptr);
    vkDestroyFramebuffer(m_device, m_scanlineFramebuffer, nullptr);
    m_scDesc.destroy(m_device);
    m_shaderBindingTableBW.destroy(m_device);

    m_rtBuilder.destroy();

    vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
    m_shaderBindingTableBW.destroy(m_device);

    vkDestroyDescriptorPool(m_device, m_imguiDescPool, nullptr);
    ImGui_ImplVulkan_Shutdown();

    m_lightBuff.destroy(m_device);

    vkDestroyCommandPool(m_device, m_cmdPool, nullptr);

    vkDestroyPipelineLayout(m_device, m_scanlinePipelineLayout, nullptr);
    vkDestroyPipeline(m_device, m_scanlinePipeline, nullptr);

    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VkApp::recreateSizedResources(VkExtent2D size)
{
    assert(false && "Not ready for onResize events.");
    // Destroy everything related to the window size
    // (RE)Create them all at the new size
}
 
void VkApp::createInstance(bool doApiDump)
{
    uint32_t countGLFWextensions{0};
    const char** reqGLFWextensions = glfwGetRequiredInstanceExtensions(&countGLFWextensions);

    // @@
    // Append each GLFW required extension in reqGLFWextensions to reqInstanceExtensions
    // Print them out while your are at it
    printf("GLFW required extensions:\n");

    reqInstanceExtensions.push_back(*reqGLFWextensions);
    reqInstanceExtensions.push_back(*(++reqGLFWextensions));

    // Suggestion: Parse a command line argument to set/unset doApiDump
    if (doApiDump)
        reqInstanceLayers.push_back("VK_LAYER_LUNARG_api_dump");
  
    uint32_t count;
    // The two step procedure for getting a variable length list
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> availableLayers(count);
    vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

    // @@
    // Print out the availableLayers
    // ...  use availableLayers[i].layerName

    printf("InstanceLayer count: %d\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("Layer #%d : %s\n", i, availableLayers[i].layerName);
    }
    printf("\n");

    // Another two step dance
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions.data());

    // @@
    // Print out the availableExtensions
    printf("InstanceExtensions count: %d\n", count);
    // ...  use availableExtensions[i].extensionName
    for (int i = 0; i < count; ++i)
    {
        printf("Extension #%d : %s\n", i, availableExtensions[i].extensionName);
    }
    printf("\n");

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "rtrt";
    applicationInfo.pEngineName      = "no-engine";
    applicationInfo.apiVersion       = VK_MAKE_VERSION(1, 3, 0);

    VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pNext                   = nullptr;
    instanceCreateInfo.pApplicationInfo        = &applicationInfo;
    
    instanceCreateInfo.enabledExtensionCount   = reqInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = reqInstanceExtensions.data();
    
    instanceCreateInfo.enabledLayerCount       = reqInstanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames     = reqInstanceLayers.data();

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("Create Instance Failed!");

    // @@
    // Verify VkResult is VK_SUCCESS
    // Document with a cut-and-paste of the three list printouts above.
    // To destroy: vkDestroyInstance(m_instance, nullptr);
}

void VkApp::createPhysicalDevice()
{
    uint physicalDevicesCount;
    vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, physicalDevices.data());

    std::vector<uint32_t> compatibleDevices;
  
    printf("%d devices\n", physicalDevicesCount);
    int i = 0;

    // For each GPU:
    for (auto physicalDevice : physicalDevices) {

        // Get the GPU's properties
        VkPhysicalDeviceProperties GPUproperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &GPUproperties);

        // Get the GPU's extension list;  Another two-step list retrieval procedure:
        uint extCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> extensionProperties(extCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                             &extCount, extensionProperties.data());

        // @@ This code is in a loop iterating variable physicalDevice
        // through a list of all physicalDevices.  The
        // physicalDevice's properties (GPUproperties) and a list of
        // its extension properties (extensionProperties) are retrieve
        // above, and here we judge if the physicalDevice (i.e.. GPU)
        // is compatible with our requirements. We consider a GPU to be
        // compatible if it satisfies both:
        //    GPUproperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        // and
        //    All reqDeviceExtensions can be found in the GPUs extensionProperties list
        //      That is: for all i, there exists a j such that:
        //                 reqDeviceExtensions[i] == extensionProperties[j].extensionName

        int extensionSize = extensionProperties.size();
        int requireSize = reqDeviceExtensions.size();
        int count = 0;
        for(i = 0; i < requireSize; ++i)
        {
	        for (int j = 0; j < extensionSize; ++j)
	        {

	            if (GPUproperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    strcmp(extensionProperties[j].extensionName, reqDeviceExtensions[i]) == 0)
	            {
		            //Means GPU is found to be compatible
	                //Return physical Device
                    count++;
	            }
	        }
        }
        m_physicalDevice = physicalDevices[0];
        //  If a GPU is found to be compatible
        //  Return it (physicalDevice), or continue the search and then return the best GPU.
        //    raise an exception of none were found
        //    tell me all about your system if more than one was found.
    }
  
}

void VkApp::chooseQueueIndex()
{
    VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
                                      | VK_QUEUE_TRANSFER_BIT;
    
    uint32_t mpCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(mpCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, queueProperties.data());

    /*
     * pQueueFamilyProperties[0]:      VkQueueFamilyProperties = 000002157189A960:
            queueFlags:                     VkQueueFlags = 15 (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT)
            queueCount:                     uint32_t = 16
            timestampValidBits:             uint32_t = 64
            minImageTransferGranularity:    VkExtent3D = 000002157189A96C:
                width:                          uint32_t = 1
                height:                         uint32_t = 1
                depth:                          uint32_t = 1
     */
    // @@ Use the api_dump to document the results of the above two
    // step.  How many queue families does your Vulkan offer.  Which
    // of them, by index, has the above three required flags?

    //How many queue families Vulkan offer : 3
    //first queue families has three required flags 
    

    //@@ Search the list for (the index of) the first queue family that has the required flags.
    // Verity that your search choose the correct queue family.
    // Record the index in m_graphicsQueueIndex.
    // Nothing to destroy as m_graphicsQueueIndex is just an integer.
    //m_graphicsQueueIndex = you chosen index;
    m_graphicsQueueIndex = 0;
}


void VkApp::createDevice()
{
    // @@
    // Build a pNext chain of the following six "feature" structures:
    //   features2->features11->features12->features13->accelFeature->rtPipelineFeature->NULL

    // Hint: Keep it simple; add a second parameter (the pNext pointer) to each
    // structure point up to the previous structure.
    
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    
    VkPhysicalDeviceVulkan13Features features13{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    
    VkPhysicalDeviceVulkan12Features features12{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    
    VkPhysicalDeviceVulkan11Features features11{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    
    VkPhysicalDeviceFeatures2 features2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    features2.pNext = &features11;
    features11.pNext = &features12;
    features12.pNext = &features13;
    features13.pNext = &accelFeature;
    accelFeature.pNext = &rtPipelineFeature;
    rtPipelineFeature.pNext = nullptr;

    // Fill in all structures on the pNext chain
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);
    //abort();
    // @@
    // Document the whole filled in pNext chain using an api_dump
    // Examine all the many features.  Do any of them look familiar?

    // Turn off robustBufferAccess (WHY?)
    features2.features.robustBufferAccess = VK_FALSE;

    float priority = 1.0;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = m_graphicsQueueIndex;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &priority;
    
    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.pNext            = &features2; // This is the whole pNext chain
  
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos    = &queueInfo;
    
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(reqDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = reqDeviceExtensions.data();

    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("Create Physical Device Failed");
    
    // @@
    // Verify VK_SUCCESS
    // To destroy: vkDestroyDevice(m_device, nullptr);
    //vkDestroyDevice(m_device, nullptr);
}

// That's all for now!
// Many more procedures will follow ...
