//
// Created by Monika on 16.02.2022.
//

#include <Graphics/GUI/Editor/Guizmo.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/GUI/Utils.h>

namespace SR_GRAPH_NS::GUI {
    void Guizmo::DrawTools() {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        const ImVec4 activeColor = ImVec4(0.20, 0.35, 0.20, 1);
        const ImVec4 notActiveColor = ImVec4(0.25, 0.25, 0.25, 1);
        const ImVec4 toggleActiveColor = ImVec4(0.60, 0.50, 0.25, 1);
        const ImVec4 toggleNotActiveColor = ImVec4(0.32, 0.28, 0.25, 1);

        if (ImGui::BeginChild("GuizmoTools", ImVec2(0, 20))) {
            if (GUI::Button("T", IsTranslate() ? activeColor : notActiveColor))
                SetOperation(ImGuizmo::OPERATION::TRANSLATE);

            ImGui::SameLine();

            if (GUI::Button("R", IsRotate() ? activeColor : notActiveColor))
                SetOperation(ImGuizmo::OPERATION::ROTATE);

            ImGui::SameLine();

            if (GUI::Button(IsBounds() ? "S+" : "S", (IsScale() || IsBounds()) ? activeColor : notActiveColor)) {
                if (IsScale())
                    SetOperation(ImGuizmo::OPERATION::BOUNDS);
                else
                    SetOperation(ImGuizmo::OPERATION::SCALE);
            }

            ImGui::SameLine();

            if (GUI::Button("U", IsUniversal() ? activeColor : notActiveColor))
                SetOperation(ImGuizmo::OPERATION::UNIVERSAL);

            ImGui::SameLine();

            if (GUI::Button("L", IsLocal() ? toggleActiveColor : toggleNotActiveColor)) {
                if (IsLocal())
                    SetMode(ImGuizmo::MODE::WORLD);
                else
                    SetMode(ImGuizmo::MODE::LOCAL);
            }

            ImGui::SameLine();

            if (GUI::Button("C", IsCenter() ? toggleActiveColor : toggleNotActiveColor))
                m_center = !m_center;

            ImGui::EndChild();
        }

        ImGui::PopStyleVar(5);
    }

    void Guizmo::Draw(Guizmo::GameObjectPtr gameObject, Guizmo::GameObjectPtr camera) {
        if (!camera.RecursiveLockIfValid()) {
            gameObject.Unlock();
            return;
        }

        if (auto&& pCamera = camera->GetComponent<SR_GRAPH_NS::Types::Camera>()) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            SetRect(pCamera);

            float* view = &pCamera->GetImGuizmoView()[0][0];
            ImGuizmo::ViewManipulate(
                    view,
                    8.f,
                    ImVec2((ImGui::GetWindowPos().x + (float)ImGui::GetWindowWidth()) - 128, ImGui::GetWindowPos().y),
                    ImVec2(128, 128),
                    0x10101010);

            /// камера может быть выбранным объектом, поэтому может произойти двойная блокировка
            if (m_active && gameObject.RecursiveLockIfValid()) {
                m_transform = gameObject->GetTransform();
                m_barycenter = gameObject->GetBarycenter();

                DrawManipulation(pCamera);

                gameObject.Unlock();
            }
        }
        else {
            SRAssert2Once(false, "Camera component not found!");
        }

