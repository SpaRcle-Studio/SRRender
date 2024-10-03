//
// Created by Monika on 14.08.2024.
//

#include <Graphics/Render/HTMLRenderer.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Window/Window.h>

#include <Utils/Profile/TracyContext.h>

namespace SR_GRAPH_NS {
    SR_UTILS_NS::StringAtom SR_SOLID_FILL_SHADER = "solid-fill"_atom;

    HTMLRendererBase::HTMLRendererBase()
        : Super()
        , m_uboManager(SR_GRAPH_NS::Memory::UBOManager::Instance())
        , m_descriptorManager(SR_GRAPH_NS::DescriptorManager::Instance())
    { }

    bool HTMLRendererBase::Init() {
        if (auto&& pShader = SR_GTYPES_NS::Shader::Load("Engine/Shaders/Web/" + SR_SOLID_FILL_SHADER.ToString() + ".srsl")) {
            pShader->AddUsePoint();
            ShaderInfo shaderInfo;
            shaderInfo.pShader = pShader;
            m_shaders[SR_SOLID_FILL_SHADER] = shaderInfo;
        }
        else {
            SR_ERROR("HTMLRendererBase::Init() : failed to load shader \"{}\"!", SR_SOLID_FILL_SHADER.c_str());
            return false;
        }

        return true;
    }

    void HTMLRendererBase::DeInit() {
        for (auto&& [id, shaderInfo] : m_shaders) {
            shaderInfo.pShader->RemoveUsePoint();

            for (auto&& memInfo : shaderInfo.UBOs) {
                if (memInfo.virtualUBO != SR_ID_INVALID && !m_uboManager.FreeUBO(&memInfo.virtualUBO)) {
                    SR_ERROR("HTMLRendererBase::DeInit() : failed to free uniform buffer object!");
                }

                if (memInfo.virtualDescriptor != SR_ID_INVALID && !m_descriptorManager.FreeDescriptorSet(&memInfo.virtualDescriptor)) {
                    SR_ERROR("HTMLRendererBase::DeInit() : failed to free descriptor set!");
                }
            }
        }

        m_shaders.clear();
    }

    void HTMLRendererBase::Draw() {
        SR_TRACY_ZONE;

        for (auto&& [id, shaderInfo] : m_shaders) {
            if (shaderInfo.pShader->HasErrors()) {
                SR_ERROR("HTMLRendererBase::Draw() : shader \"{}\" has errors!", id.c_str());
                return;
            }
            shaderInfo.index = 0;
        }

        if (SRVerify2(GetPage(), "HTMLRendererBase::Draw() : page is not set!")) {
            litehtml::position clip;
            get_client_rect(clip);
            m_viewSize = SR_MATH_NS::FVector2(clip.width, clip.height);
            GetPage()->GetDocument()->draw(reinterpret_cast<litehtml::uint_ptr>(this), 0, 0, &clip);
        }
    }

    void HTMLRendererBase::Update() {
        SR_TRACY_ZONE;

        for (auto&& [id, shaderInfo] : m_shaders) {
            if (shaderInfo.pShader->BeginSharedUBO()) {
                if (m_pCamera) {
                    shaderInfo.pShader->SetMat4(SHADER_ORTHOGONAL_MATRIX, m_pCamera->GetOrthogonal());
                }
                else {
                    SR_ERROR("HTMLRendererBase::Update() : no camera!");
                }

                shaderInfo.pShader->SetVec2(SHADER_RESOLUTION, m_viewSize);
                shaderInfo.pShader->EndSharedUBO();
            }
        }
    }

    void HTMLRendererBase::get_media_features(litehtml::media_features& media) const {
        SR_TRACY_ZONE;

        auto&& resolution = SR_PLATFORM_NS::GetScreenResolution().Cast<int32_t>();

        litehtml::position client;
        get_client_rect(client);
        media.type			= litehtml::media_type_screen;
        media.width			= client.width;
        media.height		= client.height;
        media.device_width	= resolution.x;
        media.device_height	= resolution.y;
        media.color			= 8;
        media.monochrome	= 0;
        media.color_index	= 256;
        media.resolution	= 96;
    }

