//
// Created by Monika on 15.09.2023.
//

#include <Graphics/Overlay/VulkanImGuiOverlay.h>
#include <Graphics/Pipeline/Vulkan/VulkanTracy.h>
#include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#include <Graphics/Pipeline/Vulkan/VulkanKernel.h>
#include <Graphics/Pipeline/Vulkan/VulkanMemory.h>
#include <Graphics/Window/Window.h>
#include <Graphics/GUI/WidgetManager.h>
#include <Graphics/GUI/Widget.h>
#include <Graphics/GUI/ImGUI.h>

#include <EvoVulkan/Types/DescriptorPool.h>
#include <EvoVulkan/DescriptorManager.h>

#include <Utils/Common/Features.h>
#include <Utils/Common/SubscriptionMessage.h>

#ifdef SR_WIN32
    extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#ifdef defined(SR_LINUX) && defined(SR_RENDER_GLFW)
    #include <imgui/backends/imgui_impl_glfw.h>
#endif

#ifdef SR_ANDROID
    #include <Graphics/Window/AndroidWindow.h>
    #include <imgui/backends/imgui_impl_android.h>
#endif

namespace SR_GRAPH_NS {
#ifdef SR_WIN32
    static LRESULT ImGui_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui::GetCurrentContext() == NULL)
            return 0;

        ImGuiIO& io = ImGui::GetIO();

        switch (msg) {
            case WM_CHAR:
                wchar_t wch;
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *) &wParam, 1, &wch, 1);
                io.AddInputCharacter(wch);
                return 1;
            default:
                break;
        }

        return 0;
    }
#endif

    int CreatePlatformSurface(ImGuiViewport* pv, ImU64 vk_inst, const void* vk_allocators, ImU64* out_vk_surface) {
    #ifdef SR_WIN32
        VkWin32SurfaceCreateInfoKHR sci;
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;

        vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(reinterpret_cast<VkInstance>(vk_inst), "vkCreateWin32SurfaceKHR");
        if (!vkCreateWin32SurfaceKHR) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        memset(&sci, 0, sizeof(sci));
        sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        sci.hinstance = GetModuleHandle(NULL);
        sci.hwnd = static_cast<HWND>(pv->PlatformHandleRaw);

        VkResult err = vkCreateWin32SurfaceKHR(reinterpret_cast<VkInstance>(vk_inst), &sci, static_cast<const VkAllocationCallbacks *>(vk_allocators), (VkSurfaceKHR*)out_vk_surface);
        return (int)err;
    #else
        SRHaltOnce("Unsupported platform!");
    return -1;
    #endif
    }

