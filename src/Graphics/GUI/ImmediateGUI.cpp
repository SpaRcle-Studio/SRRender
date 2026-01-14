//
// Created by Monika on 10.05.2025.
//

#include <Graphics/GUI/ImmediateGUI.h>
#include <Graphics/GUI/ImGUI.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Enum/TreeNodeFlags.hpp>

namespace SR_GRAPH_GUI_NS::Immediate {
    ImColor FCToImC(const SR_MATH_NS::FColor& color) {
        return ImColor(color.r, color.g, color.b, color.a);
    }

    ImVec2 F2ToImV2(const SR_MATH_NS::FVector2& vec) {
        return ImVec2(vec.x, vec.y);
    }

    ImVec4 FCToImV4(const SR_MATH_NS::FColor& vec) {
        return ImVec4(vec.r, vec.g, vec.b, vec.a);
    }

    SR_MATH_NS::FVector2 ImV2ToF2(const ImVec2& vec) {
        return SR_MATH_NS::FVector2(vec.x, vec.y);
    }

    SR_MATH_NS::FRect IRToFR(const ImRect& rect) {
        return SR_MATH_NS::FRect(
            rect.Min.x,
            rect.Min.y,
            rect.Max.x,
            rect.Max.y
        );
    }

    void Separator() {
        SR_TRACY_ZONE;
        ImGui::Separator();
    }

    void Text(const char* text, ...) {
        va_list args;
        va_start(args, text);
        ImGui::TextV(text, args);
        va_end(args);
    }

    void TextColored(const SR_MATH_NS::FColor& color, const char* text, ...) {
        va_list args;
        va_start(args, text);
        ImGui::TextColored(FCToImC(color), text, args);
        va_end(args);
    }

    void PushID(const char* strId) {
        ImGui::PushID(strId);
    }

    void PushID(const void* ptrId) {
        ImGui::PushID(ptrId);
    }

    void PushID(int intId) {
        ImGui::PushID(intId);
    }

    void PopID() {
        ImGui::PopID();
    }

    void PushStyleVar(StyleVar idx, float val) {
        ImGui::PushStyleVar(static_cast<ImGuiStyleVar>(idx), val);
    }

    void PushStyleVar(StyleVar idx, const SR_MATH_NS::FVector2& val) {
        ImGui::PushStyleVar(static_cast<ImGuiStyleVar>(idx), F2ToImV2(val));
    }

    SR_GRAPH_GUI_NS::Immediate::TreeNodeFlags GetNodeFlagsWithChild() {
        return SR_GRAPH_GUI_NS::Immediate::TreeNodeFlags::OpenOnArrow | SR_GRAPH_GUI_NS::Immediate::TreeNodeFlags::OpenOnDoubleClick;
    }

    SR_GRAPH_GUI_NS::Immediate::TreeNodeFlags GetNodeFlagsWithoutChild() {
        return SR_GRAPH_GUI_NS::Immediate::TreeNodeFlags::NoTreePushOnOpen | SR_GRAPH_GUI_NS::Immediate::TreeNodeFlags::Leaf;
    }

    void PopStyleVar(uint32_t count) {
        ImGui::PopStyleVar(static_cast<int>(count));
    }

    ImmediateDataType GetDataType(std::string_view type) {
        static const std::map<std::string_view, ImmediateDataType> table = {
                { "int", ImmediateDataType::Int32 },
                { "unsigned int", ImmediateDataType::UInt32 },
                { "float", ImmediateDataType::Float },
                { "double", ImmediateDataType::Double }
        };
        if (auto it = table.find(type); it != table.end()) {
            return it->second;
        }
        return ImmediateDataType::COUNT;
    }

    ImmediateDataType GetDataType(uint64_t size, bool isSigned, bool isIntegral) {
        static const std::map<uint64_t, ImmediateDataType> signedTable = {
                { 1, ImmediateDataType::Int8  },
                { 2, ImmediateDataType::Int16 },
                { 4, ImmediateDataType::Int32 },
                { 8, ImmediateDataType::Int64 }
        };

        static const std::map<uint64_t, ImmediateDataType> unsignedTable = {
                { 1, ImmediateDataType::UInt8  },
                { 2, ImmediateDataType::UInt16 },
                { 4, ImmediateDataType::UInt32 },
                { 8, ImmediateDataType::UInt64 }
        };

        if (!isIntegral) {
            if (size == 4) {
                return ImmediateDataType::Float;
            } else if (size == 8) {
                return ImmediateDataType::Double;
            }
            return ImmediateDataType::COUNT;
        }

        if (isSigned) {
            if (auto it = signedTable.find(size); it != signedTable.end()) {
                return it->second;
            }
        }
        else {
            if (auto it = unsignedTable.find(size); it != unsignedTable.end()) {
                return it->second;
            }
        }

        return ImmediateDataType::COUNT;
    }

