//
// Created by Monika on 05.09.2025.
//

#include <Graphics/SRSL/Cache.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/Types/Shader.h>

#include <Utils/Types/Marshal.h>
#include <Utils/Common/BaseMarshal.h>

namespace SR_SRSL_NS {
    namespace Details {
        void SaveShaderPropertyVariant(SR_HTYPES_NS::Marshal& marshal, const ShaderPropertyVariant& propertyVariant) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint32_t>(marshal, static_cast<uint32_t>(propertyVariant.index()));
            std::visit([&marshal](ShaderPropertyVariant&& arg) {
                if (std::holds_alternative<int32_t>(arg)) {
                    SR_UTILS_NS::MarshalUtils::SaveValue(marshal, std::get<int32_t>(arg));
                }
                else if (std::holds_alternative<float_t>(arg)) {
                    SR_UTILS_NS::MarshalUtils::SaveValue(marshal, std::get<float_t>(arg));
                }
                else if (std::holds_alternative<SR_MATH_NS::FVector2>(arg)) {
                    SR_UTILS_NS::MarshalUtils::SaveValue(marshal, std::get<SR_MATH_NS::FVector2>(arg));
                }
                else if (std::holds_alternative<SR_MATH_NS::FVector3>(arg)) {
                    SR_UTILS_NS::MarshalUtils::SaveValue(marshal, std::get<SR_MATH_NS::FVector3>(arg));
                }
                else if (std::holds_alternative<SR_MATH_NS::IVector3>(arg)) {
                    SR_UTILS_NS::MarshalUtils::SaveValue(marshal, std::get<SR_MATH_NS::IVector3>(arg));
                }
                else if (std::holds_alternative<SR_MATH_NS::FVector4>(arg)) {
                    SR_UTILS_NS::MarshalUtils::SaveValue(marshal, std::get<SR_MATH_NS::FVector4>(arg));
                }
                else {
                    SRHalt("Unsupported type!");
                }
            }, propertyVariant);
        }

        ShaderPropertyVariant LoadShaderPropertyVariant(SR_HTYPES_NS::Marshal& marshal) {
            const auto index = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
            switch (index) {
                case 1: {
                    return SR_UTILS_NS::MarshalUtils::LoadValue<float_t>(marshal);
                }
                case 2: {
                    return SR_UTILS_NS::MarshalUtils::LoadValue<int32_t>(marshal);
                }
                case 3: {
                    return SR_UTILS_NS::MarshalUtils::LoadValue<SR_MATH_NS::FVector2>(marshal);
                }
                case 4: {
                    return SR_UTILS_NS::MarshalUtils::LoadValue<SR_MATH_NS::FVector3>(marshal);
                }
                case 5: {
                    return SR_UTILS_NS::MarshalUtils::LoadValue<SR_MATH_NS::IVector3>(marshal);
                }
                case 6: {
                    return SR_UTILS_NS::MarshalUtils::LoadValue<SR_MATH_NS::FVector4>(marshal);
                }
                default: {
                    SRHalt("Unsupported type!");
                    return {};
                }
            }
        }

