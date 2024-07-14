//
// Created by Nikita on 17.11.2020.
//

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/Xml.h>
#include <Utils/Types/Thread.h>
#include <Utils/Types/DataStorage.h>
#include <Utils/Common/Hashes.h>

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/TypeInfo.h>

namespace SR_GRAPH_NS::Types {
    Shader::Shader()
        : IResource(SR_COMPILE_TIME_CRC32_TYPE_NAME(Shader))
        , m_manager(Memory::ShaderProgramManager::Instance())
        , m_uboManager(Memory::UBOManager::Instance())
    { }

    Shader::~Shader() {
        m_samplers.clear();
    }

    bool Shader::Init() {
        SR_TRACY_ZONE;

        SR_SHADER("Shader::Init() : initialize \"" + GetResourceId().ToStringRef() + "\" shader...");

        if (m_isCalculated) {
            SRHalt("Double shader initialization!");
            return true;
        }

        auto&& pContext = SR_THIS_THREAD->GetContext()->GetValue<SR_HTYPES_NS::SafePtr<RenderContext>>();
        if (!pContext) {
            SRHalt("Render context is nullptr!");
            m_hasErrors = true;
            return false;
        }

        if (pContext.LockIfValid()) {
            for (auto&& [hashName, sampler] : m_samplers) {
                if (sampler.isAttachment || sampler.isArray) {
                    continue;
                }
                sampler.samplerId = pContext->GetDefaultTexture()->GetId();
            }

            if (!m_isRegistered) {
                pContext->Register(this);
                m_isRegistered = true;
            }

            pContext.Unlock();
        }

        if (!m_shaderCreateInfo.Validate()) {
            SR_ERROR("Shader::Init() : failed to validate shader!\n\tPath: " + GetResourcePath().ToString());
            m_hasErrors = true;
            return false;
        }

        m_shaderProgram = Memory::ShaderProgramManager::Instance().ReAllocate(m_shaderProgram, m_shaderCreateInfo);
        if (m_shaderProgram == SR_ID_INVALID) {
            SR_ERROR("Shader::Init() : failed to allocate shader program!");
            m_hasErrors = true;
            return false;
        }

        /// calculate shader params hash
        {
            auto&& hash = SR_UTILS_NS::HashCombine(m_properties, 0);
            hash = SR_UTILS_NS::HashCombine(m_samplers, hash);
            SetResourceHash(hash);
        }

        m_isCalculated = true;

        return true;
    }

    ShaderBindResult Shader::Use() noexcept {
        SR_TRACY_ZONE;

        if (m_hasErrors) {
            return ShaderBindResult::Failed;
        }

        if (!m_isCalculated && !Init()) {
            SR_ERROR("Shader::Use() : failed to initialize shader!");
            return ShaderBindResult::Failed;
        }

        SRAssert(GetRenderContext());

        auto&& bindResult = Memory::ShaderProgramManager::Instance().BindProgram(m_shaderProgram);
        switch (bindResult) {
            case ShaderBindResult::Success:
            case ShaderBindResult::Duplicated:
            case ShaderBindResult::ReAllocated:
                GetRenderContext()->SetCurrentShader(this);
                break;
            case ShaderBindResult::Failed:
                SR_ERROR("Shader::Use() : failed to bind shader!");
                break;
            default:
                SRHaltOnce("Unexcepted behaviour!");
                break;
        }

        if (m_virtualUBO.second) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO.first = m_uboManager.AllocateUBO(m_virtualUBO.first, m_uniformSharedBlock.m_size);
            if (m_virtualUBO.first == SR_ID_INVALID) {
                SR_ERROR("Shader::Use() : failed to allocate UBO!");
                m_hasErrors = true;
                return ShaderBindResult::Failed;
            }

            m_virtualUBO.second = false;
        }

        if (m_uboManager.BindUBO(m_virtualUBO.first, m_uniformSharedBlock.m_size) == Memory::UBOManager::BindResult::Failed) {
            SR_ERROR("Shader::Use() : failed to bind UBO!");
            m_hasErrors = true;
            return ShaderBindResult::Failed;
        }