    ImmediateDataTypeUnion ReadDataType(void* pData, ImmediateDataType type) {
        ImmediateDataTypeUnion result = {};
        switch (type) {
            case ImmediateDataType::Int8:  result.s8   = *(int8_t*)pData; break;
            case ImmediateDataType::UInt8:  result.u8   = *(uint8_t*)pData; break;
            case ImmediateDataType::Int16: result.s16  = *(int16_t*)pData; break;
            case ImmediateDataType::UInt16: result.u16  = *(uint16_t*)pData; break;
            case ImmediateDataType::Int32: result.s32  = *(int32_t*)pData; break;
            case ImmediateDataType::UInt32: result.u32  = *(uint32_t*)pData; break;
            case ImmediateDataType::Int64: result.s64  = *(int64_t*)pData; break;
            case ImmediateDataType::UInt64: result.u64  = *(uint64_t*)pData; break;
            case ImmediateDataType::Float: result.f32 = *(float*)pData; break;
            case ImmediateDataType::Double: result.f64 = *(double*)pData; break;
            default: SRHalt("Unknown ImGuiDataType!"); break;
        }
        return result;
    }

    void WriteDataType(void* pData, ImmediateDataType type, ImmediateDataTypeUnion value) {
        switch (type) {
            case ImmediateDataType::Int8:   *(int8_t*)pData   = value.s8; break;
            case ImmediateDataType::UInt8:  *(uint8_t*)pData  = value.u8; break;
            case ImmediateDataType::Int16:  *(int16_t*)pData  = value.s16; break;
            case ImmediateDataType::UInt16: *(uint16_t*)pData = value.u16; break;
            case ImmediateDataType::Int32:  *(int32_t*)pData  = value.s32; break;
            case ImmediateDataType::UInt32: *(uint32_t*)pData = value.u32; break;
            case ImmediateDataType::Int64:  *(int64_t*)pData  = value.s64; break;
            case ImmediateDataType::UInt64: *(uint64_t*)pData = value.u64; break;
            case ImmediateDataType::Float:  *(float*)pData    = value.f32; break;
            case ImmediateDataType::Double: *(double*)pData   = value.f64; break;
            default: SRHalt("Unknown ImmediateDataType!"); break;
        }
    }

    void SameLine(float_t offsetFromStartX, float_t spacing) {
        SR_TRACY_ZONE;
        ImGui::SameLine(offsetFromStartX, spacing);
    }

