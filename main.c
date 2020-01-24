
#include <glib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

int main(int argc, char* argv[])
{
    int error = SDL_Init(SDL_INIT_EVERYTHING);
    if(error < 0)
    {
        fprintf(stderr, "Failed to initialize SDL\n");
        exit(EXIT_FAILURE);
    }

    SDL_Window* window = SDL_CreateWindow("Triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    if(!window)
    {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    VkApplicationInfo app_info;
    memset(&app_info, 0, sizeof(VkApplicationInfo));
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info;
    memset(&instance_info, 0, sizeof(VkInstanceCreateInfo));
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    uint32_t extension_cnt = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extension_cnt, NULL);

    const gchar** extensions = malloc(sizeof(const gchar*) * extension_cnt);
    SDL_Vulkan_GetInstanceExtensions(window, &extension_cnt, extensions);

    instance_info.enabledExtensionCount = extension_cnt;
    instance_info.ppEnabledExtensionNames = extensions;//(const gchar**)extensions->data;

    GPtrArray* layers = g_ptr_array_new();
    g_ptr_array_add(layers, "VK_LAYER_KHRONOS_validation");
    instance_info.enabledLayerCount = layers->len;
    instance_info.ppEnabledLayerNames = (const gchar**)layers->pdata;

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create VkInstance\n");
    }
    g_ptr_array_free(layers, TRUE);

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    uint32_t device_cnt = 0;
    vkEnumeratePhysicalDevices(instance, &device_cnt, NULL);
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_cnt);
    vkEnumeratePhysicalDevices(instance, &device_cnt, devices);

    int i = 0;
    for(i; i < device_cnt; i++)
    {
        VkPhysicalDevice d = devices[i];
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);

        fprintf(stdout, "GPU: %s\n", props.deviceName);
        if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physical_device = d;
            break;
        }
        
    }

    /**
     * Create surface
     */
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(window, instance, &surface);


    /**
     * Get physical device queue families
     */
    uint32_t queue_family_cnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_cnt, NULL);

    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_cnt);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_cnt, queue_families);
    
    i = 0;
    uint32_t graphics_family_index = 0;
    for(i; i < queue_family_cnt; i++)
    {
        VkQueueFamilyProperties props = queue_families[i];
        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family_index = i;
        }

        char queue_name[256];
        if(props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            strcpy(queue_name, "Graphics Queue");
        } else if(props.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            strcpy(queue_name, "Compute Queue");
        } else if(props.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            strcpy(queue_name, "Transfer Queue");
        } else {
            strcpy(queue_name, "Unknown Type");
        }

        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if(present_support == VK_TRUE)
        {
            fprintf(stdout, "Present supported on [%s] queue: %d\n", queue_name, i);
        }
    }


    VkDeviceCreateInfo device_create_info;
    memset(&device_create_info, 0, sizeof(VkDeviceCreateInfo));

    VkDeviceQueueCreateInfo queue_create_info;
    memset(&queue_create_info, 0, sizeof(VkDeviceQueueCreateInfo));
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = graphics_family_index;
    queue_create_info.queueCount = 1;
    float queue_priority = 1.0f;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features;
    memset(&device_features, 0, sizeof(VkPhysicalDeviceFeatures));


    GPtrArray* device_exts = g_ptr_array_new();
    g_ptr_array_add(device_exts, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = device_exts->len;
    device_create_info.ppEnabledExtensionNames = (const gchar**)device_exts->pdata;

    VkDevice device;
    result = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create VkDevice instance\n");
    }

    VkQueue graphics_queue;
    vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);

    VkSurfaceCapabilitiesKHR surface_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps);

    uint32_t format_cnt = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_cnt, NULL);
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * format_cnt);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_cnt, formats);

    uint32_t present_mode_cnt = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_cnt, NULL);
    VkPresentModeKHR* present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_cnt);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_cnt, present_modes);
    
    VkSurfaceFormatKHR surface_format;
    i = 0;
    for(i; i < format_cnt; i++) {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format = formats[i];
                break;
            }
    }

    VkPresentModeKHR present_mode;
    i = 0;
    for(i; i < present_mode_cnt; i++) {
        if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = present_modes[i];
            break;
        }

        present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D extent;
    if(surface_caps.currentExtent.width != UINT32_MAX) {
        extent = surface_caps.currentExtent;
    } else {
        VkExtent2D actual = { WINDOW_WIDTH, WINDOW_HEIGHT };
        actual.width = MAX(surface_caps.minImageExtent.width, MIN(surface_caps.maxImageExtent.width, actual.width));
        actual.height = MAX(surface_caps.minImageExtent.height, MIN(surface_caps.maxImageExtent.height, actual.height));

        extent = actual;
    }
    
    VkSwapchainCreateInfoKHR swap_create_info;
    memset(&swap_create_info, 0, sizeof(VkSwapchainCreateInfoKHR));
    swap_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_create_info.surface = surface;
    swap_create_info.minImageCount = surface_caps.minImageCount;
    swap_create_info.imageFormat = surface_format.format;
    swap_create_info.imageColorSpace = surface_format.colorSpace;
    swap_create_info.imageExtent = extent;
    swap_create_info.imageArrayLayers = 1;
    swap_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_create_info.queueFamilyIndexCount = 0;
    swap_create_info.pQueueFamilyIndices = NULL;
    swap_create_info.preTransform = surface_caps.currentTransform;
    swap_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_create_info.presentMode = present_mode;
    swap_create_info.clipped = VK_TRUE;
    swap_create_info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain;
    result = vkCreateSwapchainKHR(device, &swap_create_info, NULL, &swapchain);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan swapchain");
    }

    uint32_t image_cnt = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &image_cnt, NULL);
    VkImage* images = malloc(sizeof(VkImage) * image_cnt);
    vkGetSwapchainImagesKHR(device, swapchain, &image_cnt, images);

    VkImageView* image_views = malloc(sizeof(VkImageView) * image_cnt);
    i = 0;
    for(i; i < image_cnt; i++) {
        VkImageViewCreateInfo imgview_create_info;
        imgview_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgview_create_info.image = images[i];
        imgview_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgview_create_info.format = surface_format.format;
    }

    int running = SDL_TRUE;
    while (running)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_QUIT:
                {
                    running = SDL_FALSE;
                } break;

                default:
                    break;
            }
        }
    }
    
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}