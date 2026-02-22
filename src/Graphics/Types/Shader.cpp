//
// Created by Nikita on 17.11.2020.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/TypeInfo.h>
#include <Graphics/SRSL/Cache.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/FileWatcher.h>
#include <Utils/Types/DataStorage.h>
#include <Utils/Common/Hashes.h>

#include <Codegen/Shader.generated.hpp>

namespace SR_GRAPH_NS::Types {
    Shader::Shader()
        : m_manager(Memory::ShaderProgramManager::Instance())
        , m_uboManager(Memory::UBOManager::Instance())
    { }

    Shader::~Shader() {
        m_samplers.clear();
        SRAssert(m_defaultSamplers.empty());
        SRAssert(m_shaderProgram == SR_ID_INVALID);
        SRAssert(m_virtualUBO.first == SR_ID_INVALID);
    }

    bool Shader::Init() {
        SR_TRACY_ZONE;

        SR_SHADER("Shader::Init() : initialize \"" + GetResourceId().ToStringRef() + "\" shader...");

        RegisterGraphicsResource();

        if (m_isGLayerUsed && !GetPipeline()->IsShaderViewportIndexLayerSupported()) {
            SR_ERROR("Shader::Init() : shader uses gl_Layer but current pipeline does not support it!\n\tPath: {}", GetResourcePath());
            m_hasErrors = true;
            return false;
        }

        for (auto&& [hashName, sampler] : m_samplers) {
            if (sampler.isAttachment || sampler.isArray) {
                continue;
            }
            auto&& pIt = m_defaultSamplers.find(sampler.defaultValue);
            if (pIt != m_defaultSamplers.end()) {
                LoadDefaultSampler(sampler.defaultValue);
                sampler.samplerId = pIt->second->GetId();
            }
            else {
                sampler.samplerId = GetRenderContext()->GetDefaultTexture()->GetId();
            }
        }

        if (m_shaderCreateInfo.shaderType != SRSL2::ShaderType::PostProcessing && GetRenderContext()->IsMacroDefined("SR_DEFINE_WIREFRAME")) {
            m_shaderCreateInfo.polygonMode = PolygonMode::Line;
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

        m_isDirty = false;

        return true;
    }

    ShaderBindResult Shader::Use() noexcept {
        SR_TRACY_ZONE;

        SRAssert2(GetCountUses() > 0, "Shader is not valid!");

        if (m_hasErrors) {
            return ShaderBindResult::Failed;
        }

        if (m_isDirty && !Init()) {
            SR_ERROR("Shader::Use() : failed to initialize shader!");
            return ShaderBindResult::Failed;
        }

        SRAssert(m_shaderProgram != SR_ID_INVALID);

        if (!SRVerify2(GetRenderContext(), "Render context is nullptr!")) {
            return ShaderBindResult::Failed;
        }

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

    void Shader::FreeVMemory() {
        if (m_shaderProgram != SR_ID_INVALID) {
            if (!Memory::ShaderProgramManager::Instance().FreeProgram(&m_shaderProgram)) {
                SR_ERROR("Shader::FreeVMemory() : failed to free shader program! \n\tPath: " + GetResourcePath().ToString());
            }
        }

        if (m_virtualUBO.first != SR_ID_INVALID) {
            if (!m_uboManager.FreeUBO(&m_virtualUBO.first)) {
                SR_ERROR("Shader::FreeVMemory() : failed to free virtual UBO! \n\tPath: " + GetResourcePath().ToString());
            }
        }
        m_virtualUBO.second = true;

        IGraphicsResource::FreeVMemory();
    }

    int32_t Shader::GetId() noexcept {
        if (m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return SR_ID_INVALID;
        }

        if (m_isDirty) SR_UNLIKELY_ATTRIBUTE {
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
    void Shader::SetColor(uint64_t hashId, const SR_MATH_NS::FColor& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetRect(uint64_t hashId, const SR_MATH_NS::FRect& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetVec2(uint64_t hashId, const SR_MATH_NS::FVector2& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetIVec2(uint64_t hashId, const SR_MATH_NS::IVector2& v) noexcept { SetValue<false>(hashId, &v); }
    void Shader::SetIVec3(uint64_t hashId, const SR_MATH_NS::IVector3& v) noexcept { SetValue<false>(hashId, &v); }

    void Shader::SetConstBool(uint64_t hashId, bool v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstFloat(uint64_t hashId, float_t v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstInt(uint64_t hashId, int32_t v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstMat4(uint64_t hashId, const SR_MATH_NS::Matrix4x4& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstVec3(uint64_t hashId, const SR_MATH_NS::FVector3& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstVec4(uint64_t hashId, const SR_MATH_NS::FVector4& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstColor(uint64_t hashId, const SR_MATH_NS::FColor& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstVec2(uint64_t hashId, const SR_MATH_NS::FVector2& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstIVec2(uint64_t hashId, const SR_MATH_NS::IVector2& v) noexcept { SetValue<true>(hashId, &v); }
    void Shader::SetConstIVec3(uint64_t hashId, const SR_MATH_NS::IVector3& v) noexcept { SetValue<true>(hashId, &v); }

    void Shader::SetSampler(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept {
        if (sampler == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            auto&& pSampler = GetRenderContext()->GetNoneTexture();
            if (!pSampler) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("The none texture is nullptr!");
                return;
            }

            sampler = pSampler->GetId();

            if (sampler == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("The none texture id is invalid!");
                return;
            }
        }

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

    void Shader::SetSampler2D(SR_UTILS_NS::StringAtom name, SR_HTYPES_NS::SharedPtr<Texture> pSampler) noexcept {
        if (!IsLoaded() || m_samplers.count(name) == 0) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (!pSampler) SR_UNLIKELY_ATTRIBUTE {
            pSampler = GetRenderContext()->GetNoneTexture();
            if (!pSampler) {
                SRHalt("The none texture is nullptr!");
                return;
            }
        }

        SetSampler(name, pSampler->GetId());
    }

    bool Shader::Ready() const {
        return !m_hasErrors && !m_isDirty && m_shaderProgram != SR_ID_INVALID;
    }

    uint64_t Shader::GetUBOBlockSize() const {
        return m_uniformBlock.m_size;
    }

    bool Shader::Flush() const {
        if (!m_uniformBlock.m_memory) SR_UNLIKELY_ATTRIBUTE {
            if (!m_uniformBlock.m_size) {
                return true; /// no need to flush
            }
            return false;
        }

        auto&& ubo = GetPipeline()->GetCurrentUBO();
        if (ubo != SR_ID_INVALID && m_uniformBlock.Valid()) SR_LIKELY_ATTRIBUTE {
            GetPipeline()->UpdateUBO(ubo, m_uniformBlock.m_memory, m_uniformBlock.m_size, true);
        }

        return true;
    }

    uint32_t Shader::GetSamplersCount() const {
        return m_samplers.size();
    }

    const ShaderProperties& Shader::GetProperties() const {
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

    void Shader::LoadDefaultSampler(SR_UTILS_NS::StringAtom name) {
        if (m_defaultSamplers.count(name) == 1) {
            if (auto&& pTexture = CoreResLoader::Load<SR_GTYPES_NS::Texture>(name)) {
                AddDependency(pTexture.StaticCast<SR_UTILS_NS::ResourceContainer>());
                m_defaultSamplers[name] = pTexture;
            }
            else {
                SR_ERROR("Shader::AddDefaultSampler() : failed to load default sampler! Use none texture. \n\tPath: " + name.ToString());
                m_defaultSamplers[name] = GetRenderContext()->GetNoneTexture();
            }
        }
        else {
            SR_ERROR("Shader::AddDefaultSampler() : default sampler isn't registered! \n\tPath: " + name.ToString());
        }
    }

    void Shader::UnloadDefaultSamplers() {
        for (auto&& [name, pTexture] : m_defaultSamplers) {
            if (!pTexture) {
                continue;
            }
            RemoveDependency(pTexture.StaticCast<SR_UTILS_NS::ResourceContainer>());
        }
        m_defaultSamplers.clear();
    }

    bool Shader::Load() {
        SR_TRACY_ZONE;

        m_isDirty = true;

        const SR_UTILS_NS::Path& path = GetResourcePath();

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("Shader::Load() : loading shader \"{}\"\n\tMacros: {}", path, m_macros.ToString());
        }

        if (path.IsAbs()) {
            SR_ERROR("Shader::Load() : absolute path is not allowed!");
            return false;
        }

        if (auto&& pContext = GetRenderContext()) {
            pContext->SetDirty();
        }

        auto&& cachedPath = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(path).Concat(m_macros.GetHashStr());
        if (ShaderCache::Instance().LoadShaderFromCache(cachedPath, this)) {
            StopWatch();
            StartWatch();
            return IResource::Load();
        }

        auto&& pShader = SR_SRSL_NS::SRSLShader::Load(path, m_macros);
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
        m_isGLayerUsed = pShader->IsGLayerUsed();

        if (m_includes.empty()) {
            SR_ERROR("Shader::Load() : failed to extract includes!\n\tPath: " + path.ToString());
            return false;
        }

        StopWatch();
        StartWatch();

        /// ------------------------------------------------------------------------------------------------------------

        m_computeWorkGroupSize = pShader->GetComputeWorkGroupSize();

        if (auto&& pBlock = pShader->FindUniformBlock("BLOCK")) {
            for (auto&& field : pBlock->fields) {
                m_uniformBlock.Append(field.name.GetHash(), field.size, field.alignedSize, !field.isPublic);

                const ShaderVarType varType = SR_SRSL_NS::SRSLTypeInfo::Instance().StringToType(field.type);

                if (field.isPublic && varType != ShaderVarType::Unknown) {
                    m_properties.emplace_back(ShaderProperty(field.name, varType, false, field.defaultValue));
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
                    m_properties.emplace_back(ShaderProperty(field.name, varType, false, field.defaultValue));
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
                m_properties.emplace_back(ShaderProperty(field.name, varType, true, field.defaultValue));
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
            m_samplers[name].defaultValue = sampler.defaultValue;

            if (!sampler.defaultValue.empty()) {
                m_defaultSamplers.insert(std::make_pair(sampler.defaultValue, nullptr));
            }

            const ShaderVarType varType = SR_SRSL_NS::SRSLTypeInfo::Instance().StringToType(sampler.type);

            if (sampler.isPublic && varType != ShaderVarType::Unknown) {
                m_properties.emplace_back(ShaderProperty(name, varType, false));
            }
        }

        /// ------------------------------------------------------------------------------------------------------------

        for (auto&& property : m_properties) {
            if (!property.HasDefaultData()) {
                continue;
            }
            if (m_uniformSharedBlock.HasField(property.id)) {
                m_uniformSharedBlock.SetDefault(property.id, property.GetDefaultData());
            }
            if (m_uniformBlock.HasField(property.id)) {
                m_uniformBlock.SetDefault(property.id, property.GetDefaultData());
            }
            if (m_constBlock.HasField(property.id)) {
                m_constBlock.SetDefault(property.id, property.GetDefaultData());
            }
        }

        m_uniformBlock.ResetDefaultValues();
        m_constBlock.ResetDefaultValues();

        SR_GRAPH_NS::ShaderCache::Instance().SaveShaderToCache(cachedPath, this);

        return IResource::Load();
    }

    bool Shader::Unload() {
        bool hasErrors = !IResource::Unload();

        m_hasErrors = false;
        m_isDirty = true;

        m_uniformBlock.DeInit();
        m_uniformSharedBlock.DeInit();
        m_constBlock.DeInit();

        m_ssboBindings.clear();
        m_includes.clear();
        m_properties.clear();
        m_samplers.clear();

        UnloadDefaultSamplers();

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
        m_isDirty = true;
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
                GetPipeline()->BindAttachment(samplerInfo.binding, samplerInfo.samplerId);
            }
            else {
                GetPipeline()->BindTexture(samplerInfo.binding, samplerInfo.samplerId);
            }
        }
    }

    void Shader::FlushConstants() {
        if (m_constBlock.m_size > 0) {
            GetPipeline()->PushConstants(m_constBlock.m_memory, m_constBlock.m_size);
        }
    }

    void Shader::StartWatch() {
        SR_TRACY_ZONE;

        auto&& resourcesManager = SR_UTILS_NS::ResourceManager::Instance();

        for (auto&& path : m_includes) {
            auto&& pWatch = SR_UTILS_NS::FileWatcher::MakeShared(resourcesManager.GetResPath().Concat(path));

            pWatch->SetCallBack([this](auto&& pWatcher) {
                SignalWatch();
            });

            m_watchers.emplace_back(pWatch);
        }
    }

    bool Shader::IsSamplersValid() const {
        SR_TRACY_ZONE;
        for (auto&& [hashName, samplerInfo] : m_samplers) {
            if (!samplerInfo.isArray && !samplerInfo.isAttachment) {
                continue;
            }

            if (!GetPipeline()->IsSamplerValid(samplerInfo.samplerId)) {
                return false;
            }
        }

        return true;
    }

    bool Shader::BeginSharedUBO() {
        SR_TRACY_ZONE;

        if (m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        if (m_sharedUBOMode) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("Shared UBO mode is already enabled!");
            return false;
        }

        GetPipeline()->SetCurrentShader(this);

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
        SR_TRACY_ZONE;

        if (!m_sharedUBOMode) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("Shared UBO mode is not enabled!");
            return;
        }

        m_sharedUBOMode = false;

        if (m_uniformSharedBlock.Valid()) SR_LIKELY_ATTRIBUTE {
            auto&& ubo = GetPipeline()->GetCurrentUBO();
            GetPipeline()->UpdateUBO(ubo, m_uniformSharedBlock.m_memory, m_uniformSharedBlock.m_size, true);
        }
    }

    void Shader::SetVariant(const SR_UTILS_NS::IResourceVariant& variant) {
        m_macros = static_cast<const SR_SRSL_NS::ShaderMacrosParams&>(variant);
    }

    bool Shader::AttachDescriptorSets() {
        SR_TRACY_ZONE;

        for (auto&& [hashName, samplerInfo] : m_samplers) {
            if (samplerInfo.samplerId == SR_ID_INVALID) {
                SR_ERROR("Shader::AttachDescriptorSets() : invalid \"{}\" sampler!", hashName);
                samplerInfo.samplerId = GetRenderContext()->GetDefaultTexture()->GetId();
            }

            if (samplerInfo.isAttachment) {
                GetPipeline()->BindAttachment(samplerInfo.binding, samplerInfo.samplerId);
            }
            else {
                GetPipeline()->BindTexture(samplerInfo.binding, samplerInfo.samplerId);
            }
        }

        auto&& descriptorSet = GetPipeline()->GetCurrentDescriptorSet();
        if (descriptorSet == SR_ID_INVALID) {
            return false;
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
            if (m_virtualUBO.second) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("Virtual UBO is not allocated!");
                return false;
            }

            SRDescriptorUpdateInfo updateInfo;
            updateInfo.binding = m_uniformSharedBlock.m_binding;
            updateInfo.ubo = m_uboManager.GetUBO(m_virtualUBO.first);
            updateInfo.descriptorType = DescriptorType::Uniform;

            GetPipeline()->UpdateDescriptorSets(descriptorSet, { updateInfo });
        }

        for (auto&& ssbo : m_ssboBindings) {
            if (ssbo.ssbo == SR_ID_INVALID) {
                SR_ERROR("Shader::AttachDescriptorSets() : invalid \"{}\" SSBO!", ssbo.name.ToStringView());
                return false;
            }
            SRDescriptorUpdateInfo updateInfo;
            updateInfo.binding = ssbo.binding;
            updateInfo.ubo = ssbo.ssbo;
            updateInfo.descriptorType = DescriptorType::Storage;

            GetPipeline()->UpdateDescriptorSets(descriptorSet, { updateInfo });

            ssbo.ssbo = SR_ID_INVALID;
        }

        return true;
    }

    void Shader::ResetUBOToDefaults() {
        m_uniformBlock.ResetDefaultValues();
    }

    void Shader::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        GetPipeline()->Dispatch(x, y, z);
    }

    void Shader::Dispatch() {
        GetPipeline()->Dispatch(m_computeWorkGroupSize.x, m_computeWorkGroupSize.y, m_computeWorkGroupSize.z);
    }

    SR_UTILS_NS::IResource::RemoveUPResult Shader::RemoveUsePoint() {
        if (GetCountUses() == 1 && IsGraphicsResourceRegistered()) {
            SRAssert(m_shaderProgram != SR_ID_INVALID);
        }
        return IResource::RemoveUsePoint();
    }

    const SR_UTILS_NS::IResourceVariant* Shader::GetVariant() const {
        return &m_macros;
    }
}