        return bindResult;
    }

    void Shader::UnUse() noexcept {
        auto&& pCurrentShader = GetRenderContext()->GetCurrentShader();

        if (pCurrentShader == this) {
            GetPipeline()->UnUseShader();
        }
        else {
            SRHalt("You are trying to unuse wrong shader!");
        }
    }

    void Shader::FreeVideoMemory() {
        if (m_isCalculated) {
            SR_SHADER("Shader::FreeVideoMemory() : free \"" + GetResourceId().ToStringRef() + "\" video memory...");

            if (!Memory::ShaderProgramManager::Instance().FreeProgram(&m_shaderProgram)) {
                SR_ERROR("Shader::Free() : failed to free shader program! \n\tPath: " + GetResourcePath().ToString());
            }

            if (m_virtualUBO.first != SR_ID_INVALID) {
                if (!m_uboManager.FreeUBO(&m_virtualUBO.first)) {
                    SR_ERROR("Shader::Free() : failed to free virtual UBO! \n\tPath: " + GetResourcePath().ToString());
                }
            }
            m_virtualUBO.second = true;

            m_isCalculated = false;
        }
    }

    Shader* Shader::Load(const SR_UTILS_NS::Path &rawPath) {
        SR_TRACY_ZONE;

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(resourceManager.GetResPath());

        if (auto&& pShader = resourceManager.Find<Shader>(path)) {
            return pShader;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Medium) {
            SR_LOG("Shader::Load() : load \"" + path.ToString() + "\" shader...");
        }

        if (!SRVerifyFalse2(path.IsEmpty(), "Invalid shader path!")) {
            SR_WARN("Shader::Load() : failed to load shader!");
            return nullptr;
        }

        if (path.GetExtensionView() != "srsl") {
            SR_ERROR("Shader::Load() : unknown extension!");
            return nullptr;
        }

        auto&& pShader = new Shader();

        pShader->SetId(path.ToString(), false);

        if (!pShader->Reload()) {
            SR_ERROR("Shader::Load() : failed to reload shader!\n\tPath: " + path.ToString());
            pShader->DeleteResource();
            return nullptr;
        }

        resourceManager.RegisterResource(pShader);

        return pShader;
    }

    int32_t Shader::GetID() {
        return GetId();
    }

    int32_t Shader::GetId() noexcept {
        if (m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return SR_ID_INVALID;
        }

        if (!m_isCalculated) SR_UNLIKELY_ATTRIBUTE {
            if (!Init()) {
                SR_ERROR("Shader::GetId() : failed to initialize shader!");
                return SR_ID_INVALID;
            }
        }

        return m_manager.GetProgram(m_shaderProgram);
    }

    void Shader::SetBool(uint64_t hashId, bool v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetFloat(uint64_t hashId, float_t v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetInt(uint64_t hashId, int32_t v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetMat4(uint64_t hashId, const SR_MATH_NS::Matrix4x4& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetVec3(uint64_t hashId, const SR_MATH_NS::FVector3& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetVec4(uint64_t hashId, const SR_MATH_NS::FVector4& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetRect(uint64_t hashId, const SR_MATH_NS::FRect& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetVec2(uint64_t hashId, const SR_MATH_NS::FVector2& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetIVec2(uint64_t hashId, const SR_MATH_NS::IVector2& v) noexcept { SetValue<false>(hashId, &v); }

    void Shader::SetConstBool(uint64_t hashId, bool v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstFloat(uint64_t hashId, float_t v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstInt(uint64_t hashId, int32_t v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstMat4(uint64_t hashId, const SR_MATH_NS::Matrix4x4& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstVec3(uint64_t hashId, const SR_MATH_NS::FVector3& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstVec4(uint64_t hashId, const SR_MATH_NS::FVector4& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstVec2(uint64_t hashId, const SR_MATH_NS::FVector2& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstIVec2(uint64_t hashId, const SR_MATH_NS::IVector2& v) noexcept { SetValue<true>(hashId, &v); }

    void Shader::SetSampler(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept {
        m_samplers.at(name).samplerId = sampler;
    }

    void Shader::SetSampler2D(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept {
        if (!IsLoaded() || m_samplers.count(name) == 0) {
            return;
        }

        SetSampler(name, sampler);
    }

    void Shader::SetSamplerCube(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept {
        if (!IsLoaded() || m_samplers.count(name) == 0) {
            return;
        }

        SetSampler(name, sampler);
    }

    void Shader::BindSSBO(SR_UTILS_NS::StringAtom name, uint32_t ssbo) noexcept {
        for (auto&& ssboBinding : m_ssboBindings) {
            if (ssboBinding.name == name) {
                ssboBinding.ssbo = ssbo;
                return;
            }
        }
    }

    void Shader::SetSampler2D(SR_UTILS_NS::StringAtom name, SR_GTYPES_NS::Texture* pSampler) noexcept {
        if (!IsLoaded() || m_samplers.count(name) == 0) {
            return;
        }

        if (!pSampler) {
            pSampler = GetRenderContext()->GetNoneTexture();
        }

        if (!pSampler) {
            SRHalt("The default sampler is nullptr!");
            return;
        }

        SetSampler(name, pSampler->GetId());
    }

    bool Shader::Ready() const {
        return !m_hasErrors && m_isCalculated && m_shaderProgram != SR_ID_INVALID;
    }

    uint64_t Shader::GetUBOBlockSize() const {
        return m_uniformBlock.m_size;
    }

    bool Shader::InitUBOBlock() {
        if (m_uniformBlock.m_memory) SR_LIKELY_ATTRIBUTE {
            memset(m_uniformBlock.m_memory, 1, m_uniformBlock.m_size);
        }
        else {
            return false;
        }

        auto&& ubo = GetPipeline()->GetCurrentUBO();
        auto&& descriptorSet = GetPipeline()->GetCurrentDescriptorSet();

        if (ubo != SR_ID_INVALID && descriptorSet != SR_ID_INVALID && m_uniformBlock.Valid()) SR_LIKELY_ATTRIBUTE {
            SRDescriptorUpdateInfo updateInfo;
            updateInfo.binding = m_uniformBlock.m_binding;
            updateInfo.ubo = ubo;
            updateInfo.descriptorType = DescriptorType::Uniform;

            GetPipeline()->UpdateDescriptorSets(descriptorSet, { updateInfo });

            return true;
        }

        return false;
    }

    bool Shader::Flush() const {
        if (!m_uniformBlock.m_memory) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        auto&& ubo = m_pipeline->GetCurrentUBO();
        if (ubo != SR_ID_INVALID && m_uniformBlock.Valid()) SR_LIKELY_ATTRIBUTE {
            m_pipeline->UpdateUBO(ubo, m_uniformBlock.m_memory, m_uniformBlock.m_size);
        }

        return true;
    }

    uint32_t Shader::GetSamplersCount() const {
        return m_samplers.size();
    }

    ShaderProperties Shader::GetProperties() {
        return m_properties;
    }

    SR_UTILS_NS::Path Shader::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    void Shader::OnReloadDone() {
        m_virtualUBO.second = true;

        auto&& pContext = GetRenderContext();
        if (!pContext) {
            return;
        }

        /// пока ресурс жив, контекст будет существовать (если ресурс зарегистрирован)
        pContext->Do([](RenderContext* ptr) {
            ptr->SetDirty();
        });

        IResource::OnReloadDone();
    }

    bool Shader::Load() {
        SR_TRACY_ZONE;

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(GetResourceId());

        if (path.IsAbs()) {
            SR_ERROR("Shader::Load() : absolute path is not allowed!");
            return false;
        }

        auto&& pShader = SR_SRSL_NS::SRSLShader::Load(path);
        if (!pShader) {
            SR_ERROR("Shader::Load() : failed to load srsl shader!\n\tPath: " + path.ToString());
            return false;
        }

        if (!pShader->Export(SRSL2::ShaderLanguage::GLSL)) {
            SR_ERROR("Shader::Load() : failed to export srsl shader!\n\tPath: " + path.ToString());
            return false;
        }

        m_shaderCreateInfo = pShader->GetCreateInfo();
        m_type = pShader->GetType();
        m_includes = pShader->GetIncludes();

        if (m_includes.empty()) {
            SR_ERROR("Shader::Load() : failed to extract includes!\n\tPath: " + path.ToString());
            return false;
        }

        StopWatch();
        StartWatch();

        /// ------------------------------------------------------------------------------------------------------------

        if (auto&& pBlock = pShader->FindUniformBlock("BLOCK")) {
            for (auto&& field : pBlock->fields) {
                m_uniformBlock.Append(field.name.GetHash(), field.size, field.alignedSize, !field.isPublic);

                const ShaderVarType varType = SR_SRSL_NS::SRSLTypeInfo::Instance().StringToType(field.type);

                if (field.isPublic && varType != ShaderVarType::Unknown) {
                    m_properties.emplace_back(ShaderProperty(field.name, varType, field.defaultValue));
                }
            }

            m_uniformBlock.m_binding = pBlock->binding;
        }

        m_uniformBlock.Init();

        /// ------------------------------------------------------------------------------------------------------------

        if (auto&& pBlock = pShader->FindUniformBlock("SHARED")) {
            for (auto&& field : pBlock->fields) {
                m_uniformSharedBlock.Append(field.name.GetHash(), field.size, field.alignedSize, !field.isPublic);

                const ShaderVarType varType = SR_SRSL_NS::SRSLTypeInfo::Instance().StringToType(field.type);

                if (field.isPublic && varType != ShaderVarType::Unknown) {
                    m_properties.emplace_back(ShaderProperty(field.name, varType, field.defaultValue));
                }
            }

            m_uniformSharedBlock.m_binding = pBlock->binding;
        }

        m_uniformSharedBlock.Init();

        /// ------------------------------------------------------------------------------------------------------------

        for (auto&& field : pShader->GetPushConstants().fields) {
            m_constBlock.Append(field.name.GetHash(), field.size, field.alignedSize, !field.isPublic);

            const ShaderVarType varType = SR_SRSL_NS::SRSLTypeInfo::Instance().StringToType(field.type);

            if (field.isPublic && varType != ShaderVarType::Unknown) {
                m_properties.emplace_back(ShaderProperty(field.name, varType));
            }
        }

        m_constBlock.Init();

        /// ------------------------------------------------------------------------------------------------------------

        for (auto&& [name, ssbo] : pShader->GetSSBOBlocks()) {
            SSBOBinding ssboBinding;
            ssboBinding.name = name;
            ssboBinding.binding = ssbo.binding;
            ssboBinding.ssbo = SR_ID_INVALID;
            m_ssboBindings.emplace_back(ssboBinding);
        }

        /// ------------------------------------------------------------------------------------------------------------

        for (auto&& [name, sampler] : pShader->GetSamplers()) {
            m_samplers[name].binding = sampler.binding;
            m_samplers[name].isAttachment = sampler.attachment >= 0;
            m_samplers[name].isArray = sampler.type.Contains("Array");

            const ShaderVarType varType = SR_SRSL_NS::SRSLTypeInfo::Instance().StringToType(sampler.type);

            if (sampler.isPublic && varType != ShaderVarType::Unknown) {
                m_properties.emplace_back(ShaderProperty(name, varType));
            }
        }

        return IResource::Load();
    }

    bool Shader::Unload() {
        bool hasErrors = !IResource::Unload();

        m_isCalculated = false;
        m_hasErrors = false;

        m_uniformBlock.DeInit();
        m_uniformSharedBlock.DeInit();
        m_constBlock.DeInit();

        m_ssboBindings.clear();
        m_includes.clear();
        m_properties.clear();
        m_samplers.clear();

        return !hasErrors;
    }

    bool Shader::IsBlendEnabled() const {
        return m_shaderCreateInfo.blendEnabled;
    }

    SR_SRSL_NS::ShaderType Shader::GetType() const noexcept {
        return m_type;
    }

    bool Shader::IsAllowedToRevive() const {
        return true;
    }

    void Shader::ReviveResource() {
        m_isCalculated = false;
        m_isRegistered = false;
        m_hasErrors = false;
        IResource::ReviveResource();
    }

    bool Shader::IsAvailable() const {
        SR_TRACY_ZONE;
        return m_manager.IsAvailable(m_shaderProgram);
    }

    void Shader::FlushSamplers() {
        for (auto&& [hashName, samplerInfo] : m_samplers) {
            if (samplerInfo.isAttachment) {
                m_pipeline->BindAttachment(samplerInfo.binding, samplerInfo.samplerId);
            }
            else {
                m_pipeline->BindTexture(samplerInfo.binding, samplerInfo.samplerId);
            }
        }
    }

    void Shader::FlushConstants() {
        if (m_constBlock.m_size > 0) {
            m_pipeline->PushConstants(m_constBlock.m_memory, m_constBlock.m_size);
        }
    }

    void Shader::StartWatch() {
        auto&& resourcesManager = SR_UTILS_NS::ResourceManager::Instance();

        for (auto&& path : m_includes) {
            auto&& pWatch = resourcesManager.StartWatch(resourcesManager.GetResPath().Concat(path));

            pWatch->SetCallBack([this](auto&& pWatcher) {
                SignalWatch();
            });

            pWatch->Init();

            m_watchers.emplace_back(pWatch);
        }
    }

    bool Shader::IsSamplersValid() const {
        for (auto&& [hashName, samplerInfo] : m_samplers) {
            if (!samplerInfo.isArray && !samplerInfo.isAttachment) {
                continue;
            }

            if (!m_pipeline->IsSamplerValid(samplerInfo.samplerId)) {
                return false;
            }
        }

        return true;
    }

    bool Shader::BeginSharedUBO() {
        if (m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        if (m_sharedUBOMode) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("Shared UBO mode is already enabled!");
            return false;
        }

        if (m_uniformSharedBlock.m_memory) SR_LIKELY_ATTRIBUTE {
            memset(m_uniformSharedBlock.m_memory, 1, m_uniformSharedBlock.m_size);
        }

        m_pipeline->SetCurrentShader(this);

        if (m_virtualUBO.second) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO.first = m_uboManager.AllocateUBO(m_virtualUBO.first, m_uniformSharedBlock.m_size);
            if (m_virtualUBO.first == SR_ID_INVALID) {
                SR_ERROR("Shader::BeginSharedUBO() : failed to allocate UBO!");
                m_hasErrors = true;
                return false;
            }

            if (m_uboManager.BindUBO(m_virtualUBO.first, m_uniformSharedBlock.m_size) == Memory::UBOManager::BindResult::Failed) {
                SR_ERROR("Shader::BeginSharedUBO() : failed to bind UBO!");
                m_hasErrors = true;
                return false;
            }

            m_virtualUBO.second = false;
        }
        else {
            if (m_uboManager.BindUBO(m_virtualUBO.first, m_uniformSharedBlock.m_size) == Memory::UBOManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("Failed to bind UBO!");
                return false;
            }
        }

        m_sharedUBOMode = true;

        return true;
    }

    void Shader::EndSharedUBO() {
        if (!m_sharedUBOMode) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("Shared UBO mode is not enabled!");
            return;
        }

        m_sharedUBOMode = false;

        if (m_uniformSharedBlock.Valid()) SR_LIKELY_ATTRIBUTE {
            m_pipeline->UpdateUBO(m_pipeline->GetCurrentUBO(), m_uniformSharedBlock.m_memory, m_uniformSharedBlock.m_size);
        }
    }

    void Shader::AttachDescriptorSets() {
        SR_TRACY_ZONE;

        for (auto&& [hashName, samplerInfo] : m_samplers) {
            if (samplerInfo.isAttachment) {
                m_pipeline->BindAttachment(samplerInfo.binding, samplerInfo.samplerId);
            }
            else {
                m_pipeline->BindTexture(samplerInfo.binding, samplerInfo.samplerId);
            }
        }

        auto&& descriptorSet = GetPipeline()->GetCurrentDescriptorSet();
        if (descriptorSet == SR_ID_INVALID) {
            return;
        }

        auto&& ubo = GetPipeline()->GetCurrentUBO();

        if (ubo != SR_ID_INVALID && descriptorSet != SR_ID_INVALID && m_uniformBlock.Valid()) SR_LIKELY_ATTRIBUTE {
            SRDescriptorUpdateInfo updateInfo;
            updateInfo.binding = m_uniformBlock.m_binding;
            updateInfo.ubo = ubo;
            updateInfo.descriptorType = DescriptorType::Uniform;

            GetPipeline()->UpdateDescriptorSets(descriptorSet, { updateInfo });
        }

        if (m_uniformSharedBlock.Valid()) {
            SRDescriptorUpdateInfo updateInfo;
            updateInfo.binding = m_uniformSharedBlock.m_binding;
            updateInfo.ubo = m_uboManager.GetUBO(m_virtualUBO.first);
            updateInfo.descriptorType = DescriptorType::Uniform;

            GetPipeline()->UpdateDescriptorSets(descriptorSet, { updateInfo });
        }

        for (auto&& ssbo : m_ssboBindings) {
            if (ssbo.ssbo == SR_ID_INVALID) {
                SR_ERROR("Shader::AttachDescriptorSets() : invalid \"{}\" SSBO!", ssbo.name.ToStringView());
                continue;
            }
            SRDescriptorUpdateInfo updateInfo;
            updateInfo.binding = ssbo.binding;
            updateInfo.ubo = ssbo.ssbo;
            updateInfo.descriptorType = DescriptorType::Storage;

            GetPipeline()->UpdateDescriptorSets(descriptorSet, { updateInfo });

            ssbo.ssbo = SR_ID_INVALID;
        }
    }
}