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
    SR_UTILS_NS::StringAtom SR_TEXT_SHADER = "text"_atom;

    std::vector<SR_UTILS_NS::StringAtom> SR_HTML_SHADERS = {
        SR_SOLID_FILL_SHADER, SR_TEXT_SHADER
    };

    HTMLRenderContainer::HTMLRenderContainer()
        : Super()
        , m_uboManager(SR_GRAPH_NS::Memory::UBOManager::Instance())
        , m_descriptorManager(SR_GRAPH_NS::DescriptorManager::Instance())
    { }

    HTMLRenderContainer::~HTMLRenderContainer() {
        SRAssert2(m_textAtlases.empty(), "Text atlases are not empty!");
        SRAssert2(m_textBuilders.empty(), "Text builders are not empty!");
        SRAssert2(m_shaders.empty(), "Shaders are not empty!");
    }

    bool HTMLRenderContainer::Init() {
        for (auto&& shaderId : SR_HTML_SHADERS) {
            if (auto&& pShader = SR_GTYPES_NS::Shader::Load("Engine/Shaders/Web/" + shaderId.ToString() + ".srsl")) {
                pShader->AddUsePoint();
                ShaderInfo shaderInfo;
                shaderInfo.pShader = pShader;
                m_shaders[shaderId] = shaderInfo;
            }
            else {
                SR_ERROR("HTMLRenderContainer::Init() : failed to load shader \"{}\"!", shaderId.c_str());
                return false;
            }
        }

        return true;
    }

    void HTMLRenderContainer::DeInit() {
        ClearTextAtlases();

        for (auto& shaderInfo : m_shaders | std::views::values) {
            shaderInfo.pShader->RemoveUsePoint();

            for (auto&& memInfo : shaderInfo.UBOs) {
                if (memInfo.virtualUBO != SR_ID_INVALID && !m_uboManager.FreeUBO(&memInfo.virtualUBO)) {
                    SR_ERROR("HTMLRenderContainer::DeInit() : failed to free uniform buffer object!");
                }

                if (memInfo.virtualDescriptor != SR_ID_INVALID && !m_descriptorManager.FreeDescriptorSet(&memInfo.virtualDescriptor)) {
                    SR_ERROR("HTMLRenderContainer::DeInit() : failed to free descriptor set!");
                }
            }
        }
        m_shaders.clear();

        /// INFO: чистит сам документ через delete_font
        /// for (auto& textBuilderInfo : m_textBuilders) {
        ///     delete textBuilderInfo.pTextBuilder;
        /// }
        /// m_textBuilders.clear();
    }

    void HTMLRenderContainer::Draw() {
        SR_TRACY_ZONE;

        m_isRendered = true;

        ClearTextAtlases();

        for (auto&& [id, shaderInfo] : m_shaders) {
            if (shaderInfo.pShader->HasErrors()) {
                SR_ERROR("HTMLRenderContainer::Draw() : shader \"{}\" has errors!", id.c_str());
                return;
            }
            shaderInfo.index = 0;
        }

        if (SRVerify2(GetPage(), "HTMLRenderContainer::Draw() : page is not set!")) {
            litehtml::position clip;
            get_client_rect(clip);
            m_viewSize = SR_MATH_NS::FVector2(clip.width, clip.height);
            GetPage()->GetDocument()->draw(reinterpret_cast<litehtml::uint_ptr>(this), m_scroll.x, m_scroll.y, &clip);
        }
    }

    void HTMLRenderContainer::Update() {
        SR_TRACY_ZONE;

        if (!m_isRendered) {
            return;
        }

        if (SR_UTILS_NS::Input::Instance().GetKeyDown(SR_UTILS_NS::KeyCode::Tilde)) {
            m_scroll = SR_MATH_NS::IVector2(0, 0);
            m_pipeline->SetDirty(true);
        }

        if (SR_UTILS_NS::Input::Instance().GetKey(SR_UTILS_NS::KeyCode::DownArrow)) {
            m_scroll.y -= 10;
            m_pipeline->SetDirty(true);
        }

        if (SR_UTILS_NS::Input::Instance().GetKey(SR_UTILS_NS::KeyCode::UpArrow)) {
            m_scroll.y += 10;
            m_pipeline->SetDirty(true);
        }

        if (SR_UTILS_NS::Input::Instance().GetKey(SR_UTILS_NS::KeyCode::LeftArrow)) {
            m_scroll.x += 10;
            m_pipeline->SetDirty(true);
        }

        if (SR_UTILS_NS::Input::Instance().GetKey(SR_UTILS_NS::KeyCode::RightArrow)) {
            m_scroll.x -= 10;
            m_pipeline->SetDirty(true);
        }

        for (auto&& [id, shaderInfo] : m_shaders) {
            if (!shaderInfo.pShader->Ready()) {
                continue;
            }

            if (shaderInfo.pShader->BeginSharedUBO()) {
                if (m_pCamera) {
                    shaderInfo.pShader->SetMat4(SHADER_ORTHOGONAL_MATRIX, m_pCamera->GetOrthogonal());
                }
                else {
                    SR_ERROR("HTMLRenderContainer::Update() : no camera!");
                }

                shaderInfo.pShader->SetVec2(SHADER_RESOLUTION, m_viewSize);
                shaderInfo.pShader->EndSharedUBO();
            }
        }
    }

    void HTMLRenderContainer::get_media_features(litehtml::media_features& media) const {
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

    void HTMLRenderContainer::get_client_rect(litehtml::position& client) const {
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
            SRHalt("HTMLRenderContainer::get_client_rect() : no camera or pipeline!");
        }
    }

    litehtml::uint_ptr HTMLRenderContainer::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
        auto&& pIt = std::find_if(m_textBuilders.begin(), m_textBuilders.end(), [faceName, size](const TextBuilderInfo& info) {
            return info.fontName == faceName && info.pTextBuilder->GetFontSize() == size;
        });
        if (pIt != m_textBuilders.end()) {
            return reinterpret_cast<litehtml::uint_ptr>(pIt->pTextBuilder);
        }

        std::vector<std::string_view> fonts = SR_UTILS_NS::StringUtils::SplitView(faceName, ",");

        std::vector<SR_UTILS_NS::Path> candidates;

        for (auto&& font : fonts) {
            candidates.emplace_back(SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(font).ConcatExt(".ttf"));
            candidates.emplace_back(SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Fonts").Concat(font).ConcatExt(".ttf"));
            candidates.emplace_back(SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(font).ConcatExt(".ttf"));
        }

        SR_UTILS_NS::Path fullPath;
        for (auto&& candidate : candidates) {
            if (candidate.IsFile()) {
                fullPath = candidate;
                break;
            }
        }

        if (!fullPath.IsFile()) {
            SR_ERROR("HTMLRenderContainer::create_font() : font \"{}\" not found!", faceName);
            return 0;
        }

        auto&& pFont = SR_GTYPES_NS::Font::Load(fullPath);
        if (!pFont) {
            SR_ERROR("HTMLRenderContainer::create_font() : failed to load font \"{}\"!", fullPath.c_str());
            return 0;
        }

        auto&& pTextBuilder = new TextBuilder(pFont);

        pTextBuilder->SetFontSize(size);
        pTextBuilder->SetKerning(true);

        TextBuilderInfo textBuilderInfo;
        textBuilderInfo.pTextBuilder = pTextBuilder;
        textBuilderInfo.fontName = SR_UTILS_NS::StringAtom(faceName);

        m_textBuilders.emplace_back(textBuilderInfo);

        return reinterpret_cast<litehtml::uint_ptr>(pTextBuilder);
    }

    void HTMLRenderContainer::delete_font(litehtml::uint_ptr hFont) {
        auto&& pTextBuilder = reinterpret_cast<TextBuilder*>(hFont);
        if (!pTextBuilder) {
            return;
        }

        for (auto&& pIt = m_textBuilders.begin(); pIt != m_textBuilders.end(); ++pIt) {
            if (pIt->pTextBuilder == pTextBuilder) {
                delete pTextBuilder;
                m_textBuilders.erase(pIt);
                return;
            }
        }

        SR_ERROR("HTMLRenderContainer::delete_font() : font not found!");
    }

    int32_t HTMLRenderContainer::text_width(const char* text, litehtml::uint_ptr hFont) {
        TextBuilder* pTextBuilder = reinterpret_cast<TextBuilder*>(hFont);
        if (!pTextBuilder) {
            return 0;
        }

        return pTextBuilder->CalculateTextWidth(text);
    }

    void HTMLRenderContainer::draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer& layer, const litehtml::web_color& color) {
        SR_TRACY_ZONE;

        if (color == litehtml::web_color::transparent) {
            return;
        }

        auto&& pIt = m_shaders.find(SR_SOLID_FILL_SHADER);
        if (pIt == m_shaders.end()) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("HTMLRenderContainer::DrawElement() : shader \"{}\" not found!", SR_SOLID_FILL_SHADER.c_str());
            return;
        }
        ShaderInfo& shaderInfo = pIt->second;

        if (!BeginElement(shaderInfo)) {
            return;
        }

        auto&& pShader = m_pipeline->GetCurrentShader();
        const auto& box = layer.clip_box;

        SR_MATH_NS::FVector2 position = SR_MATH_NS::FVector2(box.x, box.y) * 2.f;
        position.x = position.x + box.width;
        position.y = -position.y - box.height;

        pShader->SetVec2("position"_atom_hash_cexpr, SR_MATH_NS::FVector2(-1, 1) + position / m_viewSize);
        pShader->SetVec2("size"_atom_hash_cexpr, SR_MATH_NS::FVector2(box.width, box.height) / m_viewSize);
        pShader->SetVec4("color"_atom_hash_cexpr, SR_MATH_NS::FColor(color.red, color.green, color.blue, color.alpha) / 255.f);
        DrawElement(shaderInfo);
        UpdateElement(shaderInfo);
        EndElement(shaderInfo);
    }

    void HTMLRenderContainer::draw_text(litehtml::uint_ptr, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {
        SR_TRACY_ZONE;

        if (color == litehtml::web_color::transparent) {
            return;
        }

        auto&& pIt = m_shaders.find(SR_TEXT_SHADER);
        if (pIt == m_shaders.end()) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("HTMLRenderContainer::DrawElement() : shader \"{}\" not found!", SR_TEXT_SHADER.c_str());
            return;
        }
        ShaderInfo& shaderInfo = pIt->second;

        auto&& pTextAtlas = GetTextAtlas(text, reinterpret_cast<TextBuilder*>(hFont));
        if (!pTextAtlas) {
            return;
        }

        if (!BeginElement(shaderInfo)) {
            return;
        }

        auto&& pShader = m_pipeline->GetCurrentShader();

        litehtml::position box = pos;
        box.height += pTextAtlas->pTextBuilderRef->GetHeight();

        SR_MATH_NS::FVector2 position = SR_MATH_NS::FVector2(box.x, box.y) * 2.f;
        position.x = position.x + box.width;
        position.y = -position.y - box.height;

        pShader->SetVec2("position"_atom_hash_cexpr, SR_MATH_NS::FVector2(-1, 1) + position / m_viewSize);
        pShader->SetVec2("size"_atom_hash_cexpr, SR_MATH_NS::FVector2(box.width, box.height) / m_viewSize);
        pShader->SetVec4("color"_atom_hash_cexpr, SR_MATH_NS::FColor(color.red, color.green, color.blue, color.alpha) / 255.f);
        pShader->SetSampler2D("textAtlas", pTextAtlas->id);

        DrawElement(shaderInfo);
        UpdateElement(shaderInfo);
        EndElement(shaderInfo);
    }

    litehtml::element::ptr HTMLRenderContainer::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) {
        SR_TRACY_ZONE;
        return nullptr;
    }

    bool HTMLRenderContainer::BeginElement(ShaderInfo& shaderInfo) {
        if (m_pipeline->GetCurrentShader() != shaderInfo.pShader) {
            const auto result = shaderInfo.pShader->Use();
            if (result == ShaderBindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("HTMLRenderContainer::BeginElement() : failed to use shader \"{}\"!", shaderInfo.pShader->GetResourceId().c_str());
                return false;
            }
        }

        return true;
    }

    void HTMLRenderContainer::UpdateElement(ShaderInfo& shaderInfo) {
        auto&& pShader = m_pipeline->GetCurrentShader();
        if (!pShader->Flush()) {
            SR_ERROR("HTMLRenderContainer::UpdateElement() : failed to flush shader \"{}\"!", pShader->GetResourceId().c_str());
        }
    }

    void HTMLRenderContainer::EndElement(ShaderInfo& shaderInfo) {
        ++shaderInfo.index;
    }

    HTMLRenderContainer::TextAtlas* HTMLRenderContainer::GetTextAtlas(const char* text, TextBuilder* pTextBuilder) {
        SR_TRACY_ZONE;

        for (auto&& textAtlas : m_textAtlases) {
            if (textAtlas.text == text && textAtlas.pTextBuilderRef == pTextBuilder) {
                return &textAtlas;
            }
        }

        if (!pTextBuilder->Build(text)) {
            //SR_ERROR("HTMLRenderContainer::GetTextAtlas() : failed to build text!");
            return nullptr;
        }

        SR_GRAPH_NS::SRTextureCreateInfo textureCreateInfo;

        textureCreateInfo.pData = pTextBuilder->GetData();
        textureCreateInfo.format = pTextBuilder->GetColorFormat();
        textureCreateInfo.width = pTextBuilder->GetWidth();
        textureCreateInfo.height = pTextBuilder->GetHeight();
        textureCreateInfo.compression = TextureCompression::None;
        textureCreateInfo.filter = TextureFilter::NEAREST;
        textureCreateInfo.mipLevels = 1;
        textureCreateInfo.cpuUsage = false;
        textureCreateInfo.alpha = true;

        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

        const int32_t id = m_pipeline->AllocateTexture(textureCreateInfo);

        EVK_POP_LOG_LEVEL();

        if (id == SR_ID_INVALID) {
            SR_ERROR("HTMLRenderContainer::GetTextAtlas() : failed to allocate texture!");
            return nullptr;
        }

        TextAtlas textAtlas;
        textAtlas.id = id;
        textAtlas.text = text;
        textAtlas.pTextBuilderRef = pTextBuilder;
        m_textAtlases.emplace_back(textAtlas);
        return &m_textAtlases.back();
    }

    void HTMLRenderContainer::ClearTextAtlases() {
        SR_TRACY_ZONE;

        for (auto&& textAtlas : m_textAtlases) {
            if (!m_pipeline->FreeTexture(&textAtlas.id)) {
                SR_ERROR("HTMLRenderContainer::ClearTextAtlases() : failed to free texture!");
            }
        }
        m_textAtlases.clear();
    }

    void HTMLRenderContainer::DrawElement(ShaderInfo &shaderInfo) {
        if (shaderInfo.index >= shaderInfo.UBOs.size()) SR_UNLIKELY_ATTRIBUTE {
            ShaderInfo::MemInfo memInfo;

            memInfo.virtualUBO = m_uboManager.AllocateUBO(SR_ID_INVALID);
            if (memInfo.virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("HTMLRenderContainer::DrawElement() : failed to allocate uniform buffer object!");
                return;
            }

            memInfo.virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(SR_ID_INVALID);
            if (memInfo.virtualDescriptor == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("HTMLRenderContainer::DrawElement() : failed to allocate descriptor set!");
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
