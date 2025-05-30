#include "Application.hpp"
#include <Logger.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

#include <Database.hpp>

namespace nsudb
{

    namespace ImGuiUtils
    {
        // Data
        static VkAllocationCallbacks* g_Allocator = nullptr;
        static VkInstance g_Instance              = VK_NULL_HANDLE;
        static VkPhysicalDevice g_PhysicalDevice  = VK_NULL_HANDLE;
        static VkDevice g_Device                  = VK_NULL_HANDLE;
        static uint32_t g_QueueFamily             = (uint32_t)-1;
        static VkQueue g_Queue                    = VK_NULL_HANDLE;
        static VkPipelineCache g_PipelineCache    = VK_NULL_HANDLE;
        static VkDescriptorPool g_DescriptorPool  = VK_NULL_HANDLE;

        static ImGui_ImplVulkanH_Window g_MainWindowData;
        static uint32_t g_MinImageCount = 2;
        static bool g_SwapChainRebuild  = false;

        static void glfw_error_callback(int error, const char* description)
        {
            fprintf(stderr, "GLFW Error %d: %s\n", error, description);
        }

        static void check_vk_result(VkResult err)
        {
            if (err == VK_SUCCESS) return;
            fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
            if (err < 0) abort();
        }

        static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
        {
            for (const VkExtensionProperties& p : properties)
                if (strcmp(p.extensionName, extension) == 0) return true;
            return false;
        }

        static void SetupVulkan(ImVector<const char*> instance_extensions)
        {
            VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
            volkInitialize();
#endif

            // Create Vulkan Instance
            {
                VkInstanceCreateInfo create_info = {};
                create_info.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

                // Enumerate available extensions
                uint32_t properties_count;
                ImVector<VkExtensionProperties> properties;
                vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
                properties.resize(properties_count);
                err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
                check_vk_result(err);

                // Enable required extensions
                if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
                    instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
                if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
                {
                    instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
                }
#endif

                // Create Vulkan Instance
                create_info.enabledExtensionCount   = (uint32_t)instance_extensions.Size;
                create_info.ppEnabledExtensionNames = instance_extensions.Data;
                err                                 = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
                check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
                volkLoadInstance(g_Instance);
#endif
            }

            // Select Physical Device (GPU)
            g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
            IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);

            // Select graphics queue family
            g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
            IM_ASSERT(g_QueueFamily != (uint32_t)-1);

            // Create Logical Device (with 1 queue)
            {
                ImVector<const char*> device_extensions;
                device_extensions.push_back("VK_KHR_swapchain");

                // Enumerate physical device extension
                uint32_t properties_count;
                ImVector<VkExtensionProperties> properties;
                vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
                properties.resize(properties_count);
                vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
                if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
                    device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

                const float queue_priority[]          = {1.0f};
                VkDeviceQueueCreateInfo queue_info[1] = {};
                queue_info[0].sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[0].queueFamilyIndex        = g_QueueFamily;
                queue_info[0].queueCount              = 1;
                queue_info[0].pQueuePriorities        = queue_priority;
                VkDeviceCreateInfo create_info        = {};
                create_info.sType                     = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                create_info.queueCreateInfoCount      = sizeof(queue_info) / sizeof(queue_info[0]);
                create_info.pQueueCreateInfos         = queue_info;
                create_info.enabledExtensionCount     = (uint32_t)device_extensions.Size;
                create_info.ppEnabledExtensionNames   = device_extensions.Data;
                err                                   = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
                check_vk_result(err);
                vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
            }

            // Create Descriptor Pool
            // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
            {
                VkDescriptorPoolSize pool_sizes[] = {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
                };
                VkDescriptorPoolCreateInfo pool_info = {};
                pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                pool_info.maxSets                    = 0;
                for (VkDescriptorPoolSize& pool_size : pool_sizes)
                    pool_info.maxSets += pool_size.descriptorCount;
                pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
                pool_info.pPoolSizes    = pool_sizes;
                err                     = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
                check_vk_result(err);
            }
        }