    bool IsCurrentlyDisabled() {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        return (ctx->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    }

    bool Button(const char* label, const SR_MATH_NS::FVector2& size) {
        SR_TRACY_ZONE;
        return ImGui::Button(label, F2ToImV2(size));
    }

    bool ButtonColored(const char* label, const SR_MATH_NS::FColor& color, const SR_MATH_NS::FVector2& size) {
        SR_TRACY_ZONE;
        PushStyleColor(StyleColor::Button, color);
        PushStyleColor(StyleColor::ButtonHovered, color + SR_MATH_NS::FColor(0.1f, 0.1f, 0.1f, 0.0f));
        PushStyleColor(StyleColor::ButtonActive, color + SR_MATH_NS::FColor(0.2f, 0.2f, 0.2f, 0.0f));

        const bool result = ImGui::Button(label, F2ToImV2(size));
        PopStyleColor(3);
        return result;
    }

    void PushItemWidth(float_t itemWidth) {
        SR_TRACY_ZONE;
        ImGui::PushItemWidth(itemWidth);
    }

    bool BeginDragDropTarget() {
        SR_TRACY_ZONE;
        return ImGui::BeginDragDropTarget();
    }

    void PopItemWidth() {
        SR_TRACY_ZONE;
        ImGui::PopItemWidth();
    }

    bool Checkbox(const char *label, bool *v) {
        SR_TRACY_ZONE;
        return ImGui::Checkbox(label, v);
    }

    bool DragScalar(const char* label, ImmediateDataType type, void* pData, float_t vSpeed, const void* pMin, const void *pMax, const char* format) {
        SR_TRACY_ZONE;
        return ImGui::DragScalar(label, static_cast<ImGuiDataType_>(type), pData, vSpeed, pMin, pMax, format);
    }

    void PushStyleColor(StyleColor idx, const SR_MATH_NS::FColor& color) {
        SR_TRACY_ZONE;
        ImGui::PushStyleColor(static_cast<ImGuiCol>(idx), FCToImV4(color));
    }

    void PopStyleColor(uint32_t count) {
        SR_TRACY_ZONE;
        ImGui::PopStyleColor(count);
    }

    void BeginGroup() {
        SR_TRACY_ZONE;
        ImGui::BeginGroup();
    }

    void EndGroup() {
        SR_TRACY_ZONE;
        ImGui::EndGroup();
    }

    float_t GetFrameHeight() {
        return ImGui::GetFrameHeight();
    }

    void Dummy(const SR_MATH_NS::FVector2& size) {
        ImGui::Dummy(F2ToImV2(size));
    }

    bool BeginCombo(const char *label, const char *previewValue, ComboFlags flags) {
        return ImGui::BeginCombo(label, previewValue, static_cast<ImGuiComboFlags>(flags));
    }

    void EndCombo() {
        ImGui::EndCombo();
    }

    bool Selectable(const char* label, bool selected) {
        return ImGui::Selectable(label, selected);
    }

    void SetItemDefaultFocus() {
        ImGui::SetItemDefaultFocus();
    }

    uint32_t BeginForceEnabled() {
        const uint32_t stackSize = GImGui->DisabledStackSize;
        for (uint32_t i = 0; i < stackSize; ++i) {
            ImGui::EndDisabled();
        }
        return stackSize;
    }

    void EndForceEnabled(uint32_t stackSize) {
        for (uint32_t i = 0; i < stackSize; ++i) {
            ImGui::BeginDisabled();
        }
    }

    void BeginDisabled() {
        ImGui::BeginDisabled();
    }

    void EndDisabled() {
        ImGui::EndDisabled();
    }

    void* GetCurrentWindow() {
        return ImGui::GetCurrentWindow();
    }

    SR_MATH_NS::FVector2 GetWindowCursorPos(void* pWindow) {
        if (auto&& pImGuiWindow = static_cast<ImGuiWindow*>(pWindow)) {
            return ImV2ToF2(pImGuiWindow->DC.CursorPos);
        }
        return ImV2ToF2(ImGui::GetCurrentWindow()->DC.CursorPos);
    }

    SR_MATH_NS::FVector2 GetCursorScreenPos() {
        return ImV2ToF2(ImGui::GetCursorScreenPos());
    }

    void* GetWindowDrawList(void *pWindow) {
        if (auto&& pImGuiWindow = static_cast<ImGuiWindow*>(pWindow)) {
            return pImGuiWindow->DrawList;
        }
        return ImGui::GetCurrentWindow()->DrawList;
    }

    uint32_t GetColorU32(StyleColor idx, float alpha_mul) {
        return ImGui::GetColorU32(static_cast<ImGuiCol>(idx), alpha_mul);
    }

    void RenderArrow(void* pDrawList, const SR_MATH_NS::FVector2& pos, uint32_t color, Direction dir, float_t scale) {
        if (auto&& pImGuiDrawList = static_cast<ImDrawList*>(pDrawList)) {
            ImGui::RenderArrow(pImGuiDrawList, F2ToImV2(pos), color, static_cast<ImGuiDir>(dir), scale);
        }
    }

    void DrawListAddRect(void* pDrawList, const SR_MATH_NS::FVector2& min, const SR_MATH_NS::FVector2& max, uint32_t color, float rounding, float thickness) {
        if (auto&& pImGuiDrawList = static_cast<ImDrawList*>(pDrawList)) {
            pImGuiDrawList->AddRect(F2ToImV2(min), F2ToImV2(max), color, rounding, 0, thickness);
        }
    }

    void DrawListAddRectFilled(void* pDrawList, const SR_MATH_NS::FVector2& min, const SR_MATH_NS::FVector2& max, uint32_t color, float rounding) {
        if (auto&& pImGuiDrawList = static_cast<ImDrawList*>(pDrawList)) {
            pImGuiDrawList->AddRectFilled(F2ToImV2(min), F2ToImV2(max), color, rounding);
        }
    }

    void DrawListAddLine(void* pDrawList, const SR_MATH_NS::FVector2& p1, const SR_MATH_NS::FVector2& p2, uint32_t color, float thickness) {
        if (auto&& pImGuiDrawList = static_cast<ImDrawList*>(pDrawList)) {
            pImGuiDrawList->AddLine(F2ToImV2(p1), F2ToImV2(p2), color, thickness);
        }
    }

    bool InputFloat(const char* label, float_t* v, float_t step, float_t stepFast, const char* format, InputTextFlags flags) {
        return ImGui::InputFloat(label, v, step, stepFast, format, static_cast<ImGuiInputTextFlags>(flags));
    }

    bool InputInt(const char* label, int* v, int step, int step_fast, InputTextFlags flags) {
        return ImGui::InputInt(label, v, step, step_fast, static_cast<ImGuiInputTextFlags>(flags));
    }

    bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros) {
        return ImGui::Combo(label, current_item, items_separated_by_zeros);
    }