        void SaveStages(SR_HTYPES_NS::Marshal& marshal, const SR_UTILS_NS::Set<ShaderStage>& stages) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, stages.size());
            for (const auto& stage : stages) {
                SR_UTILS_NS::MarshalUtils::SaveValue<uint32_t>(marshal, static_cast<uint32_t>(stage));
            }
        }

        void SaveUniformBlock(SR_HTYPES_NS::Marshal& marshal, const SRSLUniformBlock& block) {
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.size);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.binding);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.hasUsage);

            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.isVolatile);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.isCoherent);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.isRestrict);

            if (block.isReadOnly) {
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, true);
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, *block.isReadOnly);
            }
            else {
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, false);
            }

            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, block.fields.size());
            for (const auto& field : block.fields) {
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, field.type);
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, field.name);
                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, field.size);
                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, field.alignedSize);
                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, field.isPublic);

                if (field.defaultValue) {
                    SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, true);
                    SaveShaderPropertyVariant(marshal, *field.defaultValue);
                }
                else {
                    SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, false);
                }
            }

            SaveStages(marshal, block.stages);
        }

        void SaveUniforms(SR_HTYPES_NS::Marshal& marshal, const SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SRSLUniformBlock>& uniforms) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, uniforms.size());
            for (const auto& [name, block] : uniforms) {
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, name);
                SaveUniformBlock(marshal, block);
            }
        }

        void SaveSamplers(SR_HTYPES_NS::Marshal& marshal, const SRSLSamplers& samplers) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, samplers.size());

            for (const auto& [name, sampler] : samplers) {
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, name);

                SR_UTILS_NS::MarshalUtils::SaveString(marshal, sampler.type);
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, sampler.defaultValue);

                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, sampler.isPublic);
                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, sampler.binding);
                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, sampler.attachment);

                SaveStages(marshal, sampler.stages);
            }
        }

        void SaveSamplers(SR_HTYPES_NS::Marshal& marshal, const ShaderSamplers& samplers) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, samplers.size());

            for (const auto& [name, sampler] : samplers) {
                /// ignore sampler.samplerId

                SR_UTILS_NS::MarshalUtils::SaveString(marshal, name);

                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, sampler.binding);
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, sampler.isArray);
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, sampler.isAttachment);

                SR_UTILS_NS::MarshalUtils::SaveString(marshal, sampler.defaultValue);
            }
        }

        ShaderSamplers LoadSamplers(SR_HTYPES_NS::Marshal& marshal) {
            ShaderSamplers samplers;

            const auto samplersSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
            for (size_t i = 0; i < samplersSize; ++i) {
                std::string name = SR_UTILS_NS::MarshalUtils::LoadString(marshal);

                ShaderSampler sampler;
                sampler.binding = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
                sampler.isArray = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
                sampler.isAttachment = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
                sampler.defaultValue = SR_UTILS_NS::MarshalUtils::LoadString(marshal);

                samplers[name] = sampler;
            }

            return samplers;
        }

        void SaveSSBO(SR_HTYPES_NS::Marshal& marshal, const SSBOBindings& ssbo) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, ssbo.size());

            for (const auto& item : ssbo) {
                /// ignore item.ssbo
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, item.name);
                SR_UTILS_NS::MarshalUtils::SaveValue(marshal, item.binding);
            }
        }

        SSBOBindings LoadSSBO(SR_HTYPES_NS::Marshal& marshal) {
            SSBOBindings ssbo;

            const auto ssboSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
            for (size_t i = 0; i < ssboSize; ++i) {
                SSBOBinding binding;
                binding.name = SR_UTILS_NS::MarshalUtils::LoadString(marshal);
                binding.binding = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
                /// ignore binding.ssbo

                ssbo.push_back(binding);
            }

            return ssbo;
        }

        void SaveVertexLayoutDescription(SR_HTYPES_NS::Marshal& marshal, const SR_UTILS_NS::VertexLayoutDescription& description) {
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, description.attributesCount);
            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, description.stride);
            SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, description.instanced);
            for (size_t i = 0; i < description.attributesCount; ++i) {
                const auto& attribute = description.attributes[i];
                SR_UTILS_NS::MarshalUtils::SaveValue<uint32_t>(marshal, static_cast<uint32_t>(attribute.attribute));
                SR_UTILS_NS::MarshalUtils::SaveValue<uint8_t>(marshal, static_cast<uint32_t>(attribute.format));
                SR_UTILS_NS::MarshalUtils::SaveValue<uint8_t>(marshal, attribute.count);
                SR_UTILS_NS::MarshalUtils::SaveValue<uint16_t>(marshal, attribute.offset);
            }
        }

        SR_UTILS_NS::VertexLayoutDescription LoadVertexLayoutDescription(SR_HTYPES_NS::Marshal& marshal) {
            SR_UTILS_NS::VertexLayoutDescription description;

            description.attributesCount = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
            description.stride = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
            description.instanced = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
            for (size_t i = 0; i < description.attributesCount; ++i) {
                SR_UTILS_NS::VertexAttributeDescription attribute;
                attribute.attribute = static_cast<SR_UTILS_NS::VertexAttribute>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
                attribute.format = static_cast<SR_UTILS_NS::VertexAttributeFormat>(SR_UTILS_NS::MarshalUtils::LoadValue<uint8_t>(marshal));
                attribute.count = SR_UTILS_NS::MarshalUtils::LoadValue<uint8_t>(marshal);
                attribute.offset = SR_UTILS_NS::MarshalUtils::LoadValue<uint16_t>(marshal);
                description.attributes[i] = attribute;
            }

            return description;
        }

        void SaveCreateInfo(SR_HTYPES_NS::Marshal& marshal, const SRShaderCreateInfo& createInfo) {
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(createInfo.shaderType));
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(createInfo.polygonMode));
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(createInfo.cullMode));
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(createInfo.depthCompare));
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(createInfo.primitiveTopology));
            SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, createInfo.blendEnabled);
            SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, createInfo.depthWrite);
            SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, createInfo.depthTest);
            SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, createInfo.alphaCoverage);

            SR_UTILS_NS::MarshalUtils::SaveVector(marshal, createInfo.uniforms);

            if (createInfo.stages.empty()) {
                SRHalt("SRShaderCreateInfo::SaveCreateInfo() : stages size is 0!");
            }

            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, createInfo.stages.size());
            for (const auto &[stage, info]: createInfo.stages) {
                SR_UTILS_NS::MarshalUtils::SaveValue<uint32_t>(marshal, static_cast<uint32_t>(stage));
                SR_UTILS_NS::MarshalUtils::SaveString(marshal, info.path.ToStringView());
                SR_UTILS_NS::MarshalUtils::SaveVector(marshal, info.pushConstants);
            }

            SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, createInfo.vertexLayoutDescriptions.GetLayoutsCount());
            for (auto&& layout : createInfo.vertexLayoutDescriptions.GetLayouts()) {
                SaveVertexLayoutDescription(marshal, layout);
            }
        }

        SRShaderCreateInfo LoadCreateInfo(SR_HTYPES_NS::Marshal& marshal) {
            SRShaderCreateInfo createInfo;

            createInfo.shaderType = static_cast<ShaderType>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
            createInfo.polygonMode = static_cast<PolygonMode>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
            createInfo.cullMode = static_cast<CullMode>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
            createInfo.depthCompare = static_cast<DepthCompare>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
            createInfo.primitiveTopology = static_cast<PrimitiveTopology>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
            createInfo.blendEnabled = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
            createInfo.depthWrite = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
            createInfo.depthTest = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
            createInfo.alphaCoverage = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);

            createInfo.uniforms = SR_UTILS_NS::MarshalUtils::LoadVector<UBOInfo>(marshal);

            const auto stagesSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
            if (stagesSize == 0) {
                SRHalt("SRShaderCreateInfo::LoadCreateInfo() : stages size is 0!");
            }

            for (size_t i = 0; i < stagesSize; ++i) {
                const auto stage = static_cast<ShaderStage>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
                SRShaderStageInfo info;
                info.path = SR_UTILS_NS::Path(SR_UTILS_NS::MarshalUtils::LoadString(marshal));
                info.pushConstants = SR_UTILS_NS::MarshalUtils::LoadVector<SR_UTILS_NS::Vector<SRShaderPushConstant>>(marshal);
                createInfo.stages[stage] = info;
            }

            const auto layoutsCount = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
            createInfo.vertexLayoutDescriptions.layouts.resize(layoutsCount);
            for (size_t i = 0; i < layoutsCount; ++i) {
                createInfo.vertexLayoutDescriptions.layouts[i] = LoadVertexLayoutDescription(marshal);
            }

            return createInfo;
        }
    }
}