        // All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
        // Your real engine/app may not use them.
        static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
        {
            wd->Surface = surface;

            // Check for WSI support
            VkBool32 res;
            vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
            if (res != VK_TRUE)
            {
                fprintf(stderr, "Error no WSI support on physical device 0\n");
                exit(-1);
            }

            // Select Surface Format
            const VkFormat requestSurfaceImageFormat[]     = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM,
                                                              VK_FORMAT_R8G8B8_UNORM};
            const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            wd->SurfaceFormat =
                ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat,
                                                      (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

            VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
            wd->PresentMode =
                ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
            // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

            // Create SwapChain, RenderPass, Framebuffer, etc.
            IM_ASSERT(g_MinImageCount >= 2);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height,
                                                   g_MinImageCount);
        }

        static void CleanupVulkan()
        {
            vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

            vkDestroyDevice(g_Device, g_Allocator);
            vkDestroyInstance(g_Instance, g_Allocator);
        }

        static void CleanupVulkanWindow()
        {
            ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
        }

        static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
        {
            VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
            VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
            VkResult err{};
            ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
            {
                err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);  // wait indefinitely instead of periodically checking
                check_vk_result(err);

                err = vkResetFences(g_Device, 1, &fd->Fence);
                check_vk_result(err);
            }

            err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) g_SwapChainRebuild = true;
            if (err == VK_ERROR_OUT_OF_DATE_KHR) return;
            if (err != VK_SUBOPTIMAL_KHR) check_vk_result(err);

            {
                err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
                check_vk_result(err);
                VkCommandBufferBeginInfo info = {};
                info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
                check_vk_result(err);
            }
            {
                VkRenderPassBeginInfo info    = {};
                info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass               = wd->RenderPass;
                info.framebuffer              = fd->Framebuffer;
                info.renderArea.extent.width  = wd->Width;
                info.renderArea.extent.height = wd->Height;
                info.clearValueCount          = 1;
                info.pClearValues             = &wd->ClearValue;
                vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            }

            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

            // Submit command buffer
            vkCmdEndRenderPass(fd->CommandBuffer);
            {
                VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSubmitInfo info               = {};
                info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                info.waitSemaphoreCount         = 1;
                info.pWaitSemaphores            = &image_acquired_semaphore;
                info.pWaitDstStageMask          = &wait_stage;
                info.commandBufferCount         = 1;
                info.pCommandBuffers            = &fd->CommandBuffer;
                info.signalSemaphoreCount       = 1;
                info.pSignalSemaphores          = &render_complete_semaphore;

                err = vkEndCommandBuffer(fd->CommandBuffer);
                check_vk_result(err);
                err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
                check_vk_result(err);
            }
        }

        static void FramePresent(ImGui_ImplVulkanH_Window* wd)
        {
            if (g_SwapChainRebuild) return;
            VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
            VkPresentInfoKHR info                 = {};
            info.sType                            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            info.waitSemaphoreCount               = 1;
            info.pWaitSemaphores                  = &render_complete_semaphore;
            info.swapchainCount                   = 1;
            info.pSwapchains                      = &wd->Swapchain;
            info.pImageIndices                    = &wd->FrameIndex;
            VkResult err                          = vkQueuePresentKHR(g_Queue, &info);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) g_SwapChainRebuild = true;
            if (err == VK_ERROR_OUT_OF_DATE_KHR) return;
            if (err != VK_SUBOPTIMAL_KHR) check_vk_result(err);
            wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;  // Now we can use the next set of semaphores
        }

    }  // namespace ImGuiUtils

    static int32_t s_SelectedQueryIndex                        = -1;
    static const std::vector<std::string> s_HardcodedDbQueries = {
        // 1
        R"(SELECT b.outlet_id, o.address, ot.name AS outlet_type
       FROM branches b
       JOIN outlets o ON b.outlet_id = o.id
       JOIN outlet_types ot ON o.type_id = ot.id;)",

        R"(SELECT k.outlet_id, o.address, ot.name AS outlet_type, k.branch_id
       FROM kiosks k
       JOIN outlets o ON k.outlet_id = o.id
       JOIN outlet_types ot ON o.type_id = ot.id;)",

        R"(SELECT o.id AS outlet_id, o.address, ot.name AS outlet_type
       FROM outlets o
       JOIN outlet_types ot ON o.type_id = ot.id;)",

        R"(SELECT COUNT(*) AS total_order_points FROM outlets;)",

        // 2
        R"(SELECT b.outlet_id AS branch_outlet_id,
              COUNT(o.id) AS orders_count
       FROM branches b
       LEFT JOIN orders o ON o.outlet_id = b.outlet_id
           AND o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
       GROUP BY b.outlet_id
       ORDER BY b.outlet_id;)",

        R"(SELECT k.outlet_id AS kiosk_outlet_id,
              COUNT(o.id) AS orders_count
       FROM kiosks k
       LEFT JOIN orders o ON o.outlet_id = k.outlet_id
           AND o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
       GROUP BY k.outlet_id
       ORDER BY k.outlet_id;)",

        R"(SELECT COUNT(*) AS total_orders
       FROM orders
       WHERE accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp;)",

        // 3
        R"(WITH filtered_orders AS (
           SELECT o.*, so.service_type_id, so.count, so.id AS service_order_id
           FROM orders o
           LEFT JOIN service_orders so ON o.id = so.order_id
           WHERE o.accept_time BETWEEN '2024-05-20 10:00:00' AND '2024-05-22 16:45:00'
             AND o.outlet_id IN (1, 3)
       )
       SELECT fo.service_type_id,
              st.name AS service_name,
              fo.is_urgent,
              COUNT(DISTINCT fo.id) AS orders_count
       FROM filtered_orders fo
       LEFT JOIN service_types st ON fo.service_type_id = st.id
       GROUP BY fo.service_type_id, st.name, fo.is_urgent
       ORDER BY st.name, fo.is_urgent;)",

        // 4
        R"(CREATE OR REPLACE VIEW filtered_orders AS
       SELECT o.*
       FROM orders o
       WHERE o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
         AND (o.outlet_id = 1 OR o.outlet_id = 3);

       CREATE OR REPLACE VIEW service_order_sums AS
       SELECT o.is_urgent, so.service_type_id, SUM(o.overall_price) AS revenue
       FROM filtered_orders o
       JOIN service_orders so ON o.id = so.order_id
       GROUP BY o.is_urgent, so.service_type_id;

       SELECT sos.service_type_id,
              st.name AS service_name,
              sos.is_urgent,
              sos.revenue
       FROM service_order_sums sos
       JOIN service_types st ON sos.service_type_id = st.id
       ORDER BY st.name, sos.is_urgent;)",

        // 5
        R"(SELECT o.is_urgent, SUM(f.amount) AS total_printed_photos
       FROM frames f
       JOIN print_orders po ON f.print_order_id = po.id
       JOIN orders o ON po.order_id = o.id
       WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
         AND o.outlet_id IN (SELECT outlet_id FROM branches)
       GROUP BY o.is_urgent;

       SELECT o.is_urgent, SUM(f.amount) AS total_printed_photos
       FROM frames f
       JOIN print_orders po ON f.print_order_id = po.id
       JOIN orders o ON po.order_id = o.id
       WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
         AND o.outlet_id IN (SELECT outlet_id FROM kiosks)
       GROUP BY o.is_urgent;

       SELECT o.is_urgent, SUM(f.amount) AS total_printed_photos
       FROM frames f
       JOIN print_orders po ON f.print_order_id = po.id
       JOIN orders o ON po.order_id = o.id
       WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
       GROUP BY o.is_urgent;)",

        // 6
        R"(SELECT o.is_urgent, COUNT(f.id) AS total_films
       FROM films f
       JOIN service_orders so ON f.service_order_id = so.id
       JOIN orders o ON o.id = so.order_id
       WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
         AND o.outlet_id IN (SELECT outlet_id FROM branches WHERE outlet_id = 1)
       GROUP BY o.is_urgent;

       SELECT o.is_urgent, COUNT(f.id) AS total_films
       FROM films f
       JOIN service_orders so ON f.service_order_id = so.id
       JOIN orders o ON o.id = so.order_id
       WHERE o.accept_time BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
         AND o.outlet_id IN (SELECT outlet_id FROM kiosks WHERE outlet_id = 3)
       GROUP BY o.is_urgent;)",

        // 7
        R"(SELECT DISTINCT v.id AS vendor_id, v.name AS vendor_name,
                      i.id AS item_id, i.name AS item_name,
                      di.quantity, di.price, d.date
       FROM vendors v
       JOIN deliveries d ON d.vendor_id = v.id
       JOIN delivery_items di ON di.delivery_id = d.id
       JOIN items i ON i.id = di.item_id
       WHERE d.date BETWEEN '2024-01-01 10:00:00'::timestamp AND '2024-12-30 16:45:00'::timestamp
         AND di.quantity >= 2
       ORDER BY v.id;)",

        // 8
        R"(SELECT DISTINCT c.id AS client_id, c.full_name, c.discount,
                      o.id AS order_id, o.overall_price, o.outlet_id
       FROM clients c
       JOIN orders o ON o.client_id = c.id
       WHERE c.discount > 0
         AND o.overall_price >= 5
         AND o.outlet_id = 3
       ORDER BY c.id;)",

        // 9
        R"(SELECT COALESCE(SUM(stni.count * i.price * so.count), 0) AS total_revenue
       FROM orders o
       JOIN service_orders so ON so.order_id = o.id
       JOIN service_types_needed_items stni ON stni.service_type_id = so.service_type_id
       JOIN items i ON i.id = stni.item_id
       WHERE o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
         AND o.outlet_id = 1;)",

        // 10
        R"(WITH orders_in_branch AS (
           SELECT o.id FROM orders o WHERE o.outlet_id = 1
       ),
       service_orders_in_branch AS (
           SELECT so.* FROM service_orders so
           JOIN orders_in_branch ob ON so.order_id = ob.id
       ),
       items_demand_in_branch AS (
           SELECT stni.item_id, SUM(stni.count * so.count) AS total_quantity
           FROM service_orders_in_branch so
           JOIN service_types_needed_items stni ON so.service_type_id = stni.service_type_id
           GROUP BY stni.item_id
       ),
       items_demand_overall AS (
           SELECT stni.item_id, SUM(stni.count * so.count) AS total_quantity
           FROM service_orders so
           JOIN service_types_needed_items stni ON so.service_type_id = stni.service_type_id
           GROUP BY stni.item_id
       )
       SELECT i.id AS item_id, i.name AS item_name, f.name AS firm_name,
              COALESCE(d_branch.total_quantity, 0) AS demand_in_branch,
              COALESCE(d_overall.total_quantity, 0) AS demand_overall
       FROM items i
       LEFT JOIN firms f ON i.firm_id = f.id
       LEFT JOIN items_demand_in_branch d_branch ON i.id = d_branch.item_id
       LEFT JOIN items_demand_overall d_overall ON i.id = d_overall.item_id
       WHERE COALESCE(d_branch.total_quantity, 0) > 0 OR COALESCE(d_overall.total_quantity, 0) > 0
       ORDER BY demand_overall DESC, demand_in_branch DESC;)",

        // 11
        R"(WITH filtered_orders AS (
           SELECT o.id
           FROM orders o
           WHERE o.accept_time BETWEEN '2024-05-20 10:00:00'::timestamp AND '2024-05-22 16:45:00'::timestamp
             AND o.outlet_id = 1
       ),
       service_orders_filtered AS (
           SELECT so.*
           FROM service_orders so
           JOIN filtered_orders fo ON so.order_id = fo.id
       ),
       items_sold AS (
           SELECT stni.item_id, SUM(stni.count * so.count) AS total_quantity
           FROM service_orders_filtered so
           JOIN service_types_needed_items stni ON so.service_type_id = stni.service_type_id
           GROUP BY stni.item_id
       )
       SELECT i.id AS item_id, i.name AS item_name, f.name AS firm_name,
              COALESCE(items_sold.total_quantity, 0) AS quantity_sold
       FROM items i
       LEFT JOIN firms f ON i.firm_id = f.id
       LEFT JOIN items_sold ON i.id = items_sold.item_id
       WHERE items_sold.total_quantity IS NOT NULL
       ORDER BY quantity_sold DESC;)",

        // 12
        R"(SELECT o.id AS outlet_id, o.address, ot.name AS outlet_type
       FROM outlets o
       JOIN outlet_types ot ON o.type_id = ot.id
       ORDER BY o.id;

       SELECT o.id AS outlet_id, o.address, ot.name AS outlet_type
       FROM outlets o
       JOIN outlet_types ot ON o.type_id = ot.id
       WHERE ot.name = 'kiosk'
       ORDER BY o.id;)"};

    void Application::Run() noexcept
    {
        using namespace ImGuiUtils;

        // Our state
        ImGuiIO& io                  = ImGui::GetIO();
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        ImVec4 clear_color           = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // App state
        std::optional<QueryResult> tableQueryResult{};

        char sqlQueryBuffer[8192] = "SELECT * FROM outlet_types";  // Query input buffer
        std::optional<QueryResult> lastQueryResult{std::nullopt};  // Stores the last executed query result
        std::vector<std::string> tableNames{};
        uint32_t selectedTableIndex{};

        constexpr const char* queryTableNames = "SELECT tablename\n"
                                                "FROM pg_tables\n"
                                                "WHERE schemaname = 'public'\n"
                                                "ORDER BY tablename";

        DatabaseDesc dbDesc = {};
        dbDesc.HostName.resize(32, 0);
        dbDesc.Database.resize(32, 0);
        dbDesc.Username.resize(32, 0);
        dbDesc.Password.resize(32, 0);

        static bool s_bShowDbConnWindow      = true;  // On startup we have to enter db options first.
        static bool s_bShowAppSettingsWindow = false;

        // Main loop
        while (!glfwWindowShouldClose(m_Window))
        {
            // Poll and handle events (inputs, m_Window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy
            // of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your
            // copy of the keyboard data. Generally you may always pass all inputs to dear imgui, and hide them from your application based
            // on those two flags.
            glfwPollEvents();

            // Resize swap chain?
            int fb_width, fb_height;
            glfwGetFramebufferSize(m_Window, &fb_width, &fb_height);
            if (fb_width > 0 && fb_height > 0 &&
                (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
            {
                ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily,
                                                       g_Allocator, fb_width, fb_height, g_MinImageCount);
                g_MainWindowData.FrameIndex = 0;
                g_SwapChainRebuild          = false;
            }
            if (glfwGetWindowAttrib(m_Window, GLFW_ICONIFIED) != 0)
            {
                ImGui_ImplGlfw_Sleep(10);
                continue;
            }

            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // IMGUI - BEGIN FRAME

            {
                const auto BeginDockspace = [&](const char* dockspaceName)
                {
                    assert(dockspaceName);
                    static ImGuiDockNodeFlags dockspace_flags =
                        ImGuiDockNodeFlags_None | /*ImGuiDockNodeFlags_NoUndocking |*/ ImGuiDockNodeFlags_AutoHideTabBar;

                    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
                    const ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImGui::SetNextWindowPos(viewport->WorkPos);
                    ImGui::SetNextWindowSize(viewport->WorkSize);
                    ImGui::SetNextWindowViewport(viewport->ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    window_flags |=
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin(dockspaceName, nullptr, window_flags);
                    ImGui::PopStyleVar();
                    ImGui::PopStyleVar(2);  // Pop WindowRounding and WindowBorderSize

                    // DockSpace
                    ImGuiIO& io = ImGui::GetIO();
                    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
                    {
                        ImGuiID dockspace_id = ImGui::GetID(dockspaceName);

                        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                    }
                };
                const auto EndDockspace = [&]()
                {
                    ImGui::End();  // End DockSpace Demo window
                };

                BeginDockspace("nsudb_dockspace");

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("Connect to Database...")) s_bShowDbConnWindow = true;

                        if (ImGui::MenuItem("Close Database"))
                        {
                            tableQueryResult   = {};
                            selectedTableIndex = {};
                            tableNames.clear();
                            m_DbConn.reset();
                        }

                        if (ImGui::MenuItem("Open Settings")) s_bShowAppSettingsWindow = true;

                        ImGui::Separator();
                        if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(m_Window, true);

                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                if (s_bShowDbConnWindow)
                {
                    ImGui::OpenPopup("Open NSU Database##popup");  // Open the popup with a unique ID

                    // Always center the popup for better UX
                    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                    if (ImGui::BeginPopupModal("Open NSU Database##popup", &s_bShowDbConnWindow,
                                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
                    {
                        ImGui::Text("Enter connection options:");
                        ImGui::Separator();

                        ImGui::InputText("Host", dbDesc.HostName.data(), dbDesc.HostName.size());
                        ImGui::InputText("Database", dbDesc.Database.data(), dbDesc.Database.size());
                        ImGui::InputText("Username", dbDesc.Username.data(), dbDesc.Username.size());
                        ImGui::InputText("Password", dbDesc.Password.data(), dbDesc.Password.size(),
                                         ImGuiInputTextFlags_Password);  // Password field
                        ImGui::InputInt("Port", &dbDesc.Port);
                        ImGui::Separator();

                        // --- Calculate button size dynamically ---
                        // Get the size of the "Connect" and "Cancel" text
                        ImVec2 connect_text_size = ImGui::CalcTextSize("Connect");
                        ImVec2 cancel_text_size  = ImGui::CalcTextSize("Cancel");

                        // Determine the widest text to make both buttons the same width
                        float button_width = std::max(connect_text_size.x, cancel_text_size.x);

                        // Add some padding (e.g., 2 times style.FramePadding.x) to account for button frame
                        // This is a common practice to make buttons feel more natural.
                        button_width += ImGui::GetStyle().FramePadding.x * 2.0f;  // Each side has padding

                        if (ImGui::Button("Connect", ImVec2(button_width, 0)))
                        {
                            m_DbConn = std::make_unique<DatabaseConnection>(dbDesc);
                            if (!m_DbConn->TryConnectIfNotConnected()) m_DbConn.reset();

                            LOG_TRACE("Attempting to connect to database:");
                            LOG_TRACE("  Host: {}", dbDesc.HostName);
                            LOG_TRACE("  Port: {}", dbDesc.Port);
                            LOG_TRACE("  Database: {}", dbDesc.Database);
                            LOG_TRACE("  Username: {}", dbDesc.Username);

                            s_bShowDbConnWindow = false;  // Close the popup
                            ImGui::CloseCurrentPopup();

                            // reset state
                            dbDesc.HostName.resize(32, 0);
                            dbDesc.Database.resize(32, 0);
                            dbDesc.Username.resize(32, 0);
                            dbDesc.Password.resize(32, 0);
                            dbDesc.Port = DatabaseDesc{}.Port;
                        }
                        ImGui::SetItemDefaultFocus();  // Set focus to the connect button
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
                        {
                            s_bShowDbConnWindow = false;  // Close the popup
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (s_bShowAppSettingsWindow)
                {
                    ImGui::OpenPopup("Open NSU App Settings##popup");  // Open the popup with a unique ID

                    // Always center the popup for better UX
                    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                    if (ImGui::BeginPopupModal("Open NSU App Settings##popup", &s_bShowAppSettingsWindow,
                                               ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::SliderFloat("Font Scale", &io.FontGlobalScale, 1.0f, 20.0f);

                        ImGui::EndPopup();
                    }
                }

                static const ImGuiWindowFlags_ dbWindowFlags = {};  // ImGuiWindowFlags_NoMove;

                // Queries
                if (ImGui::Begin("SQL", nullptr, dbWindowFlags))
                {
                    // ����� ������� �� ������
                    ImGui::Text("Predefined Queries:");

                    // ������ ������, ����� �������� dangling pointer
                    static std::vector<std::string> queryLabelsStorage;
                    static std::vector<const char*> queryLabels;

                    queryLabelsStorage.clear();
                    queryLabels.clear();
                    queryLabelsStorage.reserve(s_HardcodedDbQueries.size());
                    queryLabels.reserve(s_HardcodedDbQueries.size());

                    for (size_t i = 0; i < s_HardcodedDbQueries.size(); ++i)
                    {
                        queryLabelsStorage.emplace_back("Query " + std::to_string(i + 1));
                        queryLabels.push_back(queryLabelsStorage.back().c_str());
                    }

                    if (ImGui::Combo("##PredefinedQueries", &s_SelectedQueryIndex, queryLabels.data(),
                                     static_cast<int>(queryLabels.size())))
                    {
                        if (s_SelectedQueryIndex >= 0 && s_SelectedQueryIndex < static_cast<int>(s_HardcodedDbQueries.size()))
                        {
                            strncpy(sqlQueryBuffer, s_HardcodedDbQueries[s_SelectedQueryIndex].c_str(), sizeof(sqlQueryBuffer) - 1);
                            sqlQueryBuffer[sizeof(sqlQueryBuffer) - 1] = '\0';  // safety null-termination
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Copy to Editor") && s_SelectedQueryIndex >= 0 &&
                        s_SelectedQueryIndex < static_cast<int>(s_HardcodedDbQueries.size()))
                    {
                        strncpy(sqlQueryBuffer, s_HardcodedDbQueries[s_SelectedQueryIndex].c_str(), sizeof(sqlQueryBuffer) - 1);
                        sqlQueryBuffer[sizeof(sqlQueryBuffer) - 1] = '\0';
                    }

                    ImGui::Separator();

                    // �������� �������
                    ImGui::Text("SQL Query Editor:");
                    ImGui::InputTextMultiline("##SQLQuery", sqlQueryBuffer, sizeof(sqlQueryBuffer),
                                              ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y * 0.4f), ImGuiInputTextFlags_AllowTabInput);

                    // ������ ����������
                    if (m_DbConn && ImGui::Button("Run Query"))
                    {
                        lastQueryResult = m_DbConn->Execute(sqlQueryBuffer);
                    }

                    ImGui::SameLine();
                    ImGui::TextDisabled("(Cmd+Enter / Ctrl+Enter to run)");
                    ImGui::SameLine();
                    if (ImGui::Button("Clear Result"))
                    {
                        lastQueryResult = std::nullopt;
                    }

                    // ����� ����������
                    if (lastQueryResult && !lastQueryResult->ColumnNames.empty())
                    {
                        ImGui::Separator();
                        ImGui::Text("Result: %zu rows, %zu cols", lastQueryResult->Rows.size(), lastQueryResult->ColumnNames.size());

                        if (ImGui::BeginTable("SQLQueryResultTable", lastQueryResult->ColumnNames.size(),
                                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                                  ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
                        {
                            for (const auto& colName : lastQueryResult->ColumnNames)
                            {
                                ImGui::TableSetupColumn(colName.c_str());
                            }
                            ImGui::TableHeadersRow();

                            for (const auto& row : lastQueryResult->Rows)
                            {
                                ImGui::TableNextRow();
                                for (size_t j = 0; j < row.size(); ++j)
                                {
                                    ImGui::TableSetColumnIndex(j);
                                    ImGui::TextUnformatted(row[j].c_str());
                                }
                            }

                            ImGui::EndTable();
                        }
                    }
                }
                ImGui::End();

                // Tables
                {
                    if (ImGui::Begin("TABLES", nullptr, dbWindowFlags) && m_DbConn)
                    {
                        // Fetch table names only once, or when a refresh is triggered
                        if (tableNames.empty())
                        {
                            if (const auto queryResult = m_DbConn->Execute(queryTableNames); queryResult)
                            {
                                tableNames.clear();  // Clear previous list
                                for (const auto& row : queryResult->Rows)
                                    if (!row.empty()) tableNames.emplace_back(row[0]);  // tablename is the first column

                                std::sort(tableNames.begin(), tableNames.end());  // Keep it sorted
                            }
                        }

                        // Left Pane: List of Tables
                        ImGui::BeginChild("##TableList", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, 0),
                                          true);  // 30% width for table list
                        ImGui::Text("Database Tables:");
                        ImGui::Separator();

                        const char* selectedTableName = tableNames[selectedTableIndex].c_str();
                        for (uint32_t i{}; i < tableNames.size(); ++i)
                        {
                            const auto& currentTableName = tableNames[i];
                            if (!ImGui::Selectable(currentTableName.c_str(), selectedTableIndex == i) || selectedTableIndex == i) continue;

                            selectedTableIndex = i;
                            selectedTableName  = tableNames[selectedTableIndex].c_str();
                            // Query content of the selected table
                            const std::string queryContent =
                                "SELECT * FROM " + std::string(selectedTableName) + " LIMIT 100;";  // Add LIMIT for safety

                            tableQueryResult = m_DbConn->Execute(queryContent);
                        }
                        ImGui::EndChild();

                        ImGui::SameLine();

                        // Right Pane: Table Content
                        ImGui::BeginChild("##TableContent", ImVec2(0, 0), true);  // Remaining width
                        if (selectedTableName)
                        {
                            ImGui::Text("Content of table: %s", selectedTableName);
                            ImGui::Separator();

                            if (tableQueryResult && !tableQueryResult->Rows.empty())
                            {
                                // Display table header
                                if (ImGui::BeginTable("##TableData", tableQueryResult->ColumnNames.size(),
                                                      ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                                          ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY))
                                {
                                    for (const auto& colName : tableQueryResult->ColumnNames)
                                        ImGui::TableSetupColumn(colName.c_str());

                                    ImGui::TableHeadersRow();

                                    // Display table rows
                                    for (const auto& row : tableQueryResult->Rows)
                                    {
                                        ImGui::TableNextRow();
                                        for (uint32_t col{}; col < row.size(); ++col)
                                        {
                                            ImGui::TableSetColumnIndex(col);
                                            ImGui::TextUnformatted(row[col].c_str());
                                        }
                                    }
                                    ImGui::EndTable();
                                }
                            }
                        }

                        ImGui::EndChild();
                    }
                    ImGui::End();
                }

                // ImGui::ShowDemoWindow();

                EndDockspace();
            }

            // IMGUI - END FRAME

            // Rendering
            ImGui::Render();
            ImDrawData* main_draw_data      = ImGui::GetDrawData();
            const bool main_is_minimized    = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            wd->ClearValue.color.float32[3] = clear_color.w;
            if (!main_is_minimized) FrameRender(wd, main_draw_data);

            // Update and Render additional Platform Windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            // Present Main Platform m_Window
            if (!main_is_minimized) FramePresent(wd);
        }
    }

    Application::Application() noexcept : m_DbConn(nullptr)
    {
        Logger::Init();
        Init();
        LOG_TRACE("Application started.");
    }

    Application::~Application() noexcept
    {
        Shutdown();
        m_DbConn.reset();
        LOG_TRACE("Application shutdown.");
        Logger::Shutdown();
    }

    void Application::Init() noexcept
    {
        using namespace ImGuiUtils;

        glfwSetErrorCallback(glfw_error_callback);
        bool glfwErr = glfwInit();
        assert(glfwErr);

        uint32_t width{780}, height{540};

        // Create m_Window with Vulkan context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(width, height, "NSU DB / ImGui C++ Vulkan PGFE", nullptr, nullptr);
        glfwErr  = glfwVulkanSupported();
        assert(glfwErr);

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);  // no resize

        // center glfw window
        {
            // Get the primary monitor and its video mode
            GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* vidMode  = glfwGetVideoMode(primaryMonitor);

            // Calculate centered position
            int xPos = (vidMode->width - width) / 2;
            int yPos = (vidMode->height - height) / 2;

            // Set window position
            glfwSetWindowPos(m_Window, xPos, yPos);
        }

        ImVector<const char*> extensions;
        uint32_t extensions_count    = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        for (uint32_t i = 0; i < extensions_count; i++)
            extensions.push_back(glfw_extensions[i]);
        SetupVulkan(extensions);

        // Create m_Window Surface
        VkSurfaceKHR surface;
        VkResult err = glfwCreateWindowSurface(g_Instance, m_Window, g_Allocator, &surface);
        check_vk_result(err);

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(m_Window, &w, &h);
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        SetupVulkanWindow(wd, surface, w, h);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
        //  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
        // io.ConfigViewportsNoAutoMerge = true;
        // io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        // ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // custom theme: https://github.com/shivang51/bess/blob/main/Bess/src/settings/themes.cpp#L38
        {
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec4* colors    = style.Colors;

            // Primary background
            colors[ImGuiCol_WindowBg]  = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);  // #131318
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);  // #131318

            colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

            // Headers
            colors[ImGuiCol_Header]        = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
            colors[ImGuiCol_HeaderActive]  = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

            // Buttons
            colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
            colors[ImGuiCol_ButtonActive]  = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

            // Frame BG
            colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
            colors[ImGuiCol_FrameBgActive]  = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

            // Tabs
            colors[ImGuiCol_Tab]                = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
            colors[ImGuiCol_TabHovered]         = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
            colors[ImGuiCol_TabActive]          = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
            colors[ImGuiCol_TabUnfocused]       = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

            // Title
            colors[ImGuiCol_TitleBg]          = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
            colors[ImGuiCol_TitleBgActive]    = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

            // Borders
            colors[ImGuiCol_Border]       = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
            colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

            // Text
            colors[ImGuiCol_Text]         = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

            // Highlights
            colors[ImGuiCol_CheckMark]         = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrab]        = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
            colors[ImGuiCol_ResizeGrip]        = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
            colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
            colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

            // Scrollbar
            colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
            colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

            // Style tweaks
            style.WindowRounding    = 5.0f;
            style.FrameRounding     = 5.0f;
            style.GrabRounding      = 5.0f;
            style.TabRounding       = 5.0f;
            style.PopupRounding     = 5.0f;
            style.ScrollbarRounding = 5.0f;
            style.WindowPadding     = ImVec2(10, 10);
            style.FramePadding      = ImVec2(6, 4);
            style.ItemSpacing       = ImVec2(8, 6);
            style.PopupBorderSize   = 0.f;
        }

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding              = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        // init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will
        // default to header version.
        init_info.Instance        = g_Instance;
        init_info.PhysicalDevice  = g_PhysicalDevice;
        init_info.Device          = g_Device;
        init_info.QueueFamily     = g_QueueFamily;
        init_info.Queue           = g_Queue;
        init_info.PipelineCache   = g_PipelineCache;
        init_info.DescriptorPool  = g_DescriptorPool;
        init_info.RenderPass      = wd->RenderPass;
        init_info.Subpass         = 0;
        init_info.MinImageCount   = g_MinImageCount;
        init_info.ImageCount      = wd->ImageCount;
        init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator       = g_Allocator;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
        // ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an
        // assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
        // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // io.Fonts->AddFontDefault();
        // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr,
        // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);
    }

    void Application::Shutdown() noexcept
    {
        using namespace ImGuiUtils;
        check_vk_result(vkDeviceWaitIdle(g_Device));
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        CleanupVulkanWindow();
        CleanupVulkan();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

}  // namespace nsudb