    void DrawTextOnCenter(const std::string& text, ImVec4 color) {
        const auto fontSize = ImGui::GetFontSize() * static_cast<float_t>(text.size()) / 2.f;
        ImGui::SameLine(ImGui::GetWindowSize().x / 2 - fontSize + (fontSize / 2));
        ImGui::TextColored(color, "%s", text.c_str());
    }

    void DrawMultiLineTextOnCenter(const std::string& text) {
        const float_t winWidth = ImGui::GetWindowSize().x;
        const float_t textWidth = ImGui::CalcTextSize(text.c_str()).x;

        /// calculate the indentation that centers the text on one line, relative
        /// to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
        float_t textIndentation = (winWidth - textWidth) * 0.5f;

        /// if text is too long to be drawn on one line, `text_indentation` can
        /// become too small or even negative, so we check a minimum indentation
        float_t minIndentation = 20.0f;
        if (textIndentation <= minIndentation) {
            textIndentation = minIndentation;
        }

        ImGui::SameLine(textIndentation);
        ImGui::PushTextWrapPos(winWidth - textIndentation);
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::PopTextWrapPos();
    }

    void DrawMultiLineTextOnCenter(const std::string &text, ImVec4 color) {
        const float_t winWidth = ImGui::GetWindowSize().x;
        const float_t textWidth = ImGui::CalcTextSize(text.c_str()).x;

        /// calculate the indentation that centers the text on one line, relative
        /// to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
        float_t textIndentation = (winWidth - textWidth) * 0.5f;

        /// if text is too long to be drawn on one line, `text_indentation` can
        /// become too small or even negative, so we check a minimum indentation
        float_t minIndentation = 20.0f;
        if (textIndentation <= minIndentation) {
            textIndentation = minIndentation;
        }

        ImGui::SameLine(textIndentation);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushTextWrapPos(winWidth - textIndentation);

        ImGui::TextWrapped("%s", text.c_str());

        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    bool CollapsingHeader(const std::string& label, TreeNodeFlags _flags) {
        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if (pWindow->SkipItems) {
            return false;
        }

        ImGuiTreeNodeFlags flags = static_cast<ImGuiTreeNodeFlags>(_flags);

        ImGuiID id = pWindow->GetID(label.c_str());
        flags |= ImGuiTreeNodeFlags_CollapsingHeader;
        flags |= ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_ClipLabelForTrailingButton;

        return ImGui::TreeNodeBehavior(id, flags, label.c_str());
    }

    bool ImageButton(std::string_view&& imageId, void* pDescriptor, const SR_MATH_NS::FVector2& size, float_t framePadding, ButtonFlags flags) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

        const bool result = ImageButtonInternal(imageId.data(), pDescriptor, size, framePadding, flags);

        ImGui::PopStyleColor();

        return result;
    }