    void HTMLRendererBase::get_client_rect(litehtml::position& client) const {
        SR_TRACY_ZONE;

        client.x = 0;
        client.y = 0;

        if (m_pCamera) {
            client.width = m_pCamera->GetSize().Cast<int32_t>().x;
            client.height = m_pCamera->GetSize().Cast<int32_t>().y;
        }
        else if (m_pipeline) {
            client.width = m_pipeline->GetWindow()->GetSize().Cast<int32_t>().x;
            client.height = m_pipeline->GetWindow()->GetSize().Cast<int32_t>().y;
        }
        else {
            SRHalt("HTMLRendererBase::get_client_rect() : no camera or pipeline!");
        }
    }

    litehtml::uint_ptr HTMLRendererBase::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
        return 0;
    }

    void HTMLRendererBase::delete_font(litehtml::uint_ptr hFont) {

    }

    void HTMLRendererBase::draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) {
        SR_TRACY_ZONE;

        if (color == litehtml::web_color::transparent) {
            return;
        }

        auto&& pIt = m_shaders.find(SR_SOLID_FILL_SHADER);
        if (pIt == m_shaders.end()) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("HTMLRendererBase::DrawElement() : shader \"{}\" not found!", SR_SOLID_FILL_SHADER.c_str());
            return;
        }
        ShaderInfo& shaderInfo = pIt->second;

        if (!BeginElement(shaderInfo)) {
            return;
        }

        DrawElement(shaderInfo);

        auto&& pShader = m_pipeline->GetCurrentShader();
        const auto& box = layer.clip_box;

        SR_MATH_NS::FVector2 position = SR_MATH_NS::FVector2(box.x, box.y) * 2.f;
        position.x = position.x + box.width;
        position.y = -position.y - box.height;

        pShader->SetVec2("position"_atom_hash_cexpr, SR_MATH_NS::FVector2(-1, 1) + position / m_viewSize);
        pShader->SetVec2("size"_atom_hash_cexpr, SR_MATH_NS::FVector2(box.width, box.height) / m_viewSize);
        pShader->SetVec4("color"_atom_hash_cexpr, SR_MATH_NS::FColor(color.red, color.green, color.blue, color.alpha) / 255.f);

        UpdateElement(shaderInfo);
        EndElement(shaderInfo);
    }

    bool HTMLRendererBase::BeginElement(ShaderInfo& shaderInfo) {
        if (m_pipeline->GetCurrentShader() != shaderInfo.pShader) {
            const auto result = shaderInfo.pShader->Use();
            if (result == ShaderBindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("HTMLRendererBase::BeginElement() : failed to use shader \"{}\"!", shaderInfo.pShader->GetResourceId().c_str());
                return false;
            }
        }

        return true;
    }

    void HTMLRendererBase::UpdateElement(ShaderInfo& shaderInfo) {
        auto&& pShader = m_pipeline->GetCurrentShader();
        if (!pShader->Flush()) {
            SR_ERROR("HTMLRendererBase::UpdateElement() : failed to flush shader \"{}\"!", pShader->GetResourceId().c_str());
        }
    }

    void HTMLRendererBase::EndElement(ShaderInfo& shaderInfo) {
        ++shaderInfo.index;
    }

    void HTMLRendererBase::DrawElement(ShaderInfo &shaderInfo) {
        if (shaderInfo.index >= shaderInfo.UBOs.size()) SR_UNLIKELY_ATTRIBUTE {
            ShaderInfo::MemInfo memInfo;

            memInfo.virtualUBO = m_uboManager.AllocateUBO(SR_ID_INVALID);
            if (memInfo.virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("HTMLRendererBase::DrawElement() : failed to allocate uniform buffer object!");
                return;
            }

            memInfo.virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(SR_ID_INVALID);
            if (memInfo.virtualDescriptor == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("HTMLRendererBase::DrawElement() : failed to allocate descriptor set!");
                return;
            }
            shaderInfo.UBOs.emplace_back(memInfo);
        }

        m_uboManager.BindUBO(shaderInfo.UBOs[shaderInfo.index].virtualUBO);

        const auto result = m_descriptorManager.Bind(shaderInfo.UBOs[shaderInfo.index].virtualDescriptor);

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            shaderInfo.pShader->FlushSamplers();
            m_descriptorManager.Flush();
            m_pipeline->GetCurrentShader()->FlushConstants();
        }

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            m_pipeline->Draw(4);
        }
    }
}