namespace SR_GRAPH_NS {
    void ShaderCache::SaveShaderToCache(const SR_UTILS_NS::Path& cachePath, const SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_TEXT(cachePath);

        if (!pShader->m_shaderCreateInfo.Validate()) {
            SRHalt("Invalid shader create info! Path: {}", cachePath);
            return;
        }

        uint64_t hash = 0;

        for (auto&& include : pShader->m_includes) {
            auto&& absPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(include.name);
            hash = SR_UTILS_NS::CombineTwoHashes(hash, absPath.GetFileHash());
        }

        SR_HTYPES_NS::Marshal marshal;
        marshal.Reserve(1024 * 64); // 64 KB

        SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, GetVersion()); // version
        SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, hash);

        SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, pShader->m_includes.size());
        for (auto&& include : pShader->m_includes) {
            SR_UTILS_NS::MarshalUtils::SaveString(marshal, include.name);
        }

        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(pShader->m_type));

        SaveUBOBlock(marshal, pShader->m_uniformBlock);
        SaveUBOBlock(marshal, pShader->m_uniformSharedBlock);
        SaveUBOBlock(marshal, pShader->m_constBlock);

        SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, pShader->m_defaultSamplers.size());
        for (auto&& [sampler, _] : pShader->m_defaultSamplers) {
            SR_UTILS_NS::MarshalUtils::SaveString(marshal, sampler);
        }

        SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, pShader->m_properties.size());
        for (const auto& property : pShader->m_properties) {
            SR_UTILS_NS::MarshalUtils::SaveString(marshal, property.id);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, static_cast<uint32_t>(property.type));
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, property.pushConstant);

            if (property.defaultData) {
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, true);
                SR_SRSL_NS::Details::SaveShaderPropertyVariant(marshal, *property.defaultData);
            }
            else {
                SR_UTILS_NS::MarshalUtils::SaveValue<bool>(marshal, false);
            }
        }

        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, pShader->m_computeWorkGroupSize);
        SR_SRSL_NS::Details::SaveCreateInfo(marshal, pShader->m_shaderCreateInfo);
        SR_SRSL_NS::Details::SaveSamplers(marshal, pShader->m_samplers);
        SR_SRSL_NS::Details::SaveSSBO(marshal, pShader->m_ssboBindings);

        marshal.Write<bool>(pShader->m_isGLayerUsed);

        auto&& cacheFile = cachePath.ConcatExt("cache");

        if (cacheFile.IsFile()) {
            SR_PLATFORM_NS::Delete(cacheFile);
        }

        if (!marshal.Save(cacheFile)) {
            SR_ERROR("SRSLShaderCache::SaveShaderToCache() : failed to save shader to cache! Path: {}", cacheFile);
        }
    }

    bool ShaderCache::LoadShaderFromCache(const SR_UTILS_NS::Path& cachePath, SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_TEXT(cachePath);

        auto&& cacheFile = cachePath.ConcatExt("cache");
        if (!cacheFile.IsFile()) {
            return false;
        }

        SR_HTYPES_NS::Marshal marshal = SR_HTYPES_NS::Marshal::Load(cacheFile);
        if (!marshal.Valid()) {
            return false;
        }

        const auto version = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
        if (version != GetVersion()) {
            return false;
        }

        const auto hash = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
        uint64_t currentHash = 0;

        SR_UTILS_NS::Vector<SR_SRSL_NS::SRSLInclude> includes;
        const auto includesSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
        for (uint64_t i = 0; i < includesSize; ++i) {
            SR_SRSL_NS::SRSLInclude& inc = includes.emplace_back();
            inc.name = SR_UTILS_NS::MarshalUtils::LoadStrAtom(marshal);
            auto&& absPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(inc.name);
            currentHash = SR_UTILS_NS::CombineTwoHashes(currentHash, absPath.GetFileHash());
        }

        if (hash != currentHash) {
            return false;
        }

        pShader->m_includes = std::move(includes);
        pShader->m_type = static_cast<SR_SRSL_NS::ShaderType>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));

        LoadUBOBlock(marshal, pShader->m_uniformBlock);
        LoadUBOBlock(marshal, pShader->m_uniformSharedBlock);
        LoadUBOBlock(marshal, pShader->m_constBlock);

        pShader->m_defaultSamplers.clear();
        const auto defaultSamplersSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
        for (uint64_t i = 0; i < defaultSamplersSize; ++i) {
            pShader->m_defaultSamplers[SR_UTILS_NS::MarshalUtils::LoadStrAtom(marshal)];
        }

        pShader->m_properties.clear();
        const auto propertiesSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
        for (uint64_t i = 0; i < propertiesSize; ++i) {
            ShaderProperty property;
            property.id = SR_UTILS_NS::MarshalUtils::LoadStrAtom(marshal);
            property.type = static_cast<ShaderVarType>(SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal));
            property.pushConstant = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);

            const auto hasDefaultData = SR_UTILS_NS::MarshalUtils::LoadValue<bool>(marshal);
            if (hasDefaultData) {
                property.defaultData = SR_SRSL_NS::Details::LoadShaderPropertyVariant(marshal);
            }

            pShader->m_properties.emplace_back(property);
        }

        pShader->m_computeWorkGroupSize = SR_UTILS_NS::MarshalUtils::LoadValue<SR_MATH_NS::UVector3>(marshal);
        pShader->m_shaderCreateInfo = SR_SRSL_NS::Details::LoadCreateInfo(marshal);
        pShader->m_samplers = SR_SRSL_NS::Details::LoadSamplers(marshal);
        pShader->m_ssboBindings = SR_SRSL_NS::Details::LoadSSBO(marshal);

        pShader->m_isGLayerUsed = marshal.Read<bool>();

        if (!pShader->m_shaderCreateInfo.Validate()) {
            SRHalt("Invalid shader create info loaded from cache!");
            return false;
        }

        return true;
    }

    void ShaderCache::SaveUBOBlock(SR_HTYPES_NS::Marshal& marshal, const Memory::ShaderUBOBlock& block) {
        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.m_alignedBlock);
        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.m_align);
        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.m_binding);
        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.m_size);

        if (block.m_memory && block.m_size > 0) {
            marshal.Write<bool>(true);
            marshal.WriteBlock(block.m_memory, block.m_size);
        }
        else {
            marshal.Write<bool>(false);
        }

        SR_UTILS_NS::MarshalUtils::SaveValue<uint64_t>(marshal, block.m_defaultValues.size());
        for (const auto& defaultValue : block.m_defaultValues) {
            SR_UTILS_NS::MarshalUtils::SaveString(marshal, defaultValue.name);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, defaultValue.size);
            SR_UTILS_NS::MarshalUtils::SaveValue(marshal, defaultValue.offset);
            SR_SRSL_NS::Details::SaveShaderPropertyVariant(marshal, defaultValue.value);
        }

        SR_UTILS_NS::MarshalUtils::SaveValue(marshal, block.m_dataCount);
        if (block.m_dataCount > 0) {
            marshal.WriteBlock(block.m_data, sizeof(Memory::ShaderUBOBlock::SubBlock) * block.m_dataCount);
        }
    }

    void ShaderCache::LoadUBOBlock(SR_HTYPES_NS::Marshal& marshal, Memory::ShaderUBOBlock& block) {
        block.m_alignedBlock = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
        block.m_align = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
        block.m_binding = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
        block.m_size = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);

        if (marshal.Read<bool>()) {
            SRAssert(!block.m_memory);
            SRAssert(block.m_size > 0);
            block.m_memory = block.AllocMemory(block.m_size);
            marshal.ReadBlock(block.m_memory);
        }
        else {
            block.m_memory = nullptr;
        }

        block.m_initialized = true;

        const auto defaultValuesSize = SR_UTILS_NS::MarshalUtils::LoadValue<uint64_t>(marshal);
        for (size_t i = 0; i < defaultValuesSize; ++i) {
            Memory::ShaderUBOBlock::DefaultValue defaultValue;
            defaultValue.name = SR_UTILS_NS::MarshalUtils::LoadStrAtom(marshal);
            defaultValue.size = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
            defaultValue.offset = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
            defaultValue.value = SR_SRSL_NS::Details::LoadShaderPropertyVariant(marshal);
            block.m_defaultValues.push_back(defaultValue);
        }

        SRAssert(!block.m_data);

        block.m_dataCount = SR_UTILS_NS::MarshalUtils::LoadValue<uint32_t>(marshal);
        if (block.m_dataCount > 0) {
            block.m_data = new Memory::ShaderUBOBlock::SubBlock[block.m_dataCount];
            marshal.ReadBlock(block.m_data);
        }
    }

    uint64_t ShaderCache::GetVersion() {
        return 0xBAD8F00D;
    }
}