    bool ImageButtonInternal(std::string_view &&imageId, void* pDescriptor, const SR_MATH_NS::FVector2& size, float_t framePadding, ButtonFlags flags) {
        if (!pDescriptor) {
            SRHalt("ImmediateGUI::ImageButtonInternal() : pDescriptor is null!");
            return false; /// NOLINT
        }

        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        if (window->SkipItems)
            return false;

        ImVec4 bg_col = ImVec4(0,0,0,0);
        ImVec4 tint_col = ImVec4(1,1,1,1);
        ImVec2 uv0, uv1;

        /// if (m_pipeLine == Graphics::PipelineType::OpenGL) {
        uv0 = ImVec2(0, 0);
        uv1 = ImVec2(1, 1);
        /// }
        /// else {
        //uv0 = ImVec2(-1, 0);
        //uv1 = ImVec2(0, 1);
        ///}

        /// Default is to use texture ID as ID. User can still push string/integer prefixes.
        ImGui::PushID((void*)(intptr_t)pDescriptor);
        const ImGuiID id = window->GetID(imageId.data());
        ImGui::PopID();

        const ImVec2 padding = (framePadding >= 0) ? ImVec2((float)framePadding, (float)framePadding) : g.Style.FramePadding;

        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size.x, size.y) + padding * 2);
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

        /// Render
        const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImGui::RenderNavHighlight(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));
        if (bg_col.w > 0.0f)
            window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, ImGui::GetColorU32(bg_col));
        window->DrawList->AddImage((ImTextureID)pDescriptor, bb.Min + padding, bb.Max - padding, uv0, uv1, ImGui::GetColorU32(tint_col));

        return pressed;
    }

    SR_MATH_NS::FVector2 DrawTexture(const void* pDescriptor, const SR_MATH_NS::FVector2& size, SR_GRAPH_NS::PipelineType pipelineType, bool imposition) {
        if (!pDescriptor) {
            return SR_MATH_NS::FVector2(); /// NOLINT
        }

        auto&& fSize = size.Cast<float_t>();

        switch (pipelineType) {
            case PipelineType::Vulkan:
                return DrawImage(const_cast<void*>(pDescriptor), SR_MATH_NS::FVector2(fSize.x, fSize.y), SR_MATH_NS::FVector2(0, 0), SR_MATH_NS::FVector2(1, 1), { 1, 1, 1, 1 }, { 0, 0, 0, 0 }, imposition);
            case PipelineType::OpenGL:
                return DrawImage(const_cast<void*>(pDescriptor), SR_MATH_NS::FVector2(fSize.x, fSize.y), SR_MATH_NS::FVector2(0, 0), SR_MATH_NS::FVector2(1, 1), { 1, 1, 1, 1 }, { 0, 0, 0, 0 }, imposition);
            default:
                return SR_MATH_NS::FVector2(); /// NOLINT
        }
    }

    SR_MATH_NS::FVector2 DrawImage(ImTextureID user_texture_id, const SR_MATH_NS::FVector2& size, const SR_MATH_NS::FVector2& uv0, const SR_MATH_NS::FVector2& uv1, const SR_MATH_NS::FColor& tint_col, const SR_MATH_NS::FColor& border_col, bool imposition) {
        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if (pWindow->SkipItems) {
            return SR_MATH_NS::FVector2(); /// NOLINT
        }

        ImRect bb(pWindow->DC.CursorPos, pWindow->DC.CursorPos + F2ToImV2(size));
        if (border_col.w > 0.0f) {
            bb.Max = bb.Max + ImVec2(2, 2);
        }

        if (!imposition) {
            ImGui::ItemSize(bb);
            if (!ImGui::ItemAdd(bb, 0)) {
                return SR_MATH_NS::FVector2(); /// NOLINT
            }
        }

        if (border_col.w > 0.0f) {
            pWindow->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(FCToImV4(border_col)), 0.0f);
            pWindow->DrawList->AddImage(user_texture_id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), F2ToImV2(uv0), F2ToImV2(uv1), ImGui::GetColorU32(FCToImV4(tint_col)));
        }
        else {
            pWindow->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, F2ToImV2(uv0), F2ToImV2(uv1), ImGui::GetColorU32(FCToImV4(tint_col)));
        }

        return ImV2ToF2(bb.GetTL());
    }

    bool BeginDragDropTargetWindow(const char* payloadType) {
        ImRect inner_rect = ImGui::GetCurrentWindow()->InnerRect;

        if (ImGui::BeginDragDropTargetCustom(inner_rect, ImGui::GetID("##WindowBgArea")))
        {
            auto&& pPayload = ImGui::AcceptDragDropPayload(payloadType, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
            if (pPayload) {
                if (pPayload->IsPreview()) {
                    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
                    draw_list->AddRectFilled(inner_rect.Min, inner_rect.Max, ImGui::GetColorU32(ImGuiCol_DragDropTarget, 0.05f));
                    draw_list->AddRect(inner_rect.Min, inner_rect.Max, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
                }

                if (pPayload->IsDelivery()) {
                    return true;
                }

                ImGui::EndDragDropTarget();
            }
        }

        return false;
    }

    bool ImageButton(void* pDescriptor, const SR_MATH_NS::FVector2& size) {
        return ImageButton(pDescriptor, size, -1);
    }

    bool ImageButton(void* pDescriptor, const SR_MATH_NS::FVector2& size, float_t framePadding) {
        return ImageButton("##image", pDescriptor, size, framePadding);
    }

    bool ImageButtonDouble(std::string_view&& imageId, void* pDescriptor, const SR_MATH_NS::FVector2& size, float_t framePadding) {
        return ImageButton(imageId.data(), pDescriptor, size, framePadding, ButtonFlags::PressedOnDoubleClick);
    }

    bool ImageButton(std::string_view&& imageId, void* pDescriptor, const SR_MATH_NS::FVector2& size, float_t framePadding) {
        return ImageButton(imageId.data(), pDescriptor, size, framePadding, ButtonFlags::None);
    }

    int ImTextCharToUtf8(char* pBuffer, int32_t bufSize, uint32_t c) {
        if (c < 0x80)
        {
            pBuffer[0] = (char)c;
            return 1;
        }
        if (c < 0x800)
        {
            if (bufSize < 2) return 0;
            pBuffer[0] = (char)(0xc0 + (c >> 6));
            pBuffer[1] = (char)(0x80 + (c & 0x3f));
            return 2;
        }
        if (c < 0x10000)
        {
            if (bufSize < 3) return 0;
            pBuffer[0] = (char)(0xe0 + (c >> 12));
            pBuffer[1] = (char)(0x80 + ((c >> 6) & 0x3f));
            pBuffer[2] = (char)(0x80 + ((c ) & 0x3f));
            return 3;
        }
        if (c <= 0x10FFFF)
        {
            if (bufSize < 4) return 0;
            pBuffer[0] = (char)(0xf0 + (c >> 18));
            pBuffer[1] = (char)(0x80 + ((c >> 12) & 0x3f));
            pBuffer[2] = (char)(0x80 + ((c >> 6) & 0x3f));
            pBuffer[3] = (char)(0x80 + ((c ) & 0x3f));
            return 4;
        }
        /// Invalid code point, the max unicode is 0x10FFFF
        return 0;
    }

    bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size) {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    SR_MATH_NS::FVector2 DrawTexture(const SR_GRAPH_NS::Pipeline* pPipeline, uint32_t textureId, const SR_MATH_NS::FVector2 &size, bool imposition) {
        void* pDescriptor = nullptr;

        switch (pPipeline->GetType()) {
            case SR_GRAPH_NS::PipelineType::Vulkan:
                pDescriptor = pPipeline->GetOverlayTextureDescriptorSet(textureId, SR_GRAPH_NS::OverlayType::ImGui);
                break;
            case SR_GRAPH_NS::PipelineType::OpenGL:
                pDescriptor = reinterpret_cast<void*>(static_cast<uint64_t>(textureId));
                break;
            default:
                break;
        }

        return SR_GRAPH_GUI_NS::Immediate::DrawTexture(pDescriptor, size, pPipeline->GetType(), false);
    }

    bool BeginTabBar(const char *str_id) {
        return ImGui::BeginTabBar(str_id);
    }

    void EndTabBar() {
        ImGui::EndTabBar();
    }

    void TextWrapped(const char* text, ...) {
        va_list args;
        va_start(args, text);
        ImGui::TextWrappedV(text, args);
        va_end(args);
    }

    void PushFont(void* pFont) {
        ImGui::PushFont(static_cast<ImFont*>(pFont));
    }

    void PopFont() {
        ImGui::PopFont();
    }

    float_t GetFontSize() {
        return GImGui->Font->FontSize;
    }

    SR_MATH_NS::FVector2 GetFramePadding() {
        return ImV2ToF2(GImGui->Style.FramePadding);
    }

    void SetKeyboardFocusHere() {
        ImGui::SetKeyboardFocusHere();
    }

    void CloseCurrentPopup() {
        ImGui::CloseCurrentPopup();
    }

    bool InputText(const char* label, std::string* str, InputTextFlags flags) {
        return ImGui::InputText(label, str, static_cast<ImGuiInputTextFlags>(flags));
    }

    bool MenuItem(const char *label) {
        return ImGui::MenuItem(label);
    }

    void EndMenu() {
        ImGui::EndMenu();
    }

    bool BeginListBox(const char* label, const SR_MATH_NS::FVector2& size) {
        return ImGui::BeginListBox(label, F2ToImV2(size));
    }

    void EndListBox() {
        ImGui::EndListBox();
    }

    bool BeginMenu(const char *label) {
        return ImGui::BeginMenu(label);
    }

    void LabelText(const char* label, const char *text, ...) {
        va_list args;
        va_start(args, text);
        ImGui::LabelTextV(label, text, args);
        va_end(args);
    }

    float_t GetFramerate() {
        return ImGui::GetIO().Framerate;
    }

    void LoadIniSettingsFromDisk() {
        ImGuiContext& g = *GImGui;
        if (g.IO.IniFilename)
            ImGui::LoadIniSettingsFromDisk(g.IO.IniFilename);
        g.SettingsLoaded = true;
    }

    void BeginDocking() {
        ImGuiViewport* pViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(pViewport->Pos);
        ImGui::SetNextWindowSize(pViewport->Size);
        ImGui::SetNextWindowViewport(pViewport->ID);

        static constexpr ImGuiWindowFlags windowFlags = 0
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("SpaRcle Engine", nullptr, windowFlags);
    }

    void EndDocking() {
        ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    void FocusTopMostWindowUnderOne() {
        ImGuiContext& g = *GImGui;
        if (g.CurrentWindow == g.NavWindow && g.NavLayer == ImGuiNavLayer_Main && !g.NavAnyRequest) {
            ImGui::FocusTopMostWindowUnderOne(g.NavWindow, nullptr);
        }
    }

    bool BeginMainMenuBar() {
        return ImGui::BeginMainMenuBar();
    }

    void EndMenuBar() {
        ImGui::EndMenuBar();
    }

    void End() {
        ImGui::End();
    }

    bool SmallButton(const char *label) {
        return ImGui::SmallButton(label);
    }

    void SetCursorPosX(float_t x) {
        ImGui::SetCursorPosX(x);
    }

    void SetCursorPosY(float_t y) {
        ImGui::SetCursorPosY(y);
    }

    SR_MATH_NS::FVector2 GetCursorPos() {
        return ImV2ToF2(ImGui::GetCursorPos());
    }

    SR_MATH_NS::FVector2 GetWindowSize() {
        return ImV2ToF2(ImGui::GetWindowSize());
    }

    bool IsMouseDragging(MouseButton button) {
        return ImGui::IsMouseDragging(static_cast<ImGuiMouseButton>(button));
    }

    bool IsMouseReleased(MouseButton button) {
        return ImGui::IsMouseReleased(static_cast<ImGuiMouseButton>(button));
    }

    bool IsMouseDown(MouseButton button) {
        return ImGui::IsMouseDown(static_cast<ImGuiMouseButton>(button));
    }

    SR_MATH_NS::FVector2 GetMousePos() {
        return ImV2ToF2(ImGui::GetMousePos());
    }

    SR_MATH_NS::FRect GetWindowRect(void* pWindow) {
        if (auto&& pImGuiWindow = static_cast<ImGuiWindow*>(pWindow)) {
            return IRToFR(pImGuiWindow->Rect());
        }
        return IRToFR(ImGui::GetCurrentWindow()->Rect());
    }

    bool Combo(const char *label, int *current_item, bool (*items_getter)(void *, int, const char **), void *data, int items_count, int popup_max_height_in_items) {
        return ImGui::Combo(label, current_item, items_getter, data, items_count, popup_max_height_in_items);
    }

    bool IsItemFocused() {
        return ImGui::IsItemFocused();
    }

    void EndPopup() {
        ImGui::EndPopup();
    }

    bool BeginPopup(const char *name) {
        return ImGui::BeginPopup(name);
    }

    void SetCursorPos(const SR_MATH_NS::FVector2& pos) {
        ImGui::SetCursorPos(F2ToImV2(pos));
    }

    float_t GetScrollbarSize() {
        return ImGui::GetStyle().ScrollbarSize;
    }

    bool IsItemDeactivatedAfterEdit() {
        return ImGui::IsItemDeactivatedAfterEdit();
    }

    bool BeginChild(const char *str_id, const Utils::Math::FVector2 &size, bool border) {
        return ImGui::BeginChild(str_id, F2ToImV2(size), border);
    }

    bool BeginTable(const char* str_id, int columns) {
        return ImGui::BeginTable(str_id, columns);
    }

    void TableSetColumnIndex(int column_n) {
        ImGui::TableSetColumnIndex(column_n);
    }

    void EndTable() {
        ImGui::EndTable();
    }

    void EndChild() {
        ImGui::EndChild();
    }

    void TableNextRow() {
        ImGui::TableNextRow();
    }

    const void* AcceptDragDropPayload(const char* type) {
        SR_TRACY_ZONE;
        return ImGui::AcceptDragDropPayload(type);
    }

    void EndDragDropTarget() {
        ImGui::EndDragDropTarget();
    }

    void* GetDataFromDragDropPayload(const void* pPayload) {
        SR_TRACY_ZONE;
        return static_cast<const ImGuiPayload*>(pPayload)->Data;
    }

    bool BeginTabItem(const char *str_id) {
        return ImGui::BeginTabItem(str_id);
    }

    void EndTabItem() {
        ImGui::EndTabItem();
    }

    float_t GetScrollMaxY() {
        return ImGui::GetScrollMaxY();
    }

    SR_MATH_NS::FVector2 GetContentRegionAvail() {
        return ImV2ToF2(ImGui::GetContentRegionAvail());
    }

    SR_MATH_NS::FVector2 CalcTextSize(const char *text) {
        return ImV2ToF2(ImGui::CalcTextSize(text));
    }

    SR_MATH_NS::FVector2 GetItemRectSize() {
        return ImV2ToF2(ImGui::GetItemRectSize());
    }

    SR_MATH_NS::FVector2 GetItemRectMin() {
        return ImV2ToF2(ImGui::GetItemRectMin());
    }

    void OpenPopup(const char *str_id) {
        ImGui::OpenPopup(str_id);
    }

    void AddText(void* pDrawList, const SR_MATH_NS::FVector2& pos, uint32_t color, const char* text) {
        if (auto&& pImGuiDrawList = static_cast<ImDrawList*>(pDrawList)) {
            pImGuiDrawList->AddText(F2ToImV2(pos), color, text);
        }
    }

    bool IsItemClicked(MouseButton button) {
        return ImGui::IsItemClicked(static_cast<ImGuiMouseButton>(button));
    }

    const void* GetDragDropPayload() {
        SR_TRACY_ZONE;
        return ImGui::GetDragDropPayload();
    }

    void EndDragDropSource() {
        ImGui::EndDragDropSource();
    }

    bool BeginDragDropSource(DragDropFlags flags) {
        return ImGui::BeginDragDropSource(static_cast<ImGuiDragDropFlags>(flags));
    }

    void SetDragDropPayload(const char *type, const void *data, size_t size, Condition cond) {
        ImGui::SetDragDropPayload(type, data, size, static_cast<ImGuiCond>(cond));
    }

    void WindowTreeNodeSetOpen(bool open, uint64_t id) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGui::TreeNodeSetOpen(window->GetID((void*)(intptr_t)id), true);
    }

    bool IsAnyItemHovered() {
        return ImGui::IsAnyItemHovered();
    }

    bool IsWindowHovered(HoveredFlags flags) {
        return ImGui::IsWindowHovered(static_cast<ImGuiHoveredFlags>(flags));
    }

    bool IsMouseDoubleClicked(MouseButton button) {
        return ImGui::IsMouseDoubleClicked(static_cast<ImGuiMouseButton>(button));
    }

    bool BeginPopupContextWindow(const char *str_id) {
        return ImGui::BeginPopupContextWindow(str_id);
    }

    bool BeginPopupContextItem(const char *str_id) {
        return ImGui::BeginPopupContextItem(str_id);
    }

    bool IsItemHovered() {
        return ImGui::IsItemHovered();
    }

    bool TreeNodeEx(const void *ptr_id, TreeNodeFlags flags, const char *fmt, ...) {
        SR_TRACY_ZONE;
        va_list args;
        va_start(args, fmt);
        const bool result = ImGui::TreeNodeExV(ptr_id, static_cast<ImGuiTreeNodeFlags>(flags), fmt, args);
        va_end(args);
        return result;
    }

    void TreePop() {
        SR_TRACY_ZONE;
        ImGui::TreePop();
    }

    const char* GetPayloadType(const void *pPayload) {
        return static_cast<const ImGuiPayload*>(pPayload)->DataType;
    }

    float_t GetFrameHeightWithSpacing() {
        return ImGui::GetFrameHeightWithSpacing();
    }

    bool IsItemToggledOpen() {
        return ImGui::IsItemToggledOpen();
    }

    bool DragFloat(const char *label, float_t *v, float_t vSpeed, const float_t min, const float_t max, const char *format) {
        return ImGui::DragFloat(label, v, vSpeed, min, max, format);
    }

    bool DragFloat2(const char *label, float_t v[2], float_t vSpeed, const float_t min, const float_t max, const char *format) {
        return ImGui::DragFloat2(label, v, vSpeed, min, max, format);
    }

    bool DragFloat3(const char *label, float_t v[3], float_t vSpeed, const float_t min, const float_t max, const char *format) {
        return ImGui::DragFloat3(label, v, vSpeed, min, max, format);
    }

    bool SliderFloat(const char *label, float_t *v, float_t min, float_t max, const char *format) {
        return ImGui::SliderFloat(label, v, min, max, format);
    }

    bool InputText(const char *label, char *str, size_t strSize, InputTextFlags flags) {
        return ImGui::InputText(label, str, strSize, static_cast<ImGuiInputTextFlags>(flags));
    }

    bool InputTextMultiline(const char *label, std::string *str, const SR_MATH_NS::FVector2 &size, InputTextFlags flags) {
        return ImGui::InputTextMultiline(label, str, F2ToImV2(size), static_cast<ImGuiInputTextFlags>(flags));
    }

    void* FindWindowByName(const char *name) {
        return ImGui::FindWindowByName(name);
    }

    void TextVertical(const char* text, SR_MATH_NS::FVector2 pos, SR_MATH_NS::FColor color) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        float yOffset = 0.0f;

        for (const char* c = text; *c; c++) {
            char buf[2] = { *c, '\0' };
            draw_list->AddText(
                    font,
                    font->FontSize,
                    ImVec2(pos.x, pos.y + yOffset),
                    ImGui::GetColorU32(FCToImV4(color)),
                    buf
            );
            yOffset += font->FontSize; // шаг вниз
        }

        // Чтобы layout ImGui знал, что занято место
        ImGui::Dummy(ImVec2(0, yOffset));
    }
}