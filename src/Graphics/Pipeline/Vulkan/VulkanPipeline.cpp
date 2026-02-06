//
// Created by Monika on 15.09.2023.
//


#include <Graphics/Render/RenderContext.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#include <Graphics/Pipeline/Vulkan/VulkanKernel.h>
#include <Graphics/Pipeline/Vulkan/AbstractCasts.h>
#include <Graphics/Pipeline/Vulkan/VulkanTracy.h>
#include <Graphics/Pipeline/Vulkan/VulkanMemory.h>

#include <EvoVulkan/Types/CmdBuffer.h>
#include <EvoVulkan/Types/VmaBuffer.h>
#include <EvoVulkan/Tools/VulkanTools.h>

#ifdef SR_USE_IMGUI
    #include <Graphics/Overlay/VulkanImGuiOverlay.h>
#endif

#ifdef SR_LINUX
    #include <Graphics/Window/GLFWWindow.h>
#endif

#ifdef SR_WIN32
    #include <Graphics/Window/Win32Window.h>
#endif

#ifdef SR_RENDER_USE_GLSL_LANG_LIB
    #include <Graphics/Pipeline/GLSLDefaultTBuiltInResource.h>
#endif

#ifdef SR_RENDER_USE_NATIVE_WAYLAND
    #include <Graphics/Window/WaylandWindow.h>
#endif

#ifdef SR_ANDROID
    #include <Graphics/Window/AndroidWindow.h>
#endif

#include <Utils/Events/EventManager.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/StoreUtils.h>

namespace SR_GRAPH_NS {
    namespace Details {
        VkRect2D ToVkRect2D(const SR_MATH_NS::IRect& rect) {
            return EvoVulkan::Tools::Initializers::Rect2D(
                static_cast<int32_t>(rect.position.x),
                static_cast<int32_t>(rect.position.y),
                static_cast<uint32_t>(rect.size.x),
                static_cast<uint32_t>(rect.size.y)
            );
        }

        bool CompareVkRect2D(const VkRect2D& a, const VkRect2D& b) {
            return a.offset.x == b.offset.x &&
                   a.offset.y == b.offset.y &&
                   a.extent.width == b.extent.width &&
                   a.extent.height == b.extent.height;
        }
    }

    std::string VulkanPipeline::GetVendor() const {
        if (m_kernel && m_kernel->GetDevice()) {
            return m_kernel->GetDevice()->GetName();
        }
        return "Invalid";
    }

    VulkanPipeline::~VulkanPipeline() {
        SR_SAFE_DELETE_PTR(m_kernel);
    }

    bool VulkanPipeline::InitOverlay() {
        SR_TRACY_ZONE;

    #ifdef SR_USE_IMGUI
        if (SR_UTILS_NS::Features::Instance().Enabled("ImGUI", false)) {
            auto&& pImGuiOverlay = m_overlays[OverlayType::ImGui];
            pImGuiOverlay = new VulkanImGuiOverlay(GetThis());
            if (!pImGuiOverlay->Init()) {
                PipelineError("VulkanPipeline::InitOverlay() : failed to initialize ImGui overlay!");
                return false;
            }
        }
    #endif

        return Pipeline::InitOverlay();
    }

    bool VulkanPipeline::Destroy() {
        SR_INFO("VulkanPipeline::Destroy() : destroying vulkan pipeline...");

    #ifdef SR_RENDER_USE_GLSL_LANG_LIB
        if (m_isGlslLangInit) {
            m_isGlslLangInit = false;
            glslang::FinalizeProcess();
        }
    #endif

        SR_TRACY_DESTROY(SR_UTILS_NS::TracyType::Vulkan);

        // Освобождаем все активные запросы пикселей
        if (m_kernel && m_kernel->GetDevice()) {
            for (auto&& [workId, request] : m_pixelRequests) {
                // Ждем завершения операции перед освобождением ресурсов
                if (!request.isReady && request.fence != VK_NULL_HANDLE) {
                    vkWaitForFences(*m_kernel->GetDevice(), 1, &request.fence, VK_TRUE, UINT64_MAX);
                }

                if (request.pBuffer) {
                    delete request.pBuffer;
                }
                if (request.pCmdBuffer) {
                    delete request.pCmdBuffer;
                }
                if (request.fence != VK_NULL_HANDLE) {
                    EvoVulkan::Tools::DestroyVulkanFence(*m_kernel->GetDevice(), &request.fence);
                }
            }
            m_pixelRequests.clear();
        }

        DestroyOverlay();

        if (m_memory) {
            m_memory->Free();
            m_memory = nullptr;
        }

        if (m_kernel) {
            m_kernel->Destroy();
        }

        EvoVulkan::Tools::VkFunctionsHolder::Instance().Reset();

        return Super::Destroy();
    }

    bool VulkanPipeline::PreInit(const PipelinePreInitInfo& info) {
        SR_TRACY_ZONE;

    #ifdef SR_RENDER_USE_GLSL_LANG_LIB
        if (!m_isGlslLangInit) {
            m_isGlslLangInit = true;
            glslang::InitializeProcess();
        }
    #endif

        if (!Pipeline::PreInit(info)) {
            PipelineError("VulkanPipeline::PreInit() : failed to pre-initialize pipeline!");
            return false;
        }

        if (!InitEvoVulkanHooks()) {
            PipelineError("VulkanPipeline::PreInit() : failed to initialize evo vulkan hooks!");
            return false;
        }

        m_enableValidationDebug = SR_UTILS_NS::Features::Instance().Enabled("VulkanValidation", false);
        m_enableGPUAssist = SR_UTILS_NS::Features::Instance().Enabled("VulkanGPUAssist", false);

    #ifdef SR_ANDROID
        m_enableGPUAssist = false;
        m_enableValidationLayers = false;
    #else
        m_enableValidationLayers = m_enableValidationDebug;
    #endif

        m_kernel = new SR_GRAPH_NS::VulkanKernel(GetThis());

        if (m_enableValidationLayers) {
            m_kernel->SetValidationLayersEnabled(true);
        }

        if (m_enableValidationDebug) {
            m_kernel->SetValidationDebugEnabled(true);
        }

        if (m_enableGPUAssist) {
            m_kernel->SetGPUAssistEnabled(true);
        }

        m_viewport = EvoVulkan::Tools::Initializers::Viewport(1, 1, 0, 0);
        m_scissor = EvoVulkan::Tools::Initializers::Rect2D(0, 0, 0, 0);
        m_cmdBufInfo = EvoVulkan::Tools::Initializers::CommandBufferBeginInfo();
        m_renderPassBI = EvoVulkan::Tools::Insert::RenderPassBeginInfo(0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr, 0);

        m_kernel->SetMultisampling(m_requiredSampleCount);
        m_kernel->SetSwapchainImagesCount(SR_UTILS_NS::StoreUtils::User::GetInt("SwapchainImages", 3));

        std::vector<const char*> validationLayers = { };
        std::vector<const char*> instanceExtensions = { };
#ifndef SR_LINUX
        instanceExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            /// VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME,
        #ifdef SR_WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #endif
        #ifdef SR_ANDROID
            #ifdef VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
                VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
            #else
                #error VK_KHR_ANDROID_SURFACE_EXTENSION_NAME is not available!
            #endif
        #endif
        };
#elif defined(SR_RENDER_GLFW)
        uint32_t count = 0;
        auto&& glfwInstanceExtensions = glfwGetRequiredInstanceExtensions(&count);

        if (!glfwInstanceExtensions) {
            PipelineError("VulkanPipeline::PreInit() : failed to get required instance extensions!");
            return false;
        }

        for (uint32_t i = 0; i < count; ++i) {
            instanceExtensions.emplace_back(glfwInstanceExtensions[i]);
        }
#elif defined(SR_RENDER_USE_NATIVE_WAYLAND)
        instanceExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        instanceExtensions.emplace_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif //SR_LINUX

        if (m_enableValidationLayers) {
            instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            if (m_enableGPUAssist) {
                instanceExtensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
            }
            validationLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }

        if (!m_kernel->PreInit(info.appName, info.engineName, info.GLSLCompilerPath.ToStringRef(),
            instanceExtensions,
            validationLayers
        )) {
            PipelineError("VulkanPipeline::PreInit() : failed to pre-init Evo Vulkan kernel!");
            return false;
        }

    #ifdef SR_TRACY_ENABLE
        SR_UTILS_NS::TracyContextManager::Instance().VulkanDestroy = [](void* pContext) {
            TracyVkDestroy((tracy::VkCtx*)pContext);
        };
    #endif