        camera.Unlock();
    }

    void Guizmo::DrawManipulation(SR_GRAPH_NS::Types::Camera* camera) {
        if (!m_transform) {
            return;
        }

        glm::mat4 transform = GetMatrix();
        glm::mat4 view = camera->GetImGuizmoView();

        if (ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(camera->GetProjection()),
                m_operation,
                m_mode,
                glm::value_ptr(transform),
                NULL, /// delta matrix
                m_snapActive ? m_snap : NULL,
                m_boundsActive ? m_bounds : NULL,
                m_snapActive && m_boundsActive ? m_boundsSnap : NULL
        )) {
            m_isUse = true;
        }
        else {
            if (m_isUse && SR_UTILS_NS::Input::Instance().GetMouseUp(SR_UTILS_NS::MouseCode::MouseLeft)) {
                SR_LOG("END GUIZMO MOVE");
                m_isUse = false;
            }
        }

        if (ImGuizmo::IsUsing()) {
            SR_MATH_NS::FVector3 translation, rotation, scale;
            SR_MATH_NS::DecomposeTransform(transform, translation, rotation, scale);

            translation = translation.InverseAxis(0);
            rotation = rotation.Degrees().InverseAxis(1).InverseAxis(2);

            switch (m_operation) {
                case ImGuizmo::TRANSLATE: {
                    m_transform->SetGlobalTranslation(translation);
                    break;
                }
                case ImGuizmo::ROTATE: {
                    m_transform->SetGlobalRotation(rotation);
                    break;
                }
                case ImGuizmo::SCALE: {
                    m_transform->SetScale(scale);
                    break;
                }
                default:
                    break;
            }
        }
    }

    void Guizmo::SetRect(SR_GRAPH_NS::Types::Camera* camera) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (!window || window->SkipItems)
            return;

        auto imgSize = camera->GetSize();

        const auto winSize = SR_MATH_NS::FVector2(window->Size.x, window->Size.y);

        const Helper::Math::Unit dx = winSize.x / imgSize.x;
        const Helper::Math::Unit dy = winSize.y / imgSize.y;

        if (dy > dx)
            imgSize *= dx;
        else
            imgSize *= dy;

        ImGuizmo::SetRect(
            static_cast<float_t>(ImGui::GetWindowPos().x + (winSize.x - imgSize.x) / 2.f),
            static_cast<float_t>(ImGui::GetWindowPos().y + (winSize.y - imgSize.y) / 2.f),
            imgSize.x,
            imgSize.y
        );
    }

    glm::mat4 Guizmo::GetMatrix() {
        glm::mat4 matrix;

        //const SR_MATH_NS::FVector3 translation = m_transform->GetTranslation().InverseAxis(0);
        //const SR_MATH_NS::FVector3 rotation = m_transform->GetRotation().InverseAxis(1).InverseAxis(2);
        //const SR_MATH_NS::FVector3 scale = m_transform->GetScale();

        auto&& transformation = m_transform->GetMatrix();

        switch (m_transform->GetMeasurement()) {
            case Helper::Measurement::SpaceZero:
            case Helper::Measurement::Space1D:
            case Helper::Measurement::Space4D:
            default:
                matrix = glm::mat4(0);
                break;
            case Helper::Measurement::Space2D:
            case Helper::Measurement::Space3D: {
                const SR_MATH_NS::FVector3 translation = transformation.GetTranslate().InverseAxis(0);
                const SR_MATH_NS::FVector3 rotation = transformation.GetQuat().EulerAngle().InverseAxis(1).InverseAxis(2);
                const SR_MATH_NS::FVector3 scale = m_transform->GetScale();

                matrix = glm::translate(glm::mat4(1), translation.ToGLM());
                matrix *= mat4_cast(SR_MATH_NS::Quaternion::FromEuler(rotation).ToGLM());
                matrix = glm::scale(matrix, scale.ToGLM());
                break;
            }
        }


        return matrix;
    }

    void Guizmo::OnKeyDown(const SR_UTILS_NS::KeyboardInputData* data) {
        switch (data->GetKeyCode()) {
            case Helper::KeyCode::Q: m_active = false; break;
            case Helper::KeyCode::W: SetOperation(ImGuizmo::OPERATION::TRANSLATE); break;
            case Helper::KeyCode::E: SetOperation(ImGuizmo::OPERATION::ROTATE); break;
            case Helper::KeyCode::R: SetOperation(ImGuizmo::OPERATION::SCALE); break;
            default:
                break;
        }
    }

    void Guizmo::OnKeyPress(const SR_UTILS_NS::KeyboardInputData* data) {
        if (!m_transform) {
            return;
        }

        switch (data->GetKeyCode()) {
            case Helper::KeyCode::F: {
                //m_transform->RotateAroundParent(SR_MATH_NS::FVector3(0, 1, 0));
                break;
            }
            default:
                break;
        }
    }
}