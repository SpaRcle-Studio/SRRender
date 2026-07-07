//
// Created by Monika on 08.12.2022.
//

#include <Graphics/Window/AndroidWindow.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/Platform/Platform.h>
#include <Utils/Platform/AndroidEvent.h>

#include <android/input.h>

namespace SR_GRAPH_NS {
    bool AndroidWindow::Initialize(const std::string& name,
        const SR_MATH_NS::IVector2& position,
        const SR_MATH_NS::UVector2& size,
        bool fullScreen, bool resizable
    ) {
        SR_LOG("AndroidWindow::Initialize() : Initializing Android window...\n\tName: {}\n\tSize: {}x{}\n\tFullScreen: {}\n\tResizable: {}",
            name,
            size.x, size.y,
            fullScreen ? "true" : "false",
            resizable ? "true" : "false"
        );

        m_isValid = true;
        m_isInitialized = true;
        m_isFocused = true;

        m_surfaceSize = m_size = size;

        return true;
    }

    ANativeWindow* AndroidWindow::GetNativeWindow() const {
        auto&& pAndroidApp = (android_app*)SR_PLATFORM_NS::GetInstance();
        return pAndroidApp->window;
    }

    void AndroidWindow::PollEvents() {
        SR_UTILS_NS::AndroidEvent event;

        while (SR_UTILS_NS::AndroidEventQueue::Instance().PopEvent(event)) {
            switch (event.type) {
                // --- Тач (тапы, свайпы, мультитач) ---
                case SR_UTILS_NS::AndroidEvent::Motion: {
                    int32_t action = event.motion.action & AMOTION_EVENT_ACTION_MASK;
                    int32_t pointerIndex = (event.motion.action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

                    float x = event.motion.x;
                    float y = event.motion.y;

                    auto&& pMouseDown = SR_GRAPH_GUI_NS::Immediate::GetMouseDown();

                    switch (action) {
                        case AMOTION_EVENT_ACTION_DOWN:
                        case AMOTION_EVENT_ACTION_POINTER_DOWN:
                            if (pointerIndex < 5) {
                                pMouseDown[pointerIndex] = true;
                                SR_GRAPH_GUI_NS::Immediate::SetMousePos(SR_MATH_NS::FVector2(x, y));
                            }
                            break;

                        case AMOTION_EVENT_ACTION_UP:
                        case AMOTION_EVENT_ACTION_POINTER_UP:
                            if (pointerIndex < 5) {
                                pMouseDown[pointerIndex] = false;
                            }
                            break;

                        case AMOTION_EVENT_ACTION_MOVE:
                            SR_GRAPH_GUI_NS::Immediate::SetMousePos(SR_MATH_NS::FVector2(x, y));
                            break;

                        case AMOTION_EVENT_ACTION_CANCEL:
                            for (int i = 0; i < 5; i++) {
                                pMouseDown[i] = false;
                            }
                            break;

                        case AMOTION_EVENT_ACTION_SCROLL:
                            // На Android скролл приходит отдельным событием
                            //io.MouseWheel += event.motion.scrollDeltaY;
                            //io.MouseWheelH += event.motion.scrollDeltaX;
                            break;
                    }
                    break;
                }

                    // --- Клавиатура ---
                case SR_UTILS_NS::AndroidEvent::Key: {
                    //int keycode = event.key.keycode;

                    //if (event.key.action == AKEY_EVENT_ACTION_DOWN) {
                    //    if (keycode < IM_ARRAYSIZE(io.KeysDown))
                    //        io.KeysDown[keycode] = true;
                    //    if (event.key.character != 0)
                    //        io.AddInputCharacter((unsigned int)event.key.character);
                    //}
                    //else if (event.key.action == AKEY_EVENT_ACTION_UP) {
                    //    if (keycode < IM_ARRAYSIZE(io.KeysDown))
                    //        io.KeysDown[keycode] = false;
                    //}
                    break;
                }

                    // --- Сенсоры (акселерометр, гироскоп) ---
                case SR_UTILS_NS::AndroidEvent::Sensor: {
                    // Пример: акселерометр влево/вправо как навигация в ImGui
                    //io.NavInputs[ImGuiNavInput_LStickLeft]  = std::max(0.0f, -event.sensorEvent.sensor.accelX);
                    //io.NavInputs[ImGuiNavInput_LStickRight] = std::max(0.0f,  event.sensorEvent.sensor.accelX);
                    //io.NavInputs[ImGuiNavInput_LStickUp]    = std::max(0.0f, -event.sensorEvent.sensor.accelY);
                    //io.NavInputs[ImGuiNavInput_LStickDown]  = std::max(0.0f,  event.sensorEvent.sensor.accelY);
                    break;
                }

                    // --- Жизненный цикл ---
                case SR_UTILS_NS::AndroidEvent::Lifecycle: {
                    // Можно реагировать на паузу/возврат
                    // io.AddFocusEvent(event.lifecycle.hasFocus);
                    break;
                }

                default:
                    break;
            }
        }
    }

}