        return true;
    }

    bool VulkanPipeline::Init() {
        SR_TRACY_ZONE;

        SR_GRAPH_LOG("VulkanPipeline::Init() : initializing vulkan...");

        auto&& createSurfaceFn = [this](const VkInstance &instance) -> VkSurfaceKHR {
    #ifdef VK_USE_PLATFORM_WIN32_KHR
            if (auto&& pImpl = m_window->GetImplementation<Win32Window>()) {
                VkWin32SurfaceCreateInfoKHR surfaceInfo = { };
                surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.pNext = nullptr;
                surfaceInfo.flags = 0;
                surfaceInfo.hinstance = pImpl->GetHINSTANCE();
                surfaceInfo.hwnd = pImpl->GetHWND();

                VkSurfaceKHR surface = VK_NULL_HANDLE;
                VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &surface);
                if (result != VK_SUCCESS) {
                    return VK_NULL_HANDLE;
                }
                else
                    return surface;
            }
            else {
                PipelineError("VulkanPipeline::Init() : window is not support this architecture!");
                return VK_NULL_HANDLE;
            }
    #elif defined(SR_ANDROID)
            if (auto&& pImpl = m_window->GetImplementation<AndroidWindow>()) {
                VkAndroidSurfaceCreateInfoKHR surfaceInfo = { };
                surfaceInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.pNext = nullptr;
                surfaceInfo.flags = 0;
                surfaceInfo.window = pImpl->GetNativeWindow();

                VkSurfaceKHR surface = VK_NULL_HANDLE;
                VkResult result = vkCreateAndroidSurfaceKHR(instance, &surfaceInfo, nullptr, &surface);
                if (result != VK_SUCCESS) {
                    return VK_NULL_HANDLE;
                }
                else
                    return surface;
            }
            else {
                PipelineError("VulkanPipeline::Init() : window is not support this architecture!");
                return VK_NULL_HANDLE;
            }
    #elif defined(SR_LINUX)
        #ifdef SR_RENDER_GLFW
            if (auto&& pImpl = m_window->GetImplementation<GLFWWindow>()) {
                VkSurfaceKHR surface;
                VkResult error = glfwCreateWindowSurface(instance, pImpl->GetWindow(), nullptr, &surface);

                if (error != VK_SUCCESS) {
                    PipelineError("VulkanPipeline::Init() : GLFW window surface initialization failed!");
                    return VK_NULL_HANDLE;
                }

                return surface;
            }
            else {
                PipelineError("VulkanPipeline::Init() : failed to get window implementation!");
                return VK_NULL_HANDLE;
            }
        #elif defined(SR_RENDER_USE_NATIVE_WAYLAND)
            if (auto&& pImpl = m_window->GetImplementation<WaylandWindow>()) {
                VkWaylandSurfaceCreateInfoKHR surfaceInfo = { };
                surfaceInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.pNext = nullptr;
                surfaceInfo.flags = 0;
                surfaceInfo.display = pImpl->GetDisplay();
                surfaceInfo.surface = pImpl->GetSurface();

                VkSurfaceKHR surface = VK_NULL_HANDLE;
                VkResult result = vkCreateWaylandSurfaceKHR(instance, &surfaceInfo, nullptr, &surface);
                if (result != VK_SUCCESS) {
                    PipelineError("VulkanPipeline::Init() : failed to create wayland surface! Reason: " + EvoVulkan::Tools::Convert::result_to_description(result));
                    return VK_NULL_HANDLE;
                }
                return surface;
            }
            PipelineError("VulkanPipeline::Init() : window is not support this architecture!");
            return VK_NULL_HANDLE;
        #endif
    #endif
            SRHalt("Unsupported platform!");
            return VK_NULL_HANDLE;
        };

        if (m_window) {
            if (auto&& pImpl = m_window->GetImplementation<BasicWindowImpl>()) {
                m_kernel->SetSize(pImpl->GetSurfaceWidth(), pImpl->GetSurfaceHeight());
            }
        }

        std::vector<const char*> deviceExtensions = {
            //VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME
            //VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME,
        };

        const bool dynamicRenderingRequire = SR_UTILS_NS::Features::Instance().Enabled("VulkanDynamicRendering", false);

        if (!m_kernel->Init(createSurfaceFn, m_window ? m_window->GetHandle() : nullptr, deviceExtensions, true, dynamicRenderingRequire, m_preInitInfo.multisampling, m_preInitInfo.vsync)) {
            PipelineError("VulkanPipeline::Init() : failed to initialize Evo Vulkan kernel!");
            return false;
        }

        /// Под Wayland'ом нужно самостоятельно отслеживать изменение размеров окна
        m_kernel->SetAutoSwapChainResize(m_window && m_window->GetImplementation() && m_window->GetImplementation()->GetType() == BasicWindowImpl::WindowType::Wayland);

        SR_INFO("VulkanPipeline::Init() : creating vulkan memory manager...");
        m_memory = VulkanTools::MemoryManager::Create(m_kernel);
        if (!m_memory) {
            PipelineError("VulkanPipeline::Init() : failed to create vulkan memory manager!");
            return false;
        }

        m_supportedSampleCount = m_kernel->GetDevice()->GetMSAASamplesCount();

        return Super::Init();
    }

    UsedVideoMemoryInfo VulkanPipeline::GetUsedVideoMemoryInfo() const {
        SR_TRACY_ZONE;

        UsedVideoMemoryInfo info = { };

        info.videoMemoryUsed = m_kernel->GetAllocator() ? m_kernel->GetAllocator()->GetAllocatedMemorySize() : 0;
        info.videoMemoryHeaps = m_kernel->GetAllocator() ? m_kernel->GetAllocator()->GetAllocatedHeapsCount() : 0;
        info.shaderProgramsCount = m_memory ? m_memory->GetShaderProgramsCount() : 0;
        info.descriptorSetsCount = m_memory ? m_memory->GetDescriptorSetsCount() : 0;
        info.UBOsCount = m_memory ? m_memory->GetUBOsCount() : 0;
        info.VBOsCount = m_memory ? m_memory->GetVBOsCount() : 0;
        info.IBOsCount = m_memory ? m_memory->GetIBOsCount() : 0;
        info.SSBOsCount = m_memory ? m_memory->GetSSBOsCount() : 0;
        info.FBOsCount = m_memory ? m_memory->GetFBOsCount() : 0;
        info.texturesCount = m_memory ? m_memory->GetTexturesCount() : 0;

        return info;
    }

    int32_t VulkanPipeline::AllocateUBO(uint32_t uboSize) {
        if (!m_isComputeState && !m_isRenderState) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::AllocateUBO() : render or compute state isn't active!");
            SRHaltOnce0();
            return SR_ID_INVALID;
        }

        if (!m_memory) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("VulkanPipeline::AllocateUBO() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += uboSize;

        SRAssert2(uboSize > 0, "Incorrect UBO size!");

        if (auto&& id = m_memory->AllocateUBO(uboSize); id >= 0) {
            return id;
        }

        PipelineError("VulkanPipeline::AllocateUBO() : failed to allocate uniform buffer object!");
        return SR_ID_INVALID;
    }

    int32_t VulkanPipeline::AllocDescriptorSet(const std::vector<DescriptorType>& types) {
        SR_TRACY_ZONE;

        if (!m_isComputeState && !m_isRenderState) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::AllocDescriptorSet() : render state isn't active or isn't in first build iteration!");
            SRHaltOnce0();
            return SR_ID_INVALID;
        }

        if (!m_memory) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("VulkanPipeline::AllocDescriptorSet() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;

        auto&& vkTypes = VulkanTools::ReferenceCastAbsDescriptorTypeToVk(types);

        if (m_state.shaderId < 0) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::AllocDescriptorSet() : shader program is not set!");
            SRHaltOnce0();
            return SR_ID_INVALID;
        }

        if (auto&& id = m_memory->AllocateDescriptorSet(m_state.shaderId, vkTypes); id >= 0) {
            return id;
        }

        PipelineError("VulkanPipeline::AllocDescriptorSet() : failed to allocate descriptor set!");
        return SR_ID_INVALID;
    }

    void* VulkanPipeline::GetCurrentFBOHandle() const {
        if (m_state.pFrameBuffer) SR_LIKELY_ATTRIBUTE {
            auto&& FBO = m_state.pFrameBuffer->GetId();

            if (FBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                PipelineError("Vulkan::GetCurrentFBOHandle() : invalid FBO!");
                return nullptr;
            }

            auto&& framebuffer = m_memory->GetFBO(FBO - 1);

            if (auto&& layers = framebuffer->GetLayers(); !layers.empty()) SR_LIKELY_ATTRIBUTE {
                return (void*)layers[SR_MIN(layers.size() - 1, m_state.frameBufferLayer)]->GetRenderPass();
            }

            PipelineError("Vulkan::GetCurrentFBOHandle() : frame buffer has no layers!");
            return nullptr;
        }

        return (void*)(VkRenderPass)m_kernel->GetRenderPass(); /// Ну типо кадровый буфер
    }

    void VulkanPipeline::GetFBOHandles(std::vector<void*>& handles) const {
        SR_TRACY_ZONE;

        handles.reserve(m_memory->GetFBOsCount() + 1);
        handles.clear();

        if (void* pHandle = (void*)(VkRenderPass)m_kernel->GetRenderPass()) {
            handles.emplace_back(pHandle);
        }

        m_memory->ForEachFBO([&handles](int32_t index, auto&& pFBO) {
            handles.emplace_back((void*)pFBO->GetRenderPass());
        });

        {
            SR_TRACY_ZONE_N("Sort");
            std::sort(handles.begin(), handles.end());
        }
    }

    void VulkanPipeline::GetShaderHandles(std::vector<void*>& handles) const {
        SR_TRACY_ZONE;

        handles.reserve(m_memory->GetShaderProgramsCount() + 1);
        handles.clear();

        m_memory->ForEachShader([&handles](int32_t index, auto&& pShader) {
            handles.emplace_back((void*)pShader->GetPipeline());
        });

        {
            SR_TRACY_ZONE_N("Sort");
            std::sort(handles.begin(), handles.end());
        }
    }

    void VulkanPipeline::UseShader(uint32_t shaderProgram) {
        Pipeline::UseShader(shaderProgram);

        m_currentDescriptorSet = VK_NULL_HANDLE;

        m_currentVkShader = m_memory->GetShaderProgram(shaderProgram);
        m_currentLayout = m_currentVkShader->GetPipelineLayout();

        if (m_currentVkShader == m_lastVkShader) {
            m_isShaderChanged = false;
            return;
        }

        m_currentVkShader->Bind(m_currentCmd);

        m_lastVkShader = m_currentVkShader;
        m_isShaderChanged = true;
    }

    int32_t VulkanPipeline::AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) {
        if (!m_memory) {
            SR_ERROR("VulkanPipeline::AllocateShaderProgram() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.allocations;

        if (fbo < 0 && createInfo.shaderType != SR_SRSL_NS::ShaderType::Compute) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("VulkanPipeline::AllocateShaderProgram() : vulkan requires valid FBO for shaders!");
            return SR_ID_INVALID;
        }

        if (!createInfo.Validate()) {
            PipelineError("VulkanPipeline::AllocateShaderProgram() : failed to validate shader create info! Create info:"
                 "\n\tPolygon mode: " + SR_UTILS_NS::EnumReflector::ToStringAtom(createInfo.polygonMode).ToStringRef() +
                          "\n\tCull mode: " + SR_UTILS_NS::EnumReflector::ToStringAtom(createInfo.cullMode).ToStringRef() +
                          "\n\tDepth compare: " + SR_UTILS_NS::EnumReflector::ToStringAtom(createInfo.depthCompare).ToStringRef() +
                          "\n\tPrimitive topology: " + SR_UTILS_NS::EnumReflector::ToStringAtom(createInfo.primitiveTopology).ToStringRef()
            );
            return SR_ID_INVALID;
        }

        EvoVulkan::Types::RenderPass renderPass = m_kernel->GetRenderPass();
        if (fbo > 0) {
            renderPass = m_memory->GetFBO(fbo - 1)->GetRenderPass();
        }

        if (!renderPass.IsReady()) {
            PipelineError("VulkanPipeline::CompileShader() : internal Evo Vulkan error! Render pass isn't ready!");
            return SR_ID_INVALID;
        }

        ShaderProgram shaderProgram = m_memory->AllocateShaderProgram(renderPass);
        if (shaderProgram < 0) {
            PipelineError("VulkanPipeline::CompileShader() : failed to allocate shader program ID!");
            return SR_ID_INVALID;
        }

        auto&& pShaderProgram = m_memory->GetShaderProgram(shaderProgram);

        std::vector<SourceShader> modules = { };

        for (auto&& [shaderStage, stage] : createInfo.stages) {
            SourceShader module(stage.path.ToString(), shaderStage);
            modules.emplace_back(module);
        }

        if (modules.empty()) {
            SRHalt("No shader modules were found!");
            return SR_ID_INVALID;
        }

        auto&& pushConstants = VulkanTools::AbstractPushConstantToVkPushConstants(createInfo);

        auto&& descriptorLayoutBindings = VulkanTools::UniformsToDescriptorLayoutBindings(createInfo.uniforms);
        if (!descriptorLayoutBindings.has_value()) {
            SRHalt("VulkanPipeline::AllocateShaderProgram() : failed to create descriptor layout bindings!");
            return SR_ID_INVALID;
        }

        std::vector<EvoVulkan::Complexes::SourceShader> vkModules;
        for (auto&& module : modules) {
            VkShaderStageFlagBits stage = VulkanTools::VkShaderShaderTypeToStage(module.m_stage);
            vkModules.emplace_back(EvoVulkan::Complexes::SourceShader(module.m_path, stage)); /// NOLINT
        }

        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

        {
            SR_TRACY_ZONE_N("Load Evo Vulkan shader");

            if (!pShaderProgram->Load(
                SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders"),
                vkModules,
                descriptorLayoutBindings.value(),
                pushConstants
            )) {
                EVK_POP_LOG_LEVEL();
                FreeShader(&shaderProgram);
                PipelineError("VulkanPipeline::CompileShader() : failed to load Evo Vulkan shader!");
                return SR_ID_INVALID;
            }
        }

        EVK_POP_LOG_LEVEL();

        if (createInfo.shaderType == SR_SRSL_NS::ShaderType::Compute) {
            EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

            SR_TRACY_ZONE_N("Compile Evo Vulkan compute shader");

            if (!pShaderProgram->CompileCompute()) {
                EVK_POP_LOG_LEVEL();
                PipelineError("VulkanPipeline::LinkShader() : failed to compile Evo Vulkan compute shader!");
                FreeShader(&shaderProgram);
                return SR_ID_INVALID;
            }

            EVK_POP_LOG_LEVEL();
        }
        else {
            auto&& vkVertexDescriptions = VulkanTools::AbstractVertexDescriptionsToVk(createInfo.vertexDescriptions);
            auto&& vkVertexAttributes = VulkanTools::AbstractAttributesToVkAttributes(createInfo.vertexAttributes);
            if (vkVertexAttributes.size() != createInfo.vertexAttributes.size()) {
                PipelineError("VulkanPipeline::LinkShader() : vkVertexDescriptions size != vertexDescriptions size!");
                FreeShader(&shaderProgram);
                return SR_ID_INVALID;
            }

            if (!pShaderProgram->SetVertexDescriptions(vkVertexDescriptions, vkVertexAttributes)) {
                PipelineError("VulkanPipeline::LinkShader() : failed to set vertex descriptions!");
                FreeShader(&shaderProgram);
                return SR_ID_INVALID;
            }

            const CullMode cullMode = createInfo.cullMode;
            const uint8_t sampleCount = GetFrameBufferSampleCount();
            const VkSampleCountFlagBits vkSampleCount = EvoVulkan::Tools::Convert::IntToSampleCount(sampleCount);
            const bool depthEnabled = m_currentVkFrameBuffer ? m_currentVkFrameBuffer->IsDepthEnabled() : true; /// NOLINT

            EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

            {
                SR_TRACY_ZONE_N("Compile Evo Vulkan shader");

                if (!pShaderProgram->Compile(
                    VulkanTools::AbstractPolygonModeToVk(createInfo.polygonMode),
                    VulkanTools::AbstractCullModeToVk(cullMode),
                    VulkanTools::AbstractDepthOpToVk(createInfo.depthCompare),
                    createInfo.blendEnabled && depthEnabled,
                    createInfo.depthWrite,
                    createInfo.depthTest,
                    createInfo.alphaCoverage,
                    VulkanTools::AbstractPrimitiveTopologyToVk(createInfo.primitiveTopology),
                    vkSampleCount
                )) {
                    EVK_POP_LOG_LEVEL();
                    PipelineError("VulkanPipeline::LinkShader() : failed to compile Evo Vulkan shader!");
                    FreeShader(&shaderProgram);
                    return SR_ID_INVALID;
                }
            }

            EVK_POP_LOG_LEVEL();
        }

        return shaderProgram;
    }

    uint8_t VulkanPipeline::GetFrameBufferSampleCount() const {
        ++m_state.operations;

        if (m_state.pFrameBuffer) {
            return m_state.pFrameBuffer->GetSamplesCount();
        }

        return GetSamplesCount();
    }

    int32_t VulkanPipeline::AllocateTexture(const SRTextureCreateInfo& createInfo) {
        SR_TRACY_ZONE;

        if (!m_memory) {
            SR_ERROR("VulkanPipeline::AllocateTexture() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        SRTextureCreateInfo textureCreateInfo = createInfo;

        ++m_state.allocations;
        ++m_state.operations;

        auto vkFormat = VulkanTools::AbstractTextureFormatToVkFormat(textureCreateInfo.format);
        if (vkFormat == VK_FORMAT_MAX_ENUM) {
            PipelineError("VulkanPipeline::AllocateTexture() : unsupported format!");
            return SR_ID_INVALID;
        }

        if (textureCreateInfo.compression != TextureCompression::None) {
            vkFormat = VulkanTools::AbstractTextureCompToVkFormat(textureCreateInfo.compression, vkFormat);
            if (vkFormat == VK_FORMAT_MAX_ENUM) {
                PipelineError("VulkanPipeline::AllocateTexture() : unsupported format with compression!");
                return SR_ID_INVALID;
            }

            if (auto&& size = MakeGoodSizes(textureCreateInfo.width, textureCreateInfo.height); size != std::pair(textureCreateInfo.width, textureCreateInfo.height)) {
                textureCreateInfo.pData = ResizeToLess(textureCreateInfo.width, textureCreateInfo.height, size.first, size.second, textureCreateInfo.pData);
                textureCreateInfo.width = size.first;
                textureCreateInfo.height = size.second;
            }

            if (textureCreateInfo.pData == nullptr || textureCreateInfo.width == 0 || textureCreateInfo.height == 0) {
                PipelineError("VulkanPipeline::AllocateTexture() : failed to reconstruct image!");
                return SR_ID_INVALID;
            }

            SR_LOG("VulkanPipeline::CalculateTexture() : compress " + SR_UTILS_NS::ToString(textureCreateInfo.width * textureCreateInfo.height * 4 / 1024 / 1024) + "MB source image...");

            textureCreateInfo.pData = Graphics::Compress(textureCreateInfo.width, textureCreateInfo.height, textureCreateInfo.pData, textureCreateInfo.compression);
            if (textureCreateInfo.pData == nullptr) {
                PipelineError("VulkanPipeline::AllocateTexture() : failed to compress image!");
                return SR_ID_INVALID;
            }
        }

        const auto pixelSize = GetPixelSize(textureCreateInfo.format);
        if (pixelSize == 0) {
            PipelineError("VulkanPipeline::AllocateTexture() : unknown pixel size!");
            return SR_ID_INVALID;
        }

        m_state.allocatedMemory += pixelSize * textureCreateInfo.width * textureCreateInfo.height;

        const VkSamplerAddressMode addressMode = VulkanTools::AbstractAddressModeToVkAddressMode(textureCreateInfo.addressMode);
        if (addressMode == VK_SAMPLER_ADDRESS_MODE_MAX_ENUM) {
            PipelineError("VulkanPipeline::AllocateTexture() : invalid address mode!");
            return SR_ID_INVALID;
        }

        auto&& id = m_memory->AllocateTexture(
            textureCreateInfo.pData, textureCreateInfo.width, textureCreateInfo.height, vkFormat, addressMode,
            VulkanTools::AbstractTextureFilterToVkFilter(textureCreateInfo.filter),
            textureCreateInfo.compression, textureCreateInfo.mipLevels, textureCreateInfo.cpuUsage
        );

        if (textureCreateInfo.compression != TextureCompression::None) {
            SRFree(const_cast<uint8_t*>(textureCreateInfo.pData)); /// Free compressed data. Original data isn't will be free.
        }

        if (id < 0) {
            PipelineError("VulkanPipeline::AllocateTexture() : failed to allocate texture!");
            return SR_ID_INVALID;
        }

        return id;
    }

    void VulkanPipeline::UnUseShader() {
        Super::UnUseShader();
        m_currentVkShader = nullptr;
        m_currentLayout = VK_NULL_HANDLE;
    }

    void VulkanPipeline::UpdateDescriptorSets(uint32_t descriptorSet, const SRDescriptorUpdateInfos& updateInfo) {
        SR_TRACY_ZONE;

        Super::UpdateDescriptorSets(descriptorSet, updateInfo);

        if (!m_isComputeState && !m_isRenderState) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::UpdateDescriptorSets() : render state isn't active or not in first build iteration!");
            SRHaltOnce0();
            return;
        }

        auto&& vkDescriptorSet = m_memory->GetDescriptorSet(descriptorSet).descriptorSet;

        m_writeDescriptorSets.clear();

        for (auto&& info : updateInfo) {
            switch (info.descriptorType) {
                case DescriptorType::Storage: {
                    auto&& vkStorageBuffer = m_memory->GetSSBO(info.ubo)->GetDescriptorRef();

                    m_writeDescriptorSets.emplace_back(EvoVulkan::Tools::Initializers::WriteDescriptorSet(
                        vkDescriptorSet,
                        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        info.binding,
                        vkStorageBuffer
                    ));

                    break;
                }
                case DescriptorType::Uniform: {
                    auto&& vkUBODescriptor = m_memory->GetUBO(info.ubo)->GetDescriptorRef();

                    m_writeDescriptorSets.emplace_back(EvoVulkan::Tools::Initializers::WriteDescriptorSet(
                        vkDescriptorSet,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        info.binding,
                        vkUBODescriptor
                    ));

                    break;
                }
                default:
                    SRHalt("VulkanPipeline::UpdateDescriptorSets() : unknown type!");
                    return;
            }
        }

        if (m_writeDescriptorSets.empty()) {
            SRHalt("writeDescriptorSets is empty!");
            return;
        }

        vkUpdateDescriptorSets(*m_kernel->GetDevice(), m_writeDescriptorSets.size(), m_writeDescriptorSets.data(), 0, nullptr);
    }

    void VulkanPipeline::UpdateUBO(uint32_t UBO, void* pData, uint64_t size) {
        SR_TRACY_ZONE;
        SRAssert2(UBO != SR_ID_INVALID, "Invalid UBO ID!");
        Super::UpdateUBO(UBO, pData, size);
        m_memory->GetUBO(UBO)->CopyToDevice(pData, size, true);
    }

    void VulkanPipeline::UpdateSSBO(uint32_t SSBO, void *pData, uint64_t size) {
        SR_TRACY_ZONE;
        SRAssert2(SSBO != SR_ID_INVALID, "Invalid SSBO ID!");
        Super::UpdateSSBO(SSBO, pData, size);
        m_memory->GetSSBO(SSBO)->CopyToDevice(pData, size, true);
    }

    void VulkanPipeline::ReadSSBO(uint32_t SSBO, void* pData, uint64_t size) {
        SR_TRACY_ZONE;
        SRAssert2(SSBO != SR_ID_INVALID, "Invalid SSBO ID!");
        Super::ReadSSBO(SSBO, pData, size);
        m_memory->GetSSBO(SSBO)->CopyFromDevice(pData, size);
    }

    void VulkanPipeline::FlushSSBO(uint32_t SSBO, uint64_t offset, uint64_t size) {
        SR_TRACY_ZONE;
        SRAssert2(SSBO != SR_ID_INVALID, "Invalid SSBO ID!");
        Super::FlushSSBO(SSBO, offset, size);
        m_memory->GetSSBO(SSBO)->Flush(offset, size);
    }

    uint8_t VulkanPipeline::GetBuildIterationsCount() const noexcept {
        return m_kernel ? m_kernel->GetCountBuildIterations() : 0;
    }

    void VulkanPipeline::SetViewport(int32_t width, int32_t height) {
        Super::SetViewport(width, height);

        if (width > 0 && height > 0) {
            m_viewport = EvoVulkan::Tools::Initializers::Viewport(
                static_cast<float_t>(width),
                static_cast<float_t>(height),
                0.f, 1.f
            );
        }
        else {
            if (m_state.frameBufferId == 0) {
                m_viewport = m_kernel->GetViewport();
            }
            else if (m_state.pFrameBuffer && m_currentVkFrameBuffer) {
                m_viewport = m_currentVkFrameBuffer->GetViewport();
            }
            else {
                SRHalt("Unresolved situation!");
                return;
            }
        }

        vkCmdSetViewport(m_currentCmd, 0, 1, &m_viewport);
    }

    void VulkanPipeline::SetScissor(int32_t width, int32_t height) {
        Super::SetScissor(width, height);

        if (width > 0 && height > 0) {
            m_scissor = EvoVulkan::Tools::Initializers::Rect2D(0, 0, width, height);
        }
        else {
            if (m_state.frameBufferId == 0) {
                m_scissor = m_kernel->GetScissor();
            }
            else if (m_state.pFrameBuffer && m_currentVkFrameBuffer) {
                m_scissor = m_currentVkFrameBuffer->GetScissor();
            }
            else {
                SRHalt("Unresolved situation!");
                return;
            }
        }

        vkCmdSetScissor(m_currentCmd, 0, 1, &m_scissor);
        m_activeScissor = m_scissor;
    }

    void VulkanPipeline::BindFrameBuffer(Pipeline::FramebufferPtr pFBO) {
        Super::BindFrameBuffer(pFBO);

        const auto frameIndex = GetCurrentImageIndex();

        if (!pFBO) {
            if (m_kernel->m_frameBuffers.size() <= frameIndex) {
                SRHalt("VulkanPipeline::BindFrameBuffer() : frame buffer index out of range! Current build iteration: {}", frameIndex);
                return;
            }
            m_renderPassBI.framebuffer = m_kernel->m_frameBuffers[frameIndex];
            m_renderPassBI.renderPass  = m_kernel->GetRenderPass();
            m_renderPassBI.renderArea  = m_kernel->GetRenderArea();

            m_currentVkFrameBuffer = nullptr;
            m_state.frameBufferId = 0;
        }
        else {
            auto&& FBO = pFBO->GetId();
            if (FBO == UINT32_MAX) {
                PipelineError("VulkanPipeline::BindFrameBuffer() : frame buffer index equals UINT32_MAX! Something went wrong...");
                return;
            }

            auto&& pFrameBuffer = m_memory->GetFBO(FBO - 1);
            auto&& layers = pFrameBuffer->GetLayers();

            uint32_t layerIndex = SR_MIN(m_state.frameBufferLayer, layers.size() - 1);

            auto&& vkFrameBuffer = layers.at(layerIndex)->GetFramebuffer();

            if (m_fboQueue.Contains(pFBO, layerIndex)) {
                PipelineError("VulkanPipeline::BindFrameBuffer() : frame buffer (\"" + std::to_string(FBO) + "\") is already added to FBO queue!");
                SRHalt0();
                return;
            }

            if (!m_fboQueue.Contains(pFBO)) {
                m_fboQueue.AddFrameBuffer(pFBO, layerIndex);
            }

            m_renderPassBI.framebuffer = vkFrameBuffer;
            m_renderPassBI.renderPass  = pFrameBuffer->GetRenderPass();
            m_renderPassBI.renderArea  = pFrameBuffer->GetRenderPassArea();
            m_currentCmd               = pFrameBuffer->GetCommandBuffer(frameIndex);

            m_currentVkFrameBuffer = pFrameBuffer;
            m_state.frameBufferId = FBO;
        }
    }

    int32_t VulkanPipeline::AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) {
        SR_TRACY_ZONE;

        if (!m_memory) {
            SR_ERROR("VulkanPipeline::AllocateFrameBuffer() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        WaitRenderIdle();

        ++m_state.allocations;
        ++m_state.operations;

        const uint8_t maxFrames = createInfo.pFBO->size();
        for (uint8_t frame = 0; frame < maxFrames; ++frame) {
            std::vector<int32_t> colorBuffers;
            colorBuffers.reserve((*createInfo.colors).size());

            std::vector<VkFormat> formats;
            formats.reserve((*createInfo.colors).size());

            for (auto&& color : (*createInfo.colors)) {
                color.texture.resize(maxFrames, SR_ID_INVALID);
                colorBuffers.emplace_back(color.texture[frame]);
                formats.emplace_back(VulkanTools::AbstractTextureFormatToVkFormat(color.format));
            }

            if (createInfo.size.x == 0 || createInfo.size.y == 0) {
                PipelineError("VulkanPipeline::AllocateFrameBuffer() : width or height equals zero!");
                return false;
            }

            if ((*createInfo.pFBO)[frame] == 0) {
                PipelineError("VulkanPipeline::AllocateFrameBuffer() : zero frame buffer are default frame buffer!");
                return false;
            }

            EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

            VulkanTools::VulkanFrameBufferAllocInfo info = {
                .FBO = ((*createInfo.pFBO)[frame] - 1),
                .frame = frame,
                .maxFrames = maxFrames,
                .width = static_cast<uint32_t>(createInfo.size.x),
                .height = static_cast<uint32_t>(createInfo.size.y),
                .pDepth = createInfo.pDepth,
                .sampleCount = createInfo.sampleCount,
                .layersCount = createInfo.layersCount,
                .arrayLayersCount = createInfo.arrayLayersCount,
                .oldColorAttachments = colorBuffers,
                .inputColorAttachments = formats,
                .pOutputColorAttachments = &colorBuffers,
            };

            info.features.depthLoad = createInfo.features.depthLoad;
            info.features.colorLoad = createInfo.features.colorLoad;
            info.features.depthTransferSrc = createInfo.features.depthTransferSrc;
            info.features.colorTransferSrc = createInfo.features.colorTransferSrc;
            info.features.depthTransferDst = createInfo.features.depthTransferDst;
            info.features.colorTransferDst = createInfo.features.colorTransferDst;
            info.features.depthShaderRead = createInfo.features.depthShaderRead;
            info.features.colorShaderRead = createInfo.features.colorShaderRead;
            info.features.offscreen = createInfo.features.offscreen;

            if ((*createInfo.pFBO)[frame] > 0) {
                if (!m_memory->ReAllocateFBO(info)) {
                    PipelineError("VulkanPipeline::AllocateFrameBuffer() : failed to re-allocate frame buffer object!");
                }
                EVK_POP_LOG_LEVEL();
                goto success;
            }

            (*createInfo.pFBO)[frame] = m_memory->AllocateFBO(info) + 1;
            if ((*createInfo.pFBO)[frame] <= 0) {
                (*createInfo.pFBO)[frame] = SR_ID_INVALID;
                PipelineError("VulkanPipeline::AllocateFrameBuffer() : failed to allocate FBO!");
                EVK_POP_LOG_LEVEL();
                return false;
            }

            EVK_POP_LOG_LEVEL();

        success:
            for (uint32_t i = 0; i < static_cast<uint32_t>((*createInfo.colors).size()); ++i) {
                (*createInfo.colors)[i].texture[frame] = colorBuffers[i];
            }
        }

        return true;
    }

    SR_MATH_NS::FColor VulkanPipeline::GetPixelColor(uint32_t textureId, uint32_t x, uint32_t y) {
        SR_TRACY_ZONE;

        ++m_state.operations;

        auto&& pTexture = m_memory->GetTexture(textureId);
        auto&& pixel = pTexture->GetPixel(x, y, 0);
        return SR_MATH_NS::FColor(
            static_cast<SR_MATH_NS::Unit>(pixel.r),
            static_cast<SR_MATH_NS::Unit>(pixel.g),
            static_cast<SR_MATH_NS::Unit>(pixel.b),
            static_cast<SR_MATH_NS::Unit>(pixel.a)
        );
    }

    uint64_t VulkanPipeline::RequestPixelRange(uint64_t workId, uint32_t textureId, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;

        ++m_state.operations;

        auto&& pTexture = m_memory->GetTexture(textureId);
        if (!pTexture) {
            PipelineError("VulkanPipeline::RequestPixelRange() : invalid texture id!");
            return SR_ID_INVALID;
        }

        auto&& image = pTexture->GetImage();
        const VkImageLayout originalLayout = image.GetLayout();
        const VkFormat format = image.GetFormat();
        const uint8_t channels = EvoVulkan::Tools::GetPixelChannelsCount(format);
        const uint8_t pixelTypeSize = EvoVulkan::Tools::GetPixelTypeSize(format);

        if (!channels || !pixelTypeSize) {
            PipelineError("VulkanPipeline::RequestPixelRange() : unsupported format!");
            return SR_ID_INVALID;
        }

        if (!(image.GetInfo().usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
            PipelineError("VulkanPipeline::RequestPixelRange() : image does not support VK_IMAGE_USAGE_TRANSFER_SRC_BIT!");
            return SR_ID_INVALID;
        }

        if (x + width > image.GetInfo().width || y + height > image.GetInfo().height) {
            PipelineError("VulkanPipeline::RequestPixelRange() : incorrect pixel range!");
            return SR_ID_INVALID;
        }

        if (workId == SR_ID_INVALID) {
            workId = m_nextWorkId.fetch_add(1);
        }

        const uint64_t bufferSize = uint64_t(width) * height * channels * pixelTypeSize;

        PixelRangeRequest* pRequest = nullptr;

        auto it = m_pixelRequests.find(workId);
        if (it == m_pixelRequests.end()) {
            auto [iter, _] = m_pixelRequests.emplace(workId, PixelRangeRequest{});
            pRequest = &iter->second;
        }
        else {
            pRequest = &it->second;
            if (!pRequest->isReady) {
                PipelineError("VulkanPipeline::RequestPixelRange() : previous request is not ready yet!");
                return SR_ID_INVALID;
            }
        }

        /* ---------- buffer ---------- */

        if (!pRequest->pBuffer || pRequest->pBuffer->GetSize() < bufferSize) {
            delete pRequest->pBuffer;

            pRequest->pBuffer = EvoVulkan::Types::VmaBuffer::Create(
                m_kernel->GetAllocator(),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY,
                bufferSize
            );

            if (!pRequest->pBuffer) {
                PipelineError("VulkanPipeline::RequestPixelRange() : buffer alloc failed!");
                return SR_ID_INVALID;
            }
        }

        /* ---------- fence ---------- */

        if (!pRequest->fence) {
            pRequest->fence = EvoVulkan::Tools::CreateVulkanFence(*m_kernel->GetDevice(), 0);
            if (!pRequest->fence) {
                PipelineError("VulkanPipeline::RequestPixelRange() : fence create failed!");
                return SR_ID_INVALID;
            }
        }
        else {
            vkResetFences(*m_kernel->GetDevice(), 1, &pRequest->fence);
        }

        /* ---------- command buffer ---------- */

        if (!pRequest->pCmdBuffer) {
            pRequest->pCmdBuffer = EvoVulkan::Types::CmdBuffer::Create(
                m_kernel->GetDevice(),
                m_kernel->GetResettableCmdPool(),
                VK_COMMAND_BUFFER_LEVEL_PRIMARY
            );

            if (!pRequest->pCmdBuffer) {
                PipelineError("VulkanPipeline::RequestPixelRange() : cmd buffer create failed!");
                return SR_ID_INVALID;
            }
        }
        else {
            vkResetCommandBuffer(*pRequest->pCmdBuffer, 0);
        }

        auto&& cmd = pRequest->pCmdBuffer;

        if (!cmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
            PipelineError("VulkanPipeline::RequestPixelRange() : begin failed!");
            return SR_ID_INVALID;
        }

        image.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { int32_t(x), int32_t(y), 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyImageToBuffer(
            *cmd,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            *pRequest->pBuffer,
            1,
            &region
        );

        image.TransitionImageLayout(originalLayout, cmd);
        cmd->End(false);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = cmd->GetCmdRef();

        if (vkQueueSubmit(
            m_kernel->GetDevice()->GetQueues()->GetGraphicsQueue(),
            1,
            &submitInfo,
            pRequest->fence
        ) != VK_SUCCESS) {
            PipelineError("VulkanPipeline::RequestPixelRange() : submit failed!");
            return SR_ID_INVALID;
        }

        pRequest->textureId = textureId;
        pRequest->x = x;
        pRequest->y = y;
        pRequest->width = width;
        pRequest->height = height;
        pRequest->originalLayout = originalLayout;
        pRequest->isReady = false;

        return workId;
    }

    bool VulkanPipeline::IsPixelRangeReady(uint64_t workId) const {
        SR_TRACY_ZONE;

        auto it = m_pixelRequests.find(workId);
        if (it == m_pixelRequests.end()) {
            return false;
        }

        auto&& request = it->second;
        
        // Если уже помечен как готовый, возвращаем true
        if (request.isReady) {
            return true;
        }

        // Проверяем статус fence
        VkResult result = vkGetFenceStatus(*m_kernel->GetDevice(), request.fence);
        
        if (result == VK_SUCCESS) {
            // Fence сигнализирован - операция завершена
            request.isReady = true;
            return true;
        }
        else if (result == VK_NOT_READY) {
            // Fence еще не сигнализирован
            return false;
        }
        else {
            // Ошибка
            PipelineError("VulkanPipeline::IsPixelRangeReady() : failed to get fence status!");
            return false;
        }
    }

    bool VulkanPipeline::GetPixelRangeResult(uint64_t workId, SR_MATH_NS::FColor* pixels, uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;

        if (!pixels) {
            PipelineError("VulkanPipeline::GetPixelRangeResult() : pixels is nullptr!");
            return false;
        }

        auto it = m_pixelRequests.find(workId);
        if (it == m_pixelRequests.end()) {
            PipelineError("VulkanPipeline::GetPixelRangeResult() : invalid workId!");
            return false;
        }

        auto&& request = it->second;

        // Проверяем размеры
        if (request.width != width || request.height != height) {
            PipelineError("VulkanPipeline::GetPixelRangeResult() : size mismatch!");
            return false;
        }

        // Если еще не готов, проверяем еще раз
        if (!request.isReady) {
            VkResult result = vkGetFenceStatus(*m_kernel->GetDevice(), request.fence);
            if (result == VK_SUCCESS) {
                request.isReady = true;
            }
            else if (result == VK_NOT_READY) {
                return false;  // Еще не готов
            }
            else {
                PipelineError("VulkanPipeline::GetPixelRangeResult() : failed to get fence status!");
                return false;
            }
        }

        // Получаем информацию о формате текстуры
        auto&& pTexture = m_memory->GetTexture(request.textureId);
        if (!pTexture) {
            PipelineError("VulkanPipeline::GetPixelRangeResult() : texture not found!");
            return false;
        }

        const VkFormat format = pTexture->GetImage().GetFormat();
        const uint8_t channels = EvoVulkan::Tools::GetPixelChannelsCount(format);
        const uint8_t pixelTypeSize = EvoVulkan::Tools::GetPixelTypeSize(format);

        // Маппим буфер и копируем данные
        if (auto&& pData = request.pBuffer->MapData()) {
            const auto&& GetPixelValue = [pixelTypeSize](void* pData, uint8_t offset) -> int64_t {
                switch (pixelTypeSize) {
                    case 1: return ((uint8_t*)pData)[offset];
                    case 2: return ((uint16_t*)pData)[offset];
                    case 4: return ((uint32_t*)pData)[offset];
                    case 8: return ((uint64_t*)pData)[offset];
                    default: return 0;
                }
            };

            const uint32_t pixelStride = channels * pixelTypeSize;
            const uint32_t rowStride = width * pixelStride;

            for (uint32_t py = 0; py < height; ++py) {
                for (uint32_t px = 0; px < width; ++px) {
                    const uint32_t bufferOffset = py * rowStride + px * pixelStride;
                    uint8_t* pPixelData = static_cast<uint8_t*>(pData) + bufferOffset;

                    uint64_t r = 0, g = 0, b = 0, a = 255;
                    if (channels >= 1) { r = GetPixelValue(pPixelData, 0); }
                    if (channels >= 2) { g = GetPixelValue(pPixelData, 1); }
                    if (channels >= 3) { b = GetPixelValue(pPixelData, 2); }
                    if (channels >= 4) { a = GetPixelValue(pPixelData, 3); }

                    pixels[py * width + px] = SR_MATH_NS::FColor(
                        static_cast<SR_MATH_NS::Unit>(r),
                        static_cast<SR_MATH_NS::Unit>(g),
                        static_cast<SR_MATH_NS::Unit>(b),
                        static_cast<SR_MATH_NS::Unit>(a)
                    );
                }
            }

            request.pBuffer->Unmap();
        }
        else {
            PipelineError("VulkanPipeline::GetPixelRangeResult() : failed to map buffer!");
            return false;
        }

        return true;
    }

    void VulkanPipeline::ReleasePixelRangeRequest(uint64_t workId) {
        SR_TRACY_ZONE;

        auto it = m_pixelRequests.find(workId);
        if (it == m_pixelRequests.end()) {
            return;  // Уже удален или не существует
        }

        auto&& request = it->second;

        // Ждем завершения операции перед освобождением ресурсов
        if (!request.isReady) {
            vkWaitForFences(*m_kernel->GetDevice(), 1, &request.fence, VK_TRUE, UINT64_MAX);
        }

        // Освобождаем ресурсы
        if (request.pBuffer) {
            delete request.pBuffer;
        }
        if (request.pCmdBuffer) {
            delete request.pCmdBuffer;
        }
        if (request.fence != VK_NULL_HANDLE) {
            EvoVulkan::Tools::DestroyVulkanFence(*m_kernel->GetDevice(), &request.fence);
        }

        m_pixelRequests.erase(it);
    }

    void VulkanPipeline::CleanupCompletedRequests() {
        SR_TRACY_ZONE;

        auto it = m_pixelRequests.begin();
        while (it != m_pixelRequests.end()) {
            auto&& request = it->second;

            // Проверяем статус fence
            VkResult result = vkGetFenceStatus(*m_kernel->GetDevice(), request.fence);
            
            if (result == VK_SUCCESS && request.isReady) {
                // Операция завершена и данные уже получены - можно удалить
                if (request.pBuffer) {
                    delete request.pBuffer;
                }
                if (request.pCmdBuffer) {
                    delete request.pCmdBuffer;
                }
                if (request.fence != VK_NULL_HANDLE) {
                    EvoVulkan::Tools::DestroyVulkanFence(*m_kernel->GetDevice(), &request.fence);
                }
                it = m_pixelRequests.erase(it);
            }
            else {
                ++it;
            }
        }
    }

#ifdef SR_RENDER_USE_GLSL_LANG_LIB
    static EShLanguage GetShaderStageFromFileExtension(const std::string& filename) {
        if (filename.ends_with(".vert")) return EShLangVertex;
        if (filename.ends_with(".frag")) return EShLangFragment;
        if (filename.ends_with(".comp")) return EShLangCompute;
        if (filename.ends_with(".geom")) return EShLangGeometry;
        if (filename.ends_with(".tesc")) return EShLangTessControl;
        if (filename.ends_with(".tese")) return EShLangTessEvaluation;
        if (filename.ends_with(".mesh")) return EShLangMesh;
        if (filename.ends_with(".task")) return EShLangTask;
        if (filename.ends_with(".rgen")) return EShLangRayGen;
        if (filename.ends_with(".rint")) return EShLangIntersect;
        if (filename.ends_with(".rahit")) return EShLangAnyHit;
        if (filename.ends_with(".rchit")) return EShLangClosestHit;
        if (filename.ends_with(".rmiss")) return EShLangMiss;
        if (filename.ends_with(".rcall")) return EShLangCallable;
        SRHalt("Unknown shader file extension: " + filename);
        return EShLangCount;
    }
#endif

    bool VulkanPipeline::InitEvoVulkanHooks() {
        SR_TRACY_ZONE;
        SR_GRAPH("VulkanPipeline::InitEvoVulkanHooks() : initializing evo vulkan hooks...");

        auto&& GetUsedMemoryFn = [pPipeline = GetThis()]() -> uint32_t {
            return pPipeline ? pPipeline->GetUsedVideoMemoryInfo().videoMemoryUsed / 1024 / 1024 : 0;
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().ValidationErrorAsAssert = SR_UTILS_NS::Features::Instance().Enabled("VulkanValidationErrorAsAssert", false) &&
            SR_PLATFORM_NS::IsRunningUnderDebugger();

        EvoVulkan::Tools::VkFunctionsHolder::Instance().LogCallback = [GetUsedMemoryFn](const std::string& msg) {
            SR_VULKAN_LOG("[{} MB] {}", GetUsedMemoryFn(), msg);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().WarnCallback = [GetUsedMemoryFn](const std::string& msg) {
            SR_WARN("[{} MB] {}", GetUsedMemoryFn(), msg);
        };
        EvoVulkan::Tools::VkFunctionsHolder::Instance().ErrorCallback = [GetUsedMemoryFn](const std::string& msg) {
            SR_VULKAN_ERROR("[{} MB] {}", GetUsedMemoryFn(), msg);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().GraphCallback = [GetUsedMemoryFn](const std::string& msg) {
            SR_VULKAN_MSG("[{} MB] {}", GetUsedMemoryFn(), msg);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().AssertCallback = [GetUsedMemoryFn](const std::string &msg) {
            SRHalt("[{} MB] {}", GetUsedMemoryFn(), msg);
            return false;
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().CreateFolder = [](const std::string& path) -> bool {
            return SR_PLATFORM_NS::CreateFolder(path);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().IsSupportGLSLang = []() -> bool {
            #ifdef SR_RENDER_USE_GLSL_LANG_LIB
                return SR_UTILS_NS::Features::Instance().Enabled("UseGLSLangCompiler", true);
            #else
                return false;
            #endif
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().ReadSPIRV = [](const std::string& path) -> std::vector<uint32_t> {
            SR_TRACY_ZONE_N("Read SPIR-V");
            SR_TRACY_ZONE_TEXT(path);

            std::ifstream file(path, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                return {};
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            if (fileSize % sizeof(uint32_t) != 0) {
                // невалидный бинарь
                return {};
            }

            std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
            file.seekg(0);
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            return buffer;
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().WriteSPIRV = [](const std::string& path, const std::vector<uint32_t>& spirv) -> bool {
            SR_TRACY_ZONE_N("Write SPIR-V");
            SR_TRACY_ZONE_TEXT(path);

            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                return false;
            }

            file.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
            return true;
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().CompileGLSLtoSPIRV = [](const std::string& input) -> std::vector<uint32_t> {
            SR_TRACY_ZONE_N("Compile GLSL to SPIR-V");
            SR_TRACY_ZONE_TEXT(input);

            #ifdef SR_RENDER_USE_GLSL_LANG_LIB
                EShLanguage stage = GetShaderStageFromFileExtension(input);
                glslang::TShader shader(stage);
                glslang::TProgram program;

                // Compile the shader
                std::string shaderSourceStr = SR_UTILS_NS::FileSystem::ReadAllText(input);
                const char* shaderStrings = shaderSourceStr.c_str();
                shader.setStrings(&shaderStrings, 1);

                shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
                //shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
                //shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

                shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
                shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5); // SPIR-V 1.5 нормально для Vulkan 1.2+

                EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
                if (!shader.parse(&DefaultTBuiltInResource, 450, false, messages)) {
                    SR_ERROR("VulkanPipeline::CompileGLSLtoSPIRV() : failed to parse shader: {}\n{}", input, shader.getInfoLog());
                    return {};
                }

                program.addShader(&shader);
                if (!program.link(messages)) {
                    SR_ERROR("VulkanPipeline::CompileGLSLtoSPIRV() : failed to link shader program: {}\n{}", input, program.getInfoLog());
                    return {};
                }

                // Generate SPIR-V.
                std::vector<uint32_t> spirv;

                auto* intermediate = program.getIntermediate(stage);
                if (!intermediate) {
                    SR_ERROR("VulkanPipeline::CompileGLSLtoSPIRV() : failed to get intermediate representation for shader: {}", input);
                    return {};
                }
                glslang::GlslangToSpv(*intermediate, spirv);

                return spirv;
            #else
                SRHalt("GLSLang lib is not supported!");
                return std::vector<uint32_t>();
            #endif
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().ExecuteCommand = [](const std::string& command) {
            SR_TRACY_ZONE_N("Execute command");
            SR_TRACY_ZONE_TEXT(command);
            system(command.c_str());
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().Delete = [](const std::string& path) -> bool {
            return SR_PLATFORM_NS::Delete(path);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().IsExists = [](const std::string& path) -> bool {
            return SR_PLATFORM_NS::IsExists(path);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().Copy = [](const std::string& from, const std::string& to) -> bool {
            return SR_PLATFORM_NS::Copy(from, to);
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().ReadHash = [](const std::string& path) -> uint64_t {
            return SR_UTILS_NS::FileSystem::ReadHashFromFile(SR_UTILS_NS::Path(path));
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().GetFileHash = [](const std::string& path) -> uint64_t {
            return SR_UTILS_NS::FileSystem::GetFileHash(SR_UTILS_NS::Path(path));
        };

        EvoVulkan::Tools::VkFunctionsHolder::Instance().WriteHash = [](const std::string& path, uint64_t hash) -> bool {
            SR_UTILS_NS::FileSystem::WriteHashToFile(SR_UTILS_NS::Path(path), hash);
            return true;
        };

        return true;
    }

    bool VulkanPipeline::PostInit() {
        SR_TRACY_ZONE;

        SR_GRAPH_LOG("VulkanPipeline::PostInit() : post-initializing vulkan...");

        if (!m_kernel || !m_kernel->PostInit()) {
            SR_ERROR("VulkanPipeline::PostInit() : failed to post-initialize Evo Vulkan kernel!");
            return false;
        }

    #ifdef SR_TRACY_ENABLE
        if (SR_UTILS_NS::Features::Instance().Enabled("VulkanTracy", false)) {
            if (auto&& pSingleTimeCmd = m_kernel->CreateCmd()) {
                SR_GRAPH_LOG("VulkanPipeline::PostInit() : initializing tracy...");
                SR_TRACY_VK_CREATE(*pSingleTimeCmd, m_kernel, "EvoVulkan");
                delete pSingleTimeCmd;
            }
        }
    #endif

        return Super::PostInit();
    }

    void VulkanPipeline::WaitComputeIdle() {
        SR_TRACY_ZONE;
        if (m_kernel) {
            m_kernel->WaitComputeIdle();
        }
    }

    void VulkanPipeline::WaitDeviceIdle() {
        SR_TRACY_ZONE;
        if (m_kernel) {
            m_kernel->WaitDeviceIdle();
        }
    }

    void VulkanPipeline::OnFrameBuildBegin() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildBegin();

        /*if (!m_kernel->GetFrameSyncs().empty()) {
            SR_TRACY_ZONE_N("Wait in-flight fence");
            SR_TRACY_ZONE_COLOR(0xffa500ff);
            if (auto&& fence = m_kernel->GetInFlightFences()[GetCurrentImageIndex()]) {
                vkWaitForFences(*m_kernel->GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
            }
        }*/

        auto&& pFrameCmdPool = m_kernel->GetCurrentFrameCmdPool();
        vkResetCommandPool(*m_kernel->GetDevice(), *pFrameCmdPool, 0);
    }

    void VulkanPipeline::WaitRenderIdle() {
        SR_TRACY_ZONE;
        if (m_kernel) {
            m_kernel->WaitAllFences();
        }
    }

    bool VulkanPipeline::BeginCmdBuffer() {
        SR_TRACY_ZONE;

        const auto frameIndex = GetCurrentImageIndex();

        if (!m_isComputeState) {
            if (m_currentVkFrameBuffer) {
                m_currentCmd = m_currentVkFrameBuffer->GetCommandBuffer(frameIndex);
            }
            else {
                m_currentCmd = m_kernel->m_drawCmdBuffs[frameIndex];
            }
        }

        if (!m_currentCmd) {
            PipelineError("VulkanPipeline::BeginCmdBuffer() : cmd buffer is nullptr!");
            return false;
        }

        m_lastVkShader = nullptr;
        m_isShaderChanged = true;

        if (!m_isComputeState && !m_state.hasRenderData) {
            for (uint32_t i = 0; i < m_kernel->GetCountBuildIterations(); ++i) {
                if (m_kernel->m_drawCmdBuffs[i] == m_currentCmd) {
                    m_state.hasRenderData = true;
                    break;
                }
            }
        }

        vkBeginCommandBuffer(m_currentCmd, &m_cmdBufInfo);
        return Super::BeginCmdBuffer();
    }

    void VulkanPipeline::EndCmdBuffer() {
        SR_TRACY_ZONE;

        if (!m_currentCmd) {
            PipelineError("VulkanPipeline::EndCmdBuffer() : cmd buffer is nullptr!");
            return;
        }

        vkEndCommandBuffer(m_currentCmd);
        Super::EndCmdBuffer();
    }

    bool VulkanPipeline::BeginRender() {
        SR_TRACY_ZONE;

        if (!Super::BeginRender()) {
            return false;
        }

        if (!m_renderPassBI.pClearValues) {
            SRHaltOnce("pClearValues is nullptr! Please, call ClearBuffers before BeginRender");
            return false;
        }

        vkCmdBeginRenderPass(m_currentCmd, &m_renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        return true;
    }

    void VulkanPipeline::EndRender() {
        SR_TRACY_ZONE;

        Super::EndRender();

        if (!m_currentCmd) {
            PipelineError("VulkanPipeline::EndRender() : cmd buffer is nullptr!");
            return;
        }

        vkCmdEndRenderPass(m_currentCmd);
    }

    void VulkanPipeline::DrawFrame() {
        Super::DrawFrame();

        switch (m_kernel->NextFrame()) {
            case EvoVulkan::Core::RenderResult::Fatal:
                SR_UTILS_NS::EventManager::Instance().Broadcast(SR_UTILS_NS::EventManager::Event::FatalError);
                ++m_errorsCount;
                break;
            case EvoVulkan::Core::RenderResult::Error:
                PipelineError("VulkanPipeline::DrawFrame() : an error has been occurred!");
                break;
            case EvoVulkan::Core::RenderResult::DeviceLost:
                PipelineError("VulkanPipeline::DrawFrame() : device lost! Terminate...");
                SR_PLATFORM_NS::Terminate();
                break;
            default:
                break;
        }
    }

    void VulkanPipeline::ClearBuffers() {
        Super::ClearBuffers();

        if (m_state.frameBufferId < 0) {
            PipelineError("VulkanPipeline::ClearBuffers() : frame buffer is not attached!");
            return;
        }
        else if (m_state.frameBufferId > 0) {
            int32_t fbo = m_state.frameBufferId - 1;
            m_renderPassBI.clearValueCount = m_memory->GetFBO(fbo)->GetCountClearValues();
            m_renderPassBI.pClearValues = m_memory->GetFBO(fbo)->GetClearValues();
        }
        else {
            /// в какой ситуации это может случиться?
            SRHalt("VulkanPipeline::ClearBuffers() : TODO!");
        }
    }

    void VulkanPipeline::ClearBuffers(float_t r, float_t g, float_t b, float_t a, float_t depth, uint8_t colorCount) {
        Super::ClearBuffers(r, g, b, a, depth, colorCount);

        const uint8_t sampleCount = GetFrameBufferSampleCount();

        colorCount *= sampleCount > 1 ? 2 : 1;

        m_clearValues.resize(colorCount + 1);

        for (uint8_t i = 0; i < colorCount; ++i) {
            m_clearValues[i] = { .color = {{ r, g, b, a }} };
        }

        if (depth < 0.0f || depth > 1.0f) {
            SR_ERROR("VulkanPipeline::ClearBuffers() : depth value must be in range [0.0, 1.0]!");
            depth = std::clamp(depth, 0.0f, 1.0f);
        }

        m_clearValues[colorCount] = VkClearValue { .depthStencil = { depth, 0 } };

        m_renderPassBI.clearValueCount = colorCount + 1;
        m_renderPassBI.pClearValues    = m_clearValues.data();
    }

    void VulkanPipeline::ClearBuffers(const ClearColors& clearColors, std::optional<float_t> depth) {
        Super::ClearBuffers(clearColors, depth);

        const uint8_t sampleCount = GetFrameBufferSampleCount();

        auto colorCount = static_cast<uint8_t>(clearColors.size());
        colorCount *= sampleCount > 1 ? 2 : 1;

        const bool hasDepth = depth.has_value();

        m_clearValues.resize(colorCount + static_cast<uint8_t>(hasDepth));

        for (uint8_t i = 0; i < colorCount; ++i) {
            auto&& color = clearColors[i / (sampleCount > 1 ? 2 : 1)];

            m_clearValues[i] = {
                .color = { {
                       static_cast<float_t>(color.r),
                       static_cast<float_t>(color.g),
                       static_cast<float_t>(color.b),
                       static_cast<float_t>(color.a)
                   }
                }
            };
        }

        if (hasDepth) {
            m_clearValues[colorCount] = VkClearValue { .depthStencil = { depth.value(), 0 } };
        }

        m_renderPassBI.clearValueCount = colorCount + static_cast<uint8_t>(hasDepth);
        m_renderPassBI.pClearValues = m_clearValues.data();
    }

    void VulkanPipeline::OnResize(const SR_MATH_NS::UVector2& size) {
        if (m_kernel) {
            m_kernel->SetSize(size.x, size.y);
        }
        Super::OnResize(size);
    }

    bool VulkanPipeline::FreeTexture(int32_t *id) {
        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        if (!m_memory || !m_memory->FreeTexture(static_cast<uint32_t>(*id))) {
            SR_ERROR("VulkanPipeline::FreeTexture() : failed to free texture!");
            return false;
        }

        *id = SR_ID_INVALID;

        return true;
    }

    void VulkanPipeline::SetCurrentFrameBuffer(Pipeline::FramebufferPtr pFrameBuffer) {
        Super::SetCurrentFrameBuffer(pFrameBuffer);

        if (pFrameBuffer && pFrameBuffer->GetId() != SR_ID_INVALID) {
            int32_t id = pFrameBuffer->GetId() - 1;
            m_currentVkFrameBuffer = m_memory->GetFBO(id);
        }
        else {
            m_currentVkFrameBuffer = nullptr;
        }
    }

    bool VulkanPipeline::FreeFBO(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        bool hasErrors = !m_memory->FreeFBO(*id - 1);
        *id = SR_ID_INVALID;
        return !hasErrors;
    }

    bool VulkanPipeline::FreeShader(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        if (!m_memory->FreeShaderProgram(*id)) {
            PipelineError("VulkanPipeline::FreeShader() : failed free shader program!");
            return false;
        }

        *id = SR_ID_INVALID;

        return true;
    }

    int32_t VulkanPipeline::AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) {
        if (!m_memory) {
            SR_ERROR("VulkanPipeline::AllocateCubeMap() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;

        if (auto id = m_memory->AllocateTexture(createInfo.data, createInfo.width, createInfo.height, VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, 1, createInfo.cpuUsage); id >= 0) {
            return id;
        }

        PipelineError("VulkanPipeline::AllocateCubeMap() : failed to allocate texture!");
        return SR_ID_INVALID;
    }

    bool VulkanPipeline::FreeCubeMap(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        const bool result = m_memory->FreeTexture(*id);

        *id = SR_ID_INVALID;

        if (!result) {
            PipelineError("VulkanPipeline::FreeCubeMap() : failed to free texture! (" + std::to_string(*id) + ")");
            return false;
        }

        return true;
    }

    int32_t VulkanPipeline::AllocateVBO(const void* pVertices, Vertices::VertexType type, size_t count) {
        SR_TRACY_ZONE;

        if (!m_memory) {
            SR_ERROR("VulkanPipeline::AllocateVBO() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        const auto size = Vertices::GetVertexSize(type);

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += size * count;

        if (auto&& id = m_memory->AllocateVBO(size * count, pVertices); id >= 0) {
            return id;
        }

        return SR_ID_INVALID;
    }

    int32_t VulkanPipeline::AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) {
        SR_TRACY_ZONE;

        if (!m_memory) {
            SR_ERROR("VulkanPipeline::AllocateIBO() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += indexSize * count;

        if (auto&& id = m_memory->AllocateIBO(indexSize * count, pIndices); id >= 0) {
            return id;
        }

        return SR_ID_INVALID;
    }

    bool VulkanPipeline::FreeDescriptorSet(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

        if (!m_memory->FreeDescriptorSet(*id)) {
            SR_ERROR("Vulkan::FreeDescriptorSet() : failed to free descriptor set!");
            *id = SR_ID_INVALID;
            EVK_POP_LOG_LEVEL();
            return false;
        }

        EVK_POP_LOG_LEVEL();

        *id = SR_ID_INVALID;

        return true;
    }

    bool VulkanPipeline::FreeVBO(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        const bool result = m_memory->FreeVBO(*id);

        *id = SR_ID_INVALID;

        if (!result) {
            PipelineError("VulkanPipeline::FreeVBO() : failed to free VBO! (" + std::to_string(*id) + ")");
            return false;
        }

        return true;
    }

    bool VulkanPipeline::FreeIBO(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        const bool result = m_memory->FreeIBO(*id);

        *id = SR_ID_INVALID;

        if (!result) {
            PipelineError("VulkanPipeline::FreeIBO() : failed to free IBO! (" + std::to_string(*id) + ")");
            return false;
        }

        return true;
    }

    bool VulkanPipeline::FreeUBO(int32_t* id) {
        SR_TRACY_ZONE;

        WaitRenderIdle();

        ++m_state.operations;
        ++m_state.deletions;

        const bool result = m_memory->FreeUBO(*id);

        *id = SR_ID_INVALID;

        if (!result) {
            PipelineError("VulkanPipeline::FreeUBO() : failed to free UBO! (" + std::to_string(*id) + ")");
            return false;
        }

        return true;
    }

    void VulkanPipeline::SetOverlayEnabled(OverlayType overlayType, bool enabled) {
        Super::SetOverlayEnabled(overlayType, enabled);

        bool hasEnabled = false;
        for (auto&& [type, pOverlay] : m_overlays) {
            hasEnabled |= pOverlay->IsEnabled();
        }

        if (m_kernel && m_kernel->IsGUIEnabled() != hasEnabled) {
            m_kernel->SetGUIEnabled(hasEnabled);
        }
    }

    void VulkanPipeline::SetDirty(bool dirty) {
        SR_TRACY_ZONE;

        Super::SetDirty(dirty);

        if (m_dirty) {
            return;
        }

        /// Чистим старую очередь

        //m_kernel->ClearSubmitQueue();

        //auto&& queues = m_fboQueue.GetQueues();

        //for (auto&& queue : queues) {
        //    for (auto&& pFrameBuffer : queue) {
        //        if (auto&& fboId = pFrameBuffer->GetId(); fboId != SR_ID_INVALID) {
        //            auto&& vkFrameBuffer = m_memory->GetFBO(fboId - 1);
        //            vkFrameBuffer->ClearWaitSemaphores();
        //            vkFrameBuffer->ClearSignalSemaphores();
        //        }
        //        else {
        //            SR_ERROR("VulkanPipeline::SetDirty(false) : frame buffer id is invalid!");
        //        }
        //    }
        //}

        /// Определяем зависимости

        /*if (!queues.empty()) {
            /// Если являемся началом цепочки, то должны дождаться предыдущего кадра
            for (auto&& pFrameBuffer : queues.front()) {
                if (auto&& fboId = pFrameBuffer->GetId(); fboId != SR_ID_INVALID) {
                    auto&& vkFrameBuffer = m_memory->GetFBO(fboId - 1);
                    vkFrameBuffer->GetWaitSemaphores().emplace_back(m_kernel->GetPresentCompleteSemaphore());
                }
                else {
                    SR_ERROR("VulkanPipeline::SetDirty(false) : frame buffer id is invalid!");
                }
            }

            /// Если являемся концом цепочки, то нужно чтобы нас дождался рендер
            for (auto&& pFrameBuffer : queues.back()) {
                if (auto&& fboId = pFrameBuffer->GetId(); fboId != SR_ID_INVALID) {
                    auto&& vkFrameBuffer = m_memory->GetFBO(fboId - 1);
                    m_kernel->GetWaitSemaphores().emplace_back(vkFrameBuffer->GetSemaphore());
                }
                else {
                    SR_ERROR("VulkanPipeline::SetDirty(false) : frame buffer id is invalid!");
                }
            }
        }
        else {
            m_kernel->GetWaitSemaphores().emplace_back(m_kernel->GetPresentCompleteSemaphore());
        }

        for (uint32_t queueIndex = 1; queueIndex < queues.size(); ++queueIndex) {
            for (auto&& pFrameBuffer : queues[queueIndex]) {
                for (auto&& pDependency : queues[queueIndex - 1]) {
                    auto&& fboId = pFrameBuffer->GetId();
                    auto&& fboDependencyId = pDependency->GetId();

                    if (fboId == SR_ID_INVALID || fboDependencyId == SR_ID_INVALID) {
                        SR_ERROR("VulkanPipeline::SetDirty(false) : frame buffer's or it's dependency's id is invalid!");
                        continue;
                    }

                    auto&& vkFrameBuffer = m_memory->GetFBO(fboId - 1);
                    auto&& vkDependency = m_memory->GetFBO(fboDependencyId - 1);
                    vkFrameBuffer->GetWaitSemaphores().emplace_back(vkDependency->GetSemaphore());
                }
            }
        }

        /// Строим новую очередь

        for (auto&& queue : queues) {
            EvoVulkan::SubmitInfo submitInfo;

            submitInfo.SetWaitDstStageMask(m_kernel->GetSubmitPipelineStages());

            for (auto&& pFrameBuffer : queue) {
                auto&& fbId = pFrameBuffer->GetId();
                if (fbId == SR_ID_INVALID) {
                    SR_ERROR("VulkanPipeline::SetDirty(false) : frame buffer id is invalid!");
                    continue;
                }

                auto&& vkFrameBuffer = m_memory->GetFBO(fbId - 1);

                submitInfo.commandBuffers.emplace_back(vkFrameBuffer->GetCmd());

                for (auto&& signalSemaphore : vkFrameBuffer->GetSignalSemaphores()) {
                    submitInfo.AddSignalSemaphore(signalSemaphore);
                }

                for (auto&& waitSemaphore : vkFrameBuffer->GetWaitSemaphores()) {
                    submitInfo.AddWaitSemaphore(waitSemaphore);
                }
            }

            m_kernel->AddSubmitQueue(submitInfo);
        }*/
    }

    void VulkanPipeline::PushConstants(void* pData, uint64_t size) {
        Super::PushConstants(pData, size);

        if (!m_currentVkShader) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("Shader is nullptr!");
            return;
        }

        auto&& pushConstants = m_currentVkShader->GetPushConstants();

        if (pushConstants.size() != 1) SR_UNLIKELY_ATTRIBUTE {
            SRHaltOnce("Unsupported!");
            return;
        }

        if (size > pushConstants.data()->size) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("Push constants size is too big! Expected: {}, got: {}", pushConstants.data()->size, size);
            return;
        }

        vkCmdPushConstants(m_currentCmd, m_currentLayout,
            pushConstants.data()->stageFlags,
            0, size, pData
        );
    }

    void VulkanPipeline::PrepareFrame() {
        SR_TRACY_ZONE;

        Super::PrepareFrame();

        //if (m_kernel) {
        //    m_kernel->WaitFences();
        //}

        if (m_kernel && (m_kernel->IsDirty() || (m_kernel->GetSwapchain() && m_kernel->GetSwapchain()->IsDirty()))) {
            m_kernel->WaitAllFences();
            m_kernel->WaitIdle();
            m_kernel->WaitDeviceIdle();
            m_kernel->ReCreate(EvoVulkan::Core::FrameResult::Dirty);
        }

        for (auto&& [type, pOverlay] : m_overlays) {
            if (!pOverlay->IsSurfaceDirty()) {
                continue;
            }

            if (!pOverlay->ReCreate()) {
                PipelineError("VulkanPipeline::PrepareFrame() : failed to re-create \"" + pOverlay->GetName() + "\" overlay!");
            }
        }

        if (m_kernel) {
            m_kernel->PrepareFrame();
        }
    }

    void VulkanPipeline::OnMultiSampleChanged() {
        SR_INFO("VulkanPipeline::OnMultiSampleChanged() : samples count was changed to " + SR_UTILS_NS::ToString(GetSamplesCount()));
        if (m_kernel) {
            m_kernel->SetMultisampling(GetSamplesCount());
        }
        Super::OnMultiSampleChanged();
    }

    bool VulkanPipeline::BindDescriptorSet(uint32_t descriptorSet) {
        if (!Super::BindDescriptorSet(descriptorSet)) {
            return false;
        }

        SRAssert2(m_isComputeState || m_isRenderState, "Render or compute state must be active to bind descriptor set!");

        m_currentDescriptorSet = m_memory->GetDescriptorSet(descriptorSet).descriptorSet;

        return true;
    }

    void VulkanPipeline::BindAttachment(uint8_t activeTexture, uint32_t textureId) {
        SR_TRACY_ZONE;

        Super::BindAttachment(activeTexture, textureId);

        if (!m_bindedDescriptors.Get(m_state.descriptorSetId, false)) {
            PipelineError("Pipeline::BindAttachment() : descriptor set not binded!");
            return;
        }

        if (!m_isComputeState && !m_isRenderState) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::BindAttachment() : render state isn't active!");
            SRHaltOnce0();
            return;
        }

        if (!IsSamplerValid(static_cast<int32_t>(textureId))) {
            PipelineError("VulkanPipeline::BindAttachment() : texture is not exists!");
            return;
        }

        auto&& descriptorSet = m_memory->GetDescriptorSet(m_state.descriptorSetId);
        auto&& imageDescriptorRef = m_memory->GetTexture(textureId)->GetDescriptorRef();

        const auto&& descriptorSetWrite = EvoVulkan::Tools::Initializers::WriteDescriptorSet(
                descriptorSet.descriptorSet,
                VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, activeTexture,
                imageDescriptorRef);

        vkUpdateDescriptorSets(*m_kernel->GetDevice(), 1, &descriptorSetWrite, 0, nullptr);
    }

    void VulkanPipeline::BindVBO(uint32_t VBO) {
        Super::BindVBO(VBO);
        vkCmdBindVertexBuffers(m_currentCmd, 0, 1, m_memory->GetVBO(VBO)->GetCRef(), m_offsets);
    }

    void VulkanPipeline::BindIBO(uint32_t IBO) {
        Super::BindIBO(IBO);
        vkCmdBindIndexBuffer(m_currentCmd, *m_memory->GetIBO(IBO), 0, VK_INDEX_TYPE_UINT32);
    }

    bool VulkanPipeline::IsSamplerValid(int32_t id) const {
        return m_memory->IsTextureValid(id);
    }

    void VulkanPipeline::BindTexture(uint8_t activeTexture, uint32_t textureId) {
        SR_TRACY_ZONE;

        Super::BindTexture(activeTexture, textureId);

        if (!m_bindedDescriptors.Get(m_state.descriptorSetId, false)) {
            PipelineError("VulkanPipeline::BindTexture() : descriptor set not binded!");
            return;
        }

        if (!m_isRenderState) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::UpdateDescriptorSets() : render state isn't active or not in first build iteration!");
            SRHaltOnce0();
            return;
        }

        if (!IsSamplerValid(static_cast<int32_t>(textureId))) {
            PipelineError("VulkanPipeline::BindTexture() : texture is not exists! Id: " + SR_UTILS_NS::ToString(textureId));
            return;
        }

        auto&& descriptorSet = m_memory->GetDescriptorSet(m_state.descriptorSetId);
        auto&& pTexture = m_memory->GetTexture(textureId);

        // if (pTexture->GetImage().GetAspect() == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) SR_UNLIKELY_ATTRIBUTE {
        //     SRHalt("You are trying to use depth and stencil texture as sampler! Id: " + SR_UTILS_NS::ToString(textureId) +
        //         "\n\tOnly one of these aspects can be used as sampler!");
        //     return;
        // }

        auto&& imageDescriptorRef = pTexture->GetDescriptorRef();

        static std::set<VkImageLayout> allowedLayouts = {
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

        if (allowedLayouts.count(imageDescriptorRef->imageLayout) == 0) {
            PipelineError("VulkanPipeline::BindTexture() : texture has invalid layout! Id: " + SR_UTILS_NS::ToString(textureId) +
                "\n\tCheck \"Depth read\" option in your framebuffer if you are trying to use depth texture as sampler.");
            SRHaltOnce0();
            return;
        }

        const auto&& descriptorSetWrite = EvoVulkan::Tools::Initializers::WriteDescriptorSet(
                descriptorSet.descriptorSet,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, activeTexture,
                imageDescriptorRef);

        vkUpdateDescriptorSets(*m_kernel->GetDevice(), 1, &descriptorSetWrite, 0, nullptr);
    }

    void VulkanPipeline::Draw(uint32_t count) {
        SR_TRACY_ZONE;

        Super::Draw(count);

        if (m_currentDescriptorSet) {
            vkCmdBindDescriptorSets(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentLayout, 0, 1, &m_currentDescriptorSet, 0, nullptr);
        }

        const VkRect2D scissor = m_scissorsStack.empty() ? m_scissor : Details::ToVkRect2D(m_scissorsStack.back());
        if (!Details::CompareVkRect2D(m_activeScissor, scissor)) {
            m_activeScissor = scissor;
            vkCmdSetScissor(m_currentCmd, 0, 1, &m_activeScissor);
        }

        vkCmdDraw(m_currentCmd, count, m_drawInstancesCount, 0, m_drawInstancesStart);
    }

    void VulkanPipeline::DrawIndices(uint32_t count) {
        SR_TRACY_ZONE;

        Super::DrawIndices(count);

    #ifdef SR_RENDER_VALIDATION
        if (m_state.IBOId != SR_ID_INVALID) {
            auto&& pIBO = m_memory->GetIBO(m_state.IBOId);
            if (!pIBO) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("VulkanPipeline::DrawIndices() : IBO is nullptr!");
                return;
            }

            if (pIBO->GetDebugInfo().itemCount < count) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("VulkanPipeline::DrawIndices() : trying to draw more indices than IBO has! Has: {}, Draw: {}", pIBO->GetDebugInfo().itemCount, count);
                return;
            }
        }
    #endif

        if (m_currentDescriptorSet) {
            vkCmdBindDescriptorSets(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentLayout, 0, 1, &m_currentDescriptorSet, 0, nullptr);
        }

        const VkRect2D scissor = m_scissorsStack.empty() ? m_scissor : Details::ToVkRect2D(m_scissorsStack.back());
        if (!Details::CompareVkRect2D(m_activeScissor, scissor)) {
            m_activeScissor = scissor;
            vkCmdSetScissor(m_currentCmd, 0, 1, &m_activeScissor);
        }

        vkCmdDrawIndexed(m_currentCmd, count, m_drawInstancesCount, 0, 0, m_drawInstancesStart);
    }

    void VulkanPipeline::SetVSyncEnabled(bool enabled) {
        if (!m_kernel) {
            return;
        }

        SR_LOG("VulkanPipeline::SetVSyncEnabled() : setting VSync to {}", enabled ? "enabled" : "disabled");

        auto&& pSwapChain = m_kernel->GetSwapchain();
        if (!pSwapChain) {
            return;
        }

        pSwapChain->SetVSync(enabled);
    }

    bool VulkanPipeline::IsVSyncEnabled() const {
        if (!m_kernel) {
            return false;
        }

        auto&& pSwapChain = m_kernel->GetSwapchain();
        if (!pSwapChain) {
            return false;
        }

        return pSwapChain->IsVSyncEnabled();
    }

    void VulkanPipeline::ResetLastShader() {
        m_lastVkShader = nullptr;
        Super::ResetLastShader();
    }

    void VulkanPipeline::ClearDepthBuffer(float_t depth) {
        SR_TRACY_ZONE;

        const uint32_t layer = m_state.frameBufferLayer == SR_ID_INVALID ? 0 : m_state.frameBufferLayer;
        auto&& pLayer = m_currentVkFrameBuffer->GetLayers()[layer];
        auto&& image = pLayer->GetDepthAttachment()->GetImage();

        if (image.GetLayout() == VK_IMAGE_LAYOUT_UNDEFINED) {
            SR_ERROR("VulkanPipeline::ClearDepthBuffer() : image layout is VK_IMAGE_LAYOUT_UNDEFINED!");
            return;
        }

        if (!(image.GetInfo().usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
            SR_ERROR("VulkanPipeline::ClearDepthBuffer() : image usage don't contain VK_IMAGE_USAGE_TRANSFER_DST_BIT!");
            return;
        }

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = image.GetLayout();
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // --- determine proper srcAccessMask & srcStage depending on oldLayout ---
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkAccessFlags srcAccess = 0;

        if (barrier.oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } else if (barrier.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            srcAccess = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // or relevant shader stage
        } else {
            srcAccess = 0;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }

        barrier.srcAccessMask = srcAccess;

        vkCmdPipelineBarrier(
                m_currentCmd,
                //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                srcStage,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );

        if (depth < 0.0f || depth > 1.0f) {
            SR_ERROR("VulkanPipeline::ClearDepthBuffer() : depth value must be in range [0.0, 1.0]!");
            depth = std::clamp(depth, 0.0f, 1.0f);
        }

        VkClearDepthStencilValue depthStencilValue;
        depthStencilValue.depth = depth;
        depthStencilValue.stencil = 0;

        vkCmdClearDepthStencilImage(
                m_currentCmd,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                &depthStencilValue,
                1,
                &barrier.subresourceRange
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = image.GetLayout();
        //barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //barrier.dstAccessMask = 0;


        //barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        //barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = srcAccess;// VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        //barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
                m_currentCmd,
                //VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,

                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,

                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );

        Super::ClearDepthBuffer(depth);
    }

    void VulkanPipeline::ClearColorBuffer(const ClearColors& clearColors) {
        SR_TRACY_ZONE;

        const uint32_t layer = m_state.frameBufferLayer == SR_ID_INVALID ? 0 : m_state.frameBufferLayer;
        auto&& pLayer = m_currentVkFrameBuffer->GetLayers()[layer];

        auto&& clearBufferFunction = [this](const EvoVulkan::Types::Image& image, VkClearColorValue clearColor) {
            if (image.GetLayout() == VK_IMAGE_LAYOUT_UNDEFINED) {
                SR_ERROR("VulkanPipeline::ClearColorBuffer() : image layout is VK_IMAGE_LAYOUT_UNDEFINED!");
                return;
            }

            if (!(image.GetInfo().usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
                SRHalt("VulkanPipeline::ClearColorBuffer() : image usage don't contain VK_IMAGE_USAGE_TRANSFER_DST_BIT!");
                return;
            }

            const VkImageLayout oldLayout = image.GetLayout();

            // --- determine proper srcAccessMask & srcStage depending on oldLayout ---
            VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkAccessFlags srcAccess = 0;

            if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                srcAccess = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else {
                srcAccess = 0;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }

            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = srcAccess;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(
                    m_currentCmd,
                    srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
            );

            vkCmdClearColorImage(
                    m_currentCmd,
                    image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &clearColor,
                    1,
                    &barrier.subresourceRange
            );

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = image.GetLayout();
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(
                    m_currentCmd,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
        };

        for (uint32_t i = 0; i < clearColors.size(); ++i) {
            auto&& color = clearColors[i];

            if (i >= pLayer->GetColorAttachments().size()) {
                PipelineError("VulkanPipeline::ClearColorBuffer() : color attachment index is out of range!");
                continue;
            }

            auto&& pColorAttachment = pLayer->GetColorAttachments()[i];
            if (!pColorAttachment) {
                PipelineError("VulkanPipeline::ClearColorBuffer() : color attachment is nullptr!");
                continue;
            }

            clearBufferFunction(pColorAttachment->GetImage(), VkClearColorValue {
                .float32 = { color.r, color.g, color.b, color.a }
            });

            if (!pLayer->GetResolveAttachments().empty()) {
                auto&& pResolveAttachment = pLayer->GetResolveAttachments()[i];
                if (pResolveAttachment) {
                    clearBufferFunction(pResolveAttachment->GetImage(), VkClearColorValue{
                            .float32 = {color.r, color.g, color.b, color.a}
                    });
                }
            }
        }

        Super::ClearColorBuffer(clearColors);
    }

    void VulkanPipeline::ClearDepthAttachment(float_t depth) {
        SR_TRACY_ZONE;

        if (!m_isRenderState) {
            PipelineError("VulkanPipeline::ClearDepthAttachment() : RenderPass is not active!");
            return;
        }

        if (!m_currentCmd) {
            PipelineError("VulkanPipeline::ClearDepthAttachment() : cmd buffer is nullptr!");
            return;
        }

        if (depth < 0.0f || depth > 1.0f) {
            SR_ERROR("VulkanPipeline::ClearDepthAttachment() : depth value must be in range [0.0, 1.0]!");
            depth = std::clamp(depth, 0.0f, 1.0f);
        }

        // Use vkCmdClearAttachments to clear depth inside active RenderPass
        // This is the correct Vulkan way to clear attachments during rendering
        // Works for both framebuffers and swapchain (when m_currentVkFrameBuffer is nullptr)
        VkClearAttachment clearAttachment = {};
        clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        clearAttachment.clearValue.depthStencil.depth = depth;
        clearAttachment.clearValue.depthStencil.stencil = 0;

        // Clear the entire render area
        VkClearRect clearRect = {};
        clearRect.rect = m_renderPassBI.renderArea;
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        vkCmdClearAttachments(
            m_currentCmd,
            1,
            &clearAttachment,
            1,
            &clearRect
        );

        Super::ClearDepthAttachment(depth);
    }

    bool VulkanPipeline::FreeSSBO(int32_t* id) {
        SR_TRACY_ZONE;

        ++m_state.operations;
        ++m_state.deletions;

        WaitRenderIdle();

        const bool result = m_memory->FreeSSBO(*id);

        *id = SR_ID_INVALID;

        if (!result) {
            PipelineError("VulkanPipeline::FreeSSBO() : failed to free SSBO! (" + std::to_string(*id) + ")");
            return false;
        }

        return true;
    }

    int32_t VulkanPipeline::AllocateSSBO(uint32_t size, SSBOUsage usage) {
        if (!m_memory) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("VulkanPipeline::AllocateSSBO() : memory manager is nullptr!");
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += size;

        SRAssert2(size > 0, "Incorrect SSBO size!");

        if (auto&& id = m_memory->AllocateSSBO(size, usage); id >= 0) SR_LIKELY_ATTRIBUTE {
            return id;
        }

        PipelineError("VulkanPipeline::AllocateSSBO() : failed to allocate shader storage buffer object!");
        return SR_ID_INVALID;
    }

    void VulkanPipeline::BindSSBO(uint32_t SSBO) {
        Super::BindSSBO(SSBO);
    }

    uint16_t VulkanPipeline::GetSwapchainImagesCount() const {
        return m_kernel ? m_kernel->GetSwapchainImagesCount() : 0;
    }

    uint8_t VulkanPipeline::GetCurrentFrameIndex() const {
        return m_kernel ? m_kernel->GetCurrentFrameIndex() : 0;
    }

    uint8_t VulkanPipeline::GetCurrentImageIndex() const {
        return m_kernel ? m_kernel->GetCurrentImageIndex() : 0;
    }

    void* VulkanPipeline::GetCurrentShaderHandle() const {
        ++m_state.operations;

        if (!m_state.pShader) SR_UNLIKELY_ATTRIBUTE {
            return nullptr;
        }

        auto&& shaderProgram = m_state.pShader->GetId();
        if (shaderProgram == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            return nullptr;
        }

        return (void*)m_memory->GetShaderProgram(shaderProgram)->GetPipeline();
    }

    void VulkanPipeline::ResetSubmitQueue() {
        m_kernel->ClearSubmitQueue();
        //m_kernel->GetWaitSemaphores().emplace_back(m_kernel->GetPresentCompleteSemaphore());

        Super::ResetSubmitQueue();
    }

    void VulkanPipeline::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        SR_TRACY_ZONE;

        if (!m_currentCmd) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::Dispatch() : cmd buffer is nullptr!");
            return;
        }

        if (!m_currentVkShader) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::Dispatch() : current shader is nullptr!");
            return;
        }

        if (!m_state.pShader) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::Dispatch() : shader program is nullptr!");
            return;
        }

        if (m_state.pShader->GetType() != SR_SRSL_NS::ShaderType::Compute) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("VulkanPipeline::Dispatch() : current shader is not a compute shader!");
            return;
        }

        Super::Dispatch(groupCountX, groupCountY, groupCountZ);

        if (m_currentDescriptorSet) {
            vkCmdBindDescriptorSets(m_currentCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_currentLayout, 0, 1, &m_currentDescriptorSet, 0, nullptr);
        }

        vkCmdDispatch(m_currentCmd, groupCountX, groupCountY, groupCountZ);
    }

    bool VulkanPipeline::BeginCompute() {
        if (!Super::BeginCompute()) {
            return false;
        }

        m_lastVkShader = nullptr;
        m_isShaderChanged = true;

        m_currentCmd = m_kernel->GetComputeCmdBuffers()[0];
        vkBeginCommandBuffer(m_currentCmd, &m_cmdBufInfo);

        return true;
    }

    void VulkanPipeline::EndCompute() {
        Super::EndCompute();

        vkEndCommandBuffer(m_currentCmd);
    }

    bool VulkanPipeline::MapSSBO(uint32_t SSBO, void **ppData) {
        SR_TRACY_ZONE;

        if (!ppData) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("VulkanPipeline::MapSSBO() : ppData is nullptr!");
            return false;
        }

        if (SSBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("VulkanPipeline::MapSSBO() : invalid SSBO ID!");
            return false;
        }

        *ppData = m_memory->GetSSBO(SSBO)->MapData();
        return true;
    }

    void VulkanPipeline::UnMapSSBO(uint32_t SSBO) {
        SR_TRACY_ZONE;
        SRAssert2(SSBO != SR_ID_INVALID, "Invalid SSBO ID!");
        m_memory->GetSSBO(SSBO)->Unmap();
    }

    void VulkanPipeline::SetFrameBufferAccessMode(FrameBufferAccessMode mode) {
        /// Необходимо поменять режим доступа к текущему фреймбуферу
        /// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - для рендеринга (Write)
        /// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL - для чтения в шейдере (Read)

        SR_TRACY_ZONE;

        auto&& pCurrentFBO = GetCurrentFrameBuffer();
        if (!m_currentVkFrameBuffer) {
            SRHalt("VulkanPipeline::SetFrameBufferAccessMode() : current frame buffer is nullptr!");
            return;
        }

        auto&& features = pCurrentFBO->GetFeatures();

        const auto newColorLayout = (mode == FrameBufferAccessMode::Read && features.colorShaderRead) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        for (uint32_t layer = 0; layer < pCurrentFBO->GetColorLayersCount(); ++layer) {
            int32_t colorTexture = pCurrentFBO->GetColorTexture(layer, GetCurrentImageIndex());
            if (colorTexture == SR_ID_INVALID) {
                continue;
            }
            m_memory->GetTexture(colorTexture)->GetImage().TransitionImageLayout(newColorLayout, m_currentCmd);
        }

        //for (auto&& colorAttachment : pLayer->GetColorAttachments()) {
        //    const auto newLayout = (mode == FrameBufferAccessMode::Read && features.colorShaderRead) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        //    colorAttachment->GetImage().TransitionImageLayout(newLayout, m_currentCmd);
        //}

        //if (auto&& pDepth = pLayer->GetDepthAttachment()) {
        //    const auto newLayout = (mode == FrameBufferAccessMode::Read && features.depthShaderRead) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        //    pDepth->GetImage().TransitionImageLayout(newLayout, m_currentCmd);
        //}

        Super::SetFrameBufferAccessMode(mode);
    }

    void VulkanPipeline::SetSwapchainImagesCount(uint16_t count) {
        if (m_kernel) {
            m_kernel->SetSwapchainImagesCount(count);
        }

        Super::SetSwapchainImagesCount(count);
    }

    bool VulkanPipeline::IsShaderViewportIndexLayerSupported() const noexcept {
        ++m_state.operations;
        return m_kernel && m_kernel->GetDevice() && m_kernel->GetDevice()->IsShaderViewportIndexLayerSupported();
    }

    uint16_t VulkanPipeline::GetMaxFramesInFlight() const {
        if (m_kernel) {
            return m_kernel->GetMaxFramesInFlight();
        }
        return 0;
    }
}