#ifdef SR_WIN32
    LRESULT CustomWindowProcPlatform(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)  {
        if (!SR_GRAPH_NS::ImGui_WndProcHandler(hwnd, msg, wParam, lParam)) {
            if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
                return true;
            }
        }

        switch (msg) {
            case WM_CREATE: {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_CLOSE: {
                auto&& viewport = ImGui::FindViewportByPlatformHandle(hwnd);
                if (auto&& widget = SR_GRAPH_GUI_NS::ViewportsTableManager::Instance().GetWidgetByViewport(viewport)) {
                    widget->Close();
                }
                SR_FALLTHROUGH;
            }
            case WM_DESTROY: {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_SETCURSOR: {
                /// Костыльный фикс курсора для вторичных окон.
                SetClassLongPtr(hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_ARROW)));
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    struct ImGui_ImplWin32_ViewportData
    {
        HWND    Hwnd;
        bool    HwndOwned;
        DWORD   DwStyle;
        DWORD   DwExStyle;

        ImGui_ImplWin32_ViewportData() { Hwnd = NULL; HwndOwned = false;  DwStyle = DwExStyle = 0; }
        ~ImGui_ImplWin32_ViewportData() { IM_ASSERT(Hwnd == NULL); }
    };

    static void ImGui_ImplWin32_GetWin32StyleFromViewportFlags(ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style)
    {
        if (flags & ImGuiViewportFlags_NoDecoration)
            *out_style = WS_POPUP;
        else
            *out_style = WS_OVERLAPPEDWINDOW;

        if (flags & ImGuiViewportFlags_NoTaskBarIcon)
            *out_ex_style = WS_EX_TOOLWINDOW;
        else
            *out_ex_style = WS_EX_APPWINDOW;

        if (flags & ImGuiViewportFlags_TopMost)
            *out_ex_style |= WS_EX_TOPMOST;
    }

    static void ImGui_ImplWin32_CreateWindow(ImGuiViewport* viewport)
    {
        ImGui_ImplWin32_ViewportData* vd = IM_NEW(ImGui_ImplWin32_ViewportData)();
        viewport->PlatformUserData = vd;

        // Select style and parent window
        ImGui_ImplWin32_GetWin32StyleFromViewportFlags(viewport->Flags, &vd->DwStyle, &vd->DwExStyle);
        HWND parent_window = NULL;
        if (viewport->ParentViewportId != 0)
            if (ImGuiViewport* parent_viewport = ImGui::FindViewportByID(viewport->ParentViewportId))
                parent_window = (HWND)parent_viewport->PlatformHandle;

        // Create window
        RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
        ::AdjustWindowRectEx(&rect, vd->DwStyle, FALSE, vd->DwExStyle);
        vd->Hwnd = ::CreateWindowEx(
                vd->DwExStyle, "ImGui Platform", "Untitled", vd->DwStyle,   // Style, class name, window name
                rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,    // Window area
                parent_window, NULL, ::GetModuleHandle(NULL), NULL);                    // Parent window, Menu, Instance, Param
        vd->HwndOwned = true;
        viewport->PlatformRequestResize = false;
        viewport->PlatformHandle = viewport->PlatformHandleRaw = vd->Hwnd;

        vd->DwExStyle &= ~WS_CAPTION;
        vd->DwExStyle &= ~WS_SIZEBOX;
        vd->DwExStyle &= ~WS_SYSMENU;

        SetWindowLong(vd->Hwnd, GWL_STYLE, vd->DwExStyle);
        UpdateWindow(vd->Hwnd);
    }
#endif

    static void (*ImGui_Platform_CreateWindow)(ImGuiViewport* vp) = nullptr;

    static void CheckVulkanResult(VkResult result) {
        if (result != VK_SUCCESS) {
            SR_ERROR("VulkanImGuiOverlay::CheckVulkanError() : vulkan error! Error: {}", EvoVulkan::Tools::Convert::result_to_string(result));
        }
    }

    ImGuiKey KeyCodeToImGuiKey(SR_UTILS_NS::KeyCode keyCode) {
        switch (keyCode) {
            case SR_UTILS_NS::KeyCode::BackSpace:   return ImGuiKey_Backspace;
            case SR_UTILS_NS::KeyCode::Tab:         return ImGuiKey_Tab;
            case SR_UTILS_NS::KeyCode::Enter:       return ImGuiKey_Enter;
            case SR_UTILS_NS::KeyCode::LShift:      return ImGuiKey_LeftShift;
            case SR_UTILS_NS::KeyCode::LCtrl:       return ImGuiKey_LeftCtrl;
            case SR_UTILS_NS::KeyCode::LAlt:        return ImGuiKey_LeftAlt;
            case SR_UTILS_NS::KeyCode::Escape:      return ImGuiKey_Escape;
            case SR_UTILS_NS::KeyCode::Space:       return ImGuiKey_Space;

            case SR_UTILS_NS::KeyCode::LeftArrow:   return ImGuiKey_LeftArrow;
            case SR_UTILS_NS::KeyCode::UpArrow:     return ImGuiKey_UpArrow;
            case SR_UTILS_NS::KeyCode::RightArrow:  return ImGuiKey_RightArrow;
            case SR_UTILS_NS::KeyCode::DownArrow:   return ImGuiKey_DownArrow;

            case SR_UTILS_NS::KeyCode::Delete:      return ImGuiKey_Delete;
            case SR_UTILS_NS::KeyCode::Insert:      return ImGuiKey_Insert;
            case SR_UTILS_NS::KeyCode::Home:        return ImGuiKey_Home;
            case SR_UTILS_NS::KeyCode::End:         return ImGuiKey_End;
            case SR_UTILS_NS::KeyCode::PageUp:      return ImGuiKey_PageUp;
            case SR_UTILS_NS::KeyCode::PageDown:    return ImGuiKey_PageDown;

            case SR_UTILS_NS::KeyCode::_0: return ImGuiKey_0;
            case SR_UTILS_NS::KeyCode::_1: return ImGuiKey_1;
            case SR_UTILS_NS::KeyCode::_2: return ImGuiKey_2;
            case SR_UTILS_NS::KeyCode::_3: return ImGuiKey_3;
            case SR_UTILS_NS::KeyCode::_4: return ImGuiKey_4;
            case SR_UTILS_NS::KeyCode::_5: return ImGuiKey_5;
            case SR_UTILS_NS::KeyCode::_6: return ImGuiKey_6;
            case SR_UTILS_NS::KeyCode::_7: return ImGuiKey_7;
            case SR_UTILS_NS::KeyCode::_8: return ImGuiKey_8;
            case SR_UTILS_NS::KeyCode::_9: return ImGuiKey_9;

            case SR_UTILS_NS::KeyCode::A: return ImGuiKey_A;
            case SR_UTILS_NS::KeyCode::B: return ImGuiKey_B;
            case SR_UTILS_NS::KeyCode::C: return ImGuiKey_C;
            case SR_UTILS_NS::KeyCode::D: return ImGuiKey_D;
            case SR_UTILS_NS::KeyCode::E: return ImGuiKey_E;
            case SR_UTILS_NS::KeyCode::F: return ImGuiKey_F;
            case SR_UTILS_NS::KeyCode::G: return ImGuiKey_G;
            case SR_UTILS_NS::KeyCode::H: return ImGuiKey_H;
            case SR_UTILS_NS::KeyCode::I: return ImGuiKey_I;
            case SR_UTILS_NS::KeyCode::J: return ImGuiKey_J;
            case SR_UTILS_NS::KeyCode::K: return ImGuiKey_K;
            case SR_UTILS_NS::KeyCode::L: return ImGuiKey_L;
            case SR_UTILS_NS::KeyCode::M: return ImGuiKey_M;
            case SR_UTILS_NS::KeyCode::N: return ImGuiKey_N;
            case SR_UTILS_NS::KeyCode::O: return ImGuiKey_O;
            case SR_UTILS_NS::KeyCode::P: return ImGuiKey_P;
            case SR_UTILS_NS::KeyCode::Q: return ImGuiKey_Q;
            case SR_UTILS_NS::KeyCode::R: return ImGuiKey_R;
            case SR_UTILS_NS::KeyCode::S: return ImGuiKey_S;
            case SR_UTILS_NS::KeyCode::T: return ImGuiKey_T;
            case SR_UTILS_NS::KeyCode::U: return ImGuiKey_U;
            case SR_UTILS_NS::KeyCode::V: return ImGuiKey_V;
            case SR_UTILS_NS::KeyCode::W: return ImGuiKey_W;
            case SR_UTILS_NS::KeyCode::X: return ImGuiKey_X;
            case SR_UTILS_NS::KeyCode::Y: return ImGuiKey_Y;
            case SR_UTILS_NS::KeyCode::Z: return ImGuiKey_Z;

            case SR_UTILS_NS::KeyCode::Super: return ImGuiKey_LeftSuper;

            case SR_UTILS_NS::KeyCode::F1:  return ImGuiKey_F1;
            case SR_UTILS_NS::KeyCode::F2:  return ImGuiKey_F2;
            case SR_UTILS_NS::KeyCode::F3:  return ImGuiKey_F3;
            case SR_UTILS_NS::KeyCode::F4:  return ImGuiKey_F4;
            case SR_UTILS_NS::KeyCode::F5:  return ImGuiKey_F5;
            case SR_UTILS_NS::KeyCode::F6:  return ImGuiKey_F6;
            case SR_UTILS_NS::KeyCode::F7:  return ImGuiKey_F7;
            case SR_UTILS_NS::KeyCode::F8:  return ImGuiKey_F8;
            case SR_UTILS_NS::KeyCode::F9:  return ImGuiKey_F9;
            case SR_UTILS_NS::KeyCode::F10: return ImGuiKey_F10;
            case SR_UTILS_NS::KeyCode::F11: return ImGuiKey_F11;
            case SR_UTILS_NS::KeyCode::F12: return ImGuiKey_F12;

            case SR_UTILS_NS::KeyCode::Plus:       return ImGuiKey_Equal;
            case SR_UTILS_NS::KeyCode::Minus:      return ImGuiKey_Minus;
            case SR_UTILS_NS::KeyCode::Dot:        return ImGuiKey_Period;
            case SR_UTILS_NS::KeyCode::Slash:      return ImGuiKey_Slash;
            case SR_UTILS_NS::KeyCode::BackSlash:  return ImGuiKey_Backslash;
            case SR_UTILS_NS::KeyCode::Tilde:      return ImGuiKey_GraveAccent;
            case SR_UTILS_NS::KeyCode::CapsLock:   return ImGuiKey_CapsLock;
            default: break;
        }
        return ImGuiKey_None;
    }

    void Replacement_Platform_CreateWindow(ImGuiViewport* vp)
    {
        if (ImGui_Platform_CreateWindow != nullptr) {
            ImGui_Platform_CreateWindow(vp);
        }

        if (vp->PlatformHandle != nullptr) {
        #ifdef SR_WIN32
            /// platform dependent manipulation of viewport window, f.e. in Win32:
            SetWindowLongPtr((HWND)vp->PlatformHandle, GWLP_WNDPROC, (LONG_PTR)CustomWindowProcPlatform);
        #endif
        }
    }

    const std::vector<VkDescriptorPoolSize> VulkanImGuiOverlay::POOL_SIZES = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 }
    };

    bool VulkanImGuiOverlay::Init() {
        SR_TRACY_ZONE;

        auto&& pKernel = m_pipeline.DynamicCast<VulkanPipeline>()->GetKernel();
        if (!pKernel->GetDevice() || !pKernel->GetDevice()->IsReady()) {
            SR_ERROR("VulkanImGuiOverlay::Init() : device is nullptr or not ready!");
            return false;
        }

        m_dynamicRendering = pKernel->GetDevice()->IsDynamicRenderingSupported();

        if (!Super::Init()) {
            return false;
        }

        SR_GRAPH_LOG("VulkanImGuiOverlay::Init() : initialization vulkan ImGui overlay...");

        m_inputTextSubscription = SR_UTILS_NS::Input::Instance().Subscribe(SR_UTILS_NS::INPUT_TEXT_EVENT_ID, [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
            if (m_inputTextEvents.size() > 64) {
                m_inputTextEvents.erase(m_inputTextEvents.begin());
            }
            m_inputTextEvents.push_back(std::any_cast<SR_UTILS_NS::InputTextEvent>(msg.GetAny(SR_UTILS_NS::INPUT_TEXT_EVENT_DATA_ID)));
        });

        [[maybe_unused]] auto&& pWindow = m_pipeline->GetWindow();

    #if defined(SR_WIN32)
        ImGui_ImplWin32_Init((HWND)pWindow->GetHandle());
    #elif defined(SR_LINUX) && defined(SR_RENDER_GLFW)
        ImGui_ImplGlfw_InitForVulkan(pWindow->GetImplementation<GLFWWindow>()->GetWindow(), true);
    #elif defined(SR_LINUX) && defined(SR_RENDER_USE_NATIVE_WAYLAND)
        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformName = "imgui_impl_wayland_custom";
    #elif defined(SR_ANDROID)
        ImGui_ImplAndroid_Init(pWindow->GetImplementation<AndroidWindow>()->GetNativeWindow());
    #endif

        m_pipeline->UpdateMultiSampling();

        m_tracyEnabled = SR_UTILS_NS::Features::Instance().Enabled("VulkanTracy", false);

        m_device = pKernel->GetDevice();
        m_swapChain = pKernel->GetSwapchain();
        m_multiSample = pKernel->GetMultisampleTarget();

        if (!m_device || !m_swapChain || !m_multiSample) {
            SR_ERROR("VulkanImGuiOverlay::Init() : device, multi sample or swapChain is nullptr!\n"
                "\tDevice: {}\n\tSwapChain: {}\n\tMulti sample: {}",
                m_device ? "Ok" : "Fail", m_swapChain ? "Ok" : "Fail", m_multiSample ? "Ok" : "Fail"
            );
            return false;
        }

        if (!m_pool) {
            m_pool = EvoVulkan::Types::DescriptorPool::Create(*m_device, 1000 * POOL_SIZES.size(), POOL_SIZES);
        }

        if (!m_pool) {
            SR_ERROR("VulkanImGuiOverlay::Init() : failed to create descriptor pool!");
            return false;
        }

        /// Create vulkan command buffers
        m_cmdBuffs.resize(m_swapChain->GetCountImages());

        for (auto&& cmdBuff : m_cmdBuffs) {
            if (auto&& pool = m_device->CreateCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); pool != VK_NULL_HANDLE) {
                m_cmdPools.emplace_back(pool);

                auto&& info = EvoVulkan::Tools::Initializers::CommandBufferAllocateInfo(pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
                if (vkAllocateCommandBuffers(*m_device, &info, &cmdBuff) != VK_SUCCESS) {
                    SR_ERROR("VulkanImGuiOverlay::Init() : failed to create command buffer!");
                    return false;
                }
            }
            else {
                SR_ERROR("VulkanImGuiOverlay::Init() : failed to create command pool!");
                return false;
            }
        }

        if (!InitializeRenderer()) {
            SR_ERROR("VulkanImGuiOverlay::Init() : failed to initialize renderer!");
            return false;
        }

        if (!ReCreate()) {
            SR_ERROR("VulkanImGuiOverlay::Init() : failed to re-create!");
            return false;
        }

        m_cmdBuffBI = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };

        m_initialized = true;

        return true;
    }

    void VulkanImGuiOverlay::Destroy() {
        DeInitializeRenderer();

        for (auto&& cmdPool : m_cmdPools) {
            if (cmdPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(*m_device, cmdPool, nullptr);
            }
        }

        SR_SAFE_DELETE_PTR(m_pool);

        m_cmdPools.clear();
        m_cmdBuffs.clear();

        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(*m_device, m_semaphore, nullptr);
            m_semaphore = VK_NULL_HANDLE;
        }

        if (m_initialized) {
        #if defined(SR_WIN32)
            ImGui_ImplWin32_Shutdown();
        #endif
        }

        m_initialized = false;

        ImGuiOverlay::Destroy();
    }

    bool VulkanImGuiOverlay::BeginDraw() {
        SR_TRACY_ZONE;

        if (!m_context) {
            return false;
        }

        if (m_undockingActive != IsUndockingActive()) {
            SR_LOG("VulkanImGuiOverlay::BeginDraw() : undocking active changed!");
            m_undockingActive = IsUndockingActive();
            m_surfaceDirty = true;
            return false;
        }

        ImGui_ImplVulkan_NewFrame();

    #ifdef SR_WIN32
        ImGui_ImplWin32_NewFrame();
    #elif defined(SR_LINUX) && defined(SR_RENDER_GLFW)
        ImGui_ImplGlfw_NewFrame();
    #elif defined(SR_LINUX) && defined(SR_RENDER_USE_NATIVE_WAYLAND)
        ImGuiIO& io = ImGui::GetIO();

        if (auto&& pNativeWindow = m_pipeline->GetWindow()->GetImplementation<WaylandWindow>()) {
            const float_t scale = pNativeWindow->GetScale();
            io.DisplayFramebufferScale = ImVec2(scale, scale);
            io.DisplaySize = ImVec2(pNativeWindow->GetSurfaceWidth() / scale, pNativeWindow->GetSurfaceHeight() / scale);
        }

        static const SR_UTILS_NS::StringAtom deltaTimeKey = "DeltaTime";
        constexpr float_t defaultDeltaTime = 1.0f / 60.0f;
        io.DeltaTime = SR_THIS_THREAD->GetContext()->GetValueDef<float_t>(deltaTimeKey, defaultDeltaTime);
        io.DeltaTime = io.DeltaTime == 0.0f ? defaultDeltaTime : io.DeltaTime;

    #elif defined(SR_ANDROID)
        ImGui_ImplAndroid_NewFrame();
    #endif

        ImGui::NewFrame();

    #if defined(SR_LINUX) && defined(SR_RENDER_USE_NATIVE_WAYLAND)
        ProcessInput();
    #endif

        return true;
    }

    void VulkanImGuiOverlay::EndDraw() {
        SR_TRACY_ZONE;

        if (m_undockingActive != IsUndockingActive()) {
            SR_LOG("VulkanImGuiOverlay::EndDraw() : undocking active changed!");
            m_undockingActive = IsUndockingActive();
            m_surfaceDirty = true;

            ImGui::Render();

            if (IsViewportsEnabled()) {
                ImGui::UpdatePlatformWindows();
            }

            return;
        }

        ImGui::Render();

        /// Update and Render additional Platform Windows
        if (IsViewportsEnabled()) {
            ImGui::UpdatePlatformWindows();

            const bool old = EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationMuteSmallMemoryAllocations;
            EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationMuteSmallMemoryAllocations = true;
            ImGui::RenderPlatformWindowsDefault();
            EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationMuteSmallMemoryAllocations = old;
        }
    }

    bool VulkanImGuiOverlay::InitializeRenderer() {
        SR_INFO("VulkanImGuiOverlay::InitializeRenderer() : initialization vulkan ImGui renderer...");

        const VkSampleCountFlagBits countMSAASamples = EvoVulkan::Tools::Convert::IntToSampleCount(1);

        if (m_dynamicRendering) {
            m_pVkCmdBeginRendering = (PFN_vkCmdBeginRendering)vkGetDeviceProcAddr(*m_device, "vkCmdBeginRendering");
            m_pVkCmdEndRendering   = (PFN_vkCmdEndRendering)vkGetDeviceProcAddr(*m_device, "vkCmdEndRendering");

            if (!m_pVkCmdBeginRendering) {
                m_pVkCmdBeginRendering = (PFN_vkCmdBeginRendering)vkGetDeviceProcAddr(*m_device, "vkCmdBeginRenderingKHR");
            }

            if (!m_pVkCmdEndRendering) {
                m_pVkCmdEndRendering = (PFN_vkCmdEndRendering)vkGetDeviceProcAddr(*m_device, "vkCmdEndRenderingKHR");
            }

            if (!m_pVkCmdBeginRendering || !m_pVkCmdEndRendering) {
                SR_ERROR("VulkanImGuiOverlay::InitializeRenderer() : failed to get dynamic rendering functions! Disabling dynamic rendering...");
                m_dynamicRendering = false;
            }
        }

        if (!m_dynamicRendering && IsViewportsEnabled()) {
            SRHalt("Undocking requires dynamic rendering support!");
            return false;
        }

        m_renderPass = EvoVulkan::Types::CreateRenderPass(
            m_device, m_swapChain,
            {
                EvoVulkan::Tools::CreateColorAttachmentDescription(
                    m_swapChain->GetColorFormat(),
                    countMSAASamples,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                )
            },
            { } /** input attachments */,
            countMSAASamples,
            EvoVulkan::Tools::Initializers::EVK_IMAGE_ASPECT_NONE /** depth buffer */,
            m_device->GetDepthFormat()
        );

        if (!m_renderPass.IsReady()) {
            SR_ERROR("VulkanImGuiOverlay::InitializeRenderer() : failed to create render pass!");
            return false;
        }

        auto&& pKernel = m_pipeline.DynamicCast<VulkanPipeline>()->GetKernel();

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

        ImGuiViewport* mainViewport = platform_io.Viewports.front();
        mainViewport->PlatformHandle = pKernel->GetSurface()->GetHandle();

        platform_io.Platform_CreateVkSurface = CreatePlatformSurface;

    #ifdef SR_WIN32
        ImGui_Platform_CreateWindow = ImGui_ImplWin32_CreateWindow;
    #elif defined(SR_LINUX) || defined(SR_ANDROID)
        //SRHalt("Not yet implemented!");
    #else
        SRHalt("Unsupported platform!");
    #endif

        platform_io.Platform_CreateWindow = Replacement_Platform_CreateWindow;

        /// Setup Platform/Renderer bindings
        uint32_t images = GetCountImages();

        ImGui_ImplVulkan_InitInfo init_info = {
            .Instance                    = pKernel->GetInstance(),
            .PhysicalDevice              = *pKernel->GetDevice(),
            .Device                      = *pKernel->GetDevice(),
            .QueueFamily                 = static_cast<uint32_t>(pKernel->GetDevice()->GetQueues()->GetGraphicsIndex()),
            .Queue                       = pKernel->GetDevice()->GetQueues()->GetGraphicsQueue(),
            .DescriptorPool              = *m_pool,
            .RenderPass                  = m_renderPass,
            .MinImageCount               = images,
            .ImageCount                  = images,
            .MSAASamples                 = countMSAASamples,
            .PipelineCache               = pKernel->GetPipelineCache(),
            .Subpass                     = 0,
            .UseDynamicRendering         = false,
            .PipelineRenderingCreateInfo = { },
            .Allocator                   = nullptr,
            .CheckVkResultFn             = CheckVulkanResult,
            .MinAllocationSize           = 0,
        };

        VkFormat swapchainFormat = pKernel->GetSwapchain()->GetColorFormat();

        if (m_dynamicRendering) {
            // init_info.RenderPass = VK_NULL_HANDLE;

            init_info.UseDynamicRendering = true;
            init_info.PipelineRenderingCreateInfo = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext                   = nullptr,
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &swapchainFormat,
                .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
            };
        }

        if (!ImGui_ImplVulkan_Init(&init_info)) {
            SR_ERROR("VulkanImGuiOverlay::InitializeRenderer() : failed to init vulkan imgui implementation!");
            return false;
        }

        return true;
    }

    void VulkanImGuiOverlay::DeInitializeRenderer() {
        SR_INFO("VulkanImGuiOverlay::DeInitializeRenderer() : de-initialization vulkan ImGui renderer...");

        auto&& pVulkanBackend = ImGui::GetCurrentContext() ? (ImGui_ImplVulkan_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;

        if (pVulkanBackend) {
            ImGui_ImplVulkan_Shutdown();
        }

        DestroyBuffers();

        if (m_renderPass.IsReady()) {
            DestroyRenderPass(m_device, &m_renderPass);
        }
        else {
            SR_ERROR("VulkanImGuiOverlay::DeInitializeRenderer() : render pass isn't ready!");
        }
    }

    void VulkanImGuiOverlay::DestroyBuffers() {
        for (auto&& pBuffer : m_frameBuffs) {
            vkDestroyFramebuffer(*m_device, pBuffer, nullptr);
        }
        m_frameBuffs.clear();
    }

    void VulkanImGuiOverlay::ProcessInput() {
        SR_TRACY_ZONE;

        auto&& io = ImGui::GetIO();

        const auto& mouseState = SR_PLATFORM_NS::GetMouseState();
        if (m_mouseState.position.x != mouseState.position.x || m_mouseState.position.y != mouseState.position.y) {
            io.AddMousePosEvent(mouseState.position.x, mouseState.position.y);
        }

        for (size_t i = 0; i < mouseState.COUNT; i++) {
            if (m_mouseState.GetButton(i) != mouseState.GetButton(i)) {
                io.AddMouseButtonEvent(i, mouseState.buttonStates[i]);
            }
        }

        const auto& keyboardState = SR_PLATFORM_NS::GetSystemKeyboardState();
        for (const SR_UTILS_NS::KeyCode keyCode : SR_UTILS_NS::KeyCodes) {
            if (m_keyboardState.Get(keyCode) != keyboardState.Get(keyCode)) {
                if (const ImGuiKey imguiKey = KeyCodeToImGuiKey(keyCode); imguiKey != ImGuiKey_None) {
                    io.AddKeyEvent(imguiKey, keyboardState.Get(keyCode));
                }
            }
        }

        for (auto& event : m_inputTextEvents) {
            ImGui::GetIO().AddInputCharactersUTF8(event.GetText().data());
        }
        m_inputTextEvents.clear();

        m_keyboardState = keyboardState;
        m_mouseState = mouseState;
    }

    bool VulkanImGuiOverlay::ReCreate() {
        SR_GRAPH_LOG("VulkanImGuiOverlay::ReCreate() : re-creating vulkan ImGui overlay...");

        DestroyBuffers();

        if (!m_device || !m_swapChain) {
            SR_ERROR("VkImGUI::ReCreate() : device or swapChain is nullptr!");
            return false;
        }

        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(*m_device, m_semaphore, nullptr);
            m_semaphore = VK_NULL_HANDLE;
        }

        auto&& semaphoreCI = EvoVulkan::Tools::Initializers::SemaphoreCreateInfo();
        if (vkCreateSemaphore(*m_device, &semaphoreCI, nullptr, &m_semaphore) != VK_SUCCESS) {
            VK_ERROR("VkImGUI::Init() : failed to create vulkan semaphore!");
            return false;
        }

        auto&& surfaceSize = SR_MATH_NS::UVector2(m_swapChain->GetSurfaceWidth(), m_swapChain->GetSurfaceHeight());

        m_frameBuffs.resize(GetCountImages());

        auto&& fbInfo = EvoVulkan::Tools::Initializers::FrameBufferCI(m_renderPass, surfaceSize.x, surfaceSize.y);
        auto&& attaches = std::vector<VkImageView>(1); /// тут так и должно быть, не относится к буферизации кадров
        fbInfo.attachmentCount = attaches.size();

        for (uint32_t i = 0; i < m_frameBuffs.size(); i++) {
            attaches[0] = m_swapChain->GetBuffers()[i].m_view;

            fbInfo.pAttachments = attaches.data();
            if (vkCreateFramebuffer(*m_device, &fbInfo, nullptr, &m_frameBuffs[i]) != VK_SUCCESS) {
                SR_ERROR("VkImGUI::ReCreate() : failed to create frame buffer!");
                return false;
            }
        }

        m_clearValues = { { .color = { {0.0, 0.0, 0.0, 1.0} } } };

        m_renderPassBI = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = m_renderPass,
            .framebuffer = VK_NULL_HANDLE,
            .renderArea = { VkOffset2D(), { surfaceSize.x, surfaceSize.y } },
            .clearValueCount = static_cast<uint32_t>(m_clearValues.size()),
            .pClearValues = m_clearValues.data(),
        };

        m_surfaceDirty = false;

        return true;
    }

    EvoVulkan::SubmitInfo& VulkanImGuiOverlay::Render(uint32_t frame) {
        SR_TRACY_ZONE_S("VulkanImGuiOverlay::Render");

        if (frame >= m_cmdBuffs.size()) {
            SR_ERROR("VulkanImGuiOverlay::Render() : out of range!");
            return m_submitInfo;
        }

        m_submitInfo.commandBuffers.clear();
        m_submitInfo.commandBuffers.emplace_back(m_cmdBuffs[frame]);

        auto&& buffer = m_cmdBuffs[frame];

        {
            SR_TRACY_ZONE_S("VkResetCommandPool");
            vkResetCommandPool(*m_device, m_cmdPools[frame], 0);
        }

        static bool hasWarn = false;

        auto&& surfaceSize = SR_MATH_NS::UVector2(m_swapChain->GetSurfaceWidth(), m_swapChain->GetSurfaceHeight());

        VkRenderingAttachmentInfoKHR colorAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView   = m_swapChain->GetBuffers()[frame].m_view, // view swapchain image/framebuffer image
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = { .color = {{0.f, 0.f, 0.f, 1.f}} }
        };

        VkRenderingInfoKHR renderingInfo{
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea           = { {0, 0}, {surfaceSize.x, surfaceSize.y} },
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &colorAttachmentInfo
        };

        if (m_tracyEnabled) {
            vkBeginCommandBuffer(buffer, &m_cmdBuffBI);
            {
                SR_TRACY_VK_FRAME_ZONE_N(buffer, "VkImGUI");

                m_renderPassBI.framebuffer = m_frameBuffs[frame];
                vkCmdBeginRenderPass(m_cmdBuffs[frame], &m_renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

                if (auto&& drawData = ImGui::GetDrawData()) {
                    SR_TRACY_ZONE_S("ImGui_ImplVulkan_RenderDrawData");
                    ImGui_ImplVulkan_RenderDrawData(drawData, m_cmdBuffs[frame]);
                }
                else if (!hasWarn) {
                    hasWarn = true;
                    VK_WARN("VkImGUI::Render() : imgui draw data is nullptr!");
                }

                vkCmdEndRenderPass(m_cmdBuffs[frame]);
            }
            SR_TRACY_VK_COLLECT(buffer);
        }
        else {
            vkBeginCommandBuffer(buffer, &m_cmdBuffBI);
            {
                if (m_dynamicRendering) {
                    // Transition swapchain image from PRESENT_SRC_KHR to COLOR_ATTACHMENT_OPTIMAL
                    // Use COLOR_ATTACHMENT_OUTPUT_BIT as srcStageMask to synchronize with vkAcquireNextImageKHR
                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    barrier.image = m_swapChain->GetBuffers()[frame].m_image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.layerCount = 1;
                    vkCmdPipelineBarrier(
                        m_cmdBuffs[frame],
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier
                    );

                    m_pVkCmdBeginRendering(m_cmdBuffs[frame], &renderingInfo);
                }
                else {
                    m_renderPassBI.framebuffer = m_frameBuffs[frame];
                    vkCmdBeginRenderPass(m_cmdBuffs[frame], &m_renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
                }

                if (auto&& drawData = ImGui::GetDrawData()) {
                    SR_TRACY_ZONE_S("ImGui_ImplVulkan_RenderDrawData");
                    const bool old = EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationMuteSmallMemoryAllocations;
                    EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationMuteSmallMemoryAllocations = true;
                    ImGui_ImplVulkan_RenderDrawData(drawData, m_cmdBuffs[frame]);
                    EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationMuteSmallMemoryAllocations = old;
                }
                else if (!hasWarn) {
                    hasWarn = true;
                    VK_WARN("VkImGUI::Render() : imgui draw data is nullptr!");
                }

                if (m_dynamicRendering) {
                    m_pVkCmdEndRendering(m_cmdBuffs[frame]);

                    // Transition swapchain image from COLOR_ATTACHMENT_OPTIMAL back to PRESENT_SRC_KHR
                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = 0;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    barrier.image = m_swapChain->GetBuffers()[frame].m_image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.layerCount = 1;
                    vkCmdPipelineBarrier(
                        m_cmdBuffs[frame],
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier
                    );
                }
                else {
                    vkCmdEndRenderPass(m_cmdBuffs[frame]);
                }
            }
        }

        vkEndCommandBuffer(m_cmdBuffs[frame]);

        return m_submitInfo;
    }

    void* VulkanImGuiOverlay::GetTextureDescriptorSet(uint32_t textureId) {
        if (textureId == SR_ID_INVALID) {
            SR_ERROR("VulkanImGuiOverlay::GetTextureDescriptorSet() : invalid id!");
            return nullptr;
        }

        if (!m_context) {
            SR_ERROR("VulkanImGuiOverlay::GetTextureDescriptorSet() : ImGui is not initialized!");
            return nullptr;
        }

        auto&& pMemoryManager = m_pipeline.DynamicCast<VulkanPipeline>()->GetMemoryManager();

        if (auto&& pTexture = pMemoryManager->GetTexture(textureId)) {
            auto&& layout = ((ImGui_ImplVulkan_Data*)ImGui::GetIO().BackendRendererUserData)->DescriptorSetLayout;
            return reinterpret_cast<void*>(pTexture->GetDescriptorSet(layout).descriptorSet);
        }

        return nullptr;
    }

    uint32_t VulkanImGuiOverlay::GetCountImages() const {
        return m_swapChain ? m_swapChain->GetCountImages() : 0;
    }

    void VulkanImGuiOverlay::ResetSubmitInfo() {
        auto&& pKernel = m_pipeline.DynamicCast<VulkanPipeline>()->GetKernel();
        if (!pKernel) {
            SR_ERROR("VulkanImGuiOverlay::ResetSubmitInfo() : kernel is nullptr!");
            return;
        }

        m_submitInfo = { };
        m_submitInfo.SetWaitDstStageMask(pKernel->GetSubmitPipelineStages());
        m_submitInfo.AddSignalSemaphore(m_semaphore);
    }

    void VulkanImGuiOverlay::ReloadFonts() {
        Super::ReloadFonts();

        if (ImGui::GetIO().BackendRendererUserData) {
            ImGui_ImplVulkan_CreateFontsTexture();
        }
    }
}
