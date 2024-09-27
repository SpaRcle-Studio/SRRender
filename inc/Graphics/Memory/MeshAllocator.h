//
// Created by Monika on 15.11.2021.
//

#ifndef SR_ENGINE_MESHALLOCATOR_H
#define SR_ENGINE_MESHALLOCATOR_H

//#include <Types/Geometry/Mesh3D.h>
//#include <Types/Geometry/DebugWireframeMesh.h>
//#include <UI/Sprite.h>
//
//namespace Framework::Graphics::Memory {
//    class MeshAllocator {
//    public:
//        MeshAllocator() = delete;
//        MeshAllocator(MeshAllocator &) = delete;
//        ~MeshAllocator() = delete;
//
//    public:
//        template<typename U> static U* Allocate() {
//            if constexpr (std::is_same<Types::Mesh3D, U>::value) {
//                return new Types::Mesh3D();
//            }
//            else if constexpr (std::is_same<Types::DebugWireframeMesh, U>::value) {
//                return new Types::DebugWireframeMesh();
//            }
//            else if constexpr (std::is_same<UI::Sprite, U>::value) {
//                return new UI::Sprite();
//            }
//            else
//                return nullptr;
//        }
//    };
//}

#endif //SR_ENGINE_MESHALLOCATOR_H
