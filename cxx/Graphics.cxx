#include <Utils/macros.h>

#include "../src/Graphics/Pipeline/TextureHelper.cpp"
#include "../src/Graphics/Pipeline/Pipeline.cpp"
#include "../src/Graphics/Pipeline/EmptyPipeline.cpp"
#include "../src/Graphics/Pipeline/FrameBufferQueue.cpp"

#include "../src/Graphics/Overlay/Overlay.cpp"

#include "../src/Graphics/Lighting/DirectionalLight.cpp"
#include "../src/Graphics/Lighting/ILightComponent.cpp"
#include "../src/Graphics/Lighting/PointLight.cpp"
#include "../src/Graphics/Lighting/SpotLight.cpp"
#include "../src/Graphics/Lighting/AreaLight.cpp"
#include "../src/Graphics/Lighting/ProbeLight.cpp"
#include "../src/Graphics/Lighting/LightSystem.cpp"

#include "../src/Graphics/Loaders/FbxLoader.cpp"
#include "../src/Graphics/Loaders/ImageLoader.cpp"
#include "../src/Graphics/Loaders/ObjLoader.cpp"
#include "../src/Graphics/Loaders/SRSL.cpp"
#include "../src/Graphics/Loaders/SRSLParser.cpp"
#include "../src/Graphics/Loaders/TextureLoader.cpp"
#include "../src/Graphics/Loaders/ShaderProperties.cpp"

#include "../src/Graphics/Memory/SSBOManager.cpp"
#include "../src/Graphics/Memory/TextureConfigs.cpp"
#include "../src/Graphics/Memory/MeshManager.cpp"
#include "../src/Graphics/Memory/UBOManager.cpp"
#include "../src/Graphics/Memory/ShaderProgramManager.cpp"
#include "../src/Graphics/Memory/ShaderUBOBlock.cpp"
#include "../src/Graphics/Memory/CameraManager.cpp"
#include "../src/Graphics/Memory/IGraphicsResource.cpp"

#include "../src/Graphics/Font/Font.cpp"
#include "../src/Graphics/Font/FontLoader.cpp"
#include "../src/Graphics/Font/SDF.cpp"
#include "../src/Graphics/Font/ITextComponent.cpp"
#include "../src/Graphics/Font/Text2D.cpp"
#include "../src/Graphics/Font/Text3D.cpp"
#include "../src/Graphics/Font/TextBuilder.cpp"
#include "../src/Graphics/Font/Glyph.cpp"
#include "../src/Graphics/Font/FreeType.cpp"

#include "../src/Graphics/UI/Canvas.cpp"
#include "../src/Graphics/UI/Anchor.cpp"
#include "../src/Graphics/UI/Gizmo.cpp"

#include "../src/Graphics/Render/ScriptableRenderTechnique.cpp"
#include "../src/Graphics/Render/IRenderTechnique.cpp"
#include "../src/Graphics/Render/FlatCluster.cpp"
#include "../src/Graphics/Render/RenderScene.cpp"
#include "../src/Graphics/Render/RenderTechnique.cpp"
#include "../src/Graphics/Render/MeshCluster.cpp"
#include "../src/Graphics/Render/RenderContext.cpp"
#include "../src/Graphics/Render/SortedMeshQueue.cpp"
#include "../src/Graphics/Render/DebugRenderer.cpp"
#include "../src/Graphics/Render/RenderSettings.cpp"
#include "../src/Graphics/Render/RenderStrategy.cpp"
#include "../src/Graphics/Render/FrameBufferController.cpp"
#include "../src/Graphics/Render/FrustumCulling.cpp"

#include "../src/Graphics/Types/Geometry/DebugWireframeMesh.cpp"
#include "../src/Graphics/Types/Geometry/DebugLine.cpp"
#include "../src/Graphics/Types/Geometry/IndexedMesh.cpp"
#include "../src/Graphics/Types/Geometry/ProceduralMesh.cpp"
#include "../src/Graphics/Types/Geometry/Mesh3D.cpp"
#include "../src/Graphics/Types/Geometry/MeshComponent.cpp"
#include "../src/Graphics/Types/Geometry/SkinnedMesh.cpp"
#include "../src/Graphics/Types/Geometry/Sprite.cpp"

#include "../src/Graphics/Types/IRenderComponent.cpp"
#include "../src/Graphics/Types/EditorGrid.cpp"
#include "../src/Graphics/Types/Framebuffer.cpp"
#include "../src/Graphics/Types/Material.cpp"
#include "../src/Graphics/Types/Mesh.cpp"
#include "../src/Graphics/Types/Skybox.cpp"
#include "../src/Graphics/Types/Texture.cpp"
#include "../src/Graphics/Types/GraphicsCommand.cpp"
#include "../src/Graphics/Types/Camera.cpp"
#include "../src/Graphics/Types/Shader.cpp"
#include "../src/Graphics/Types/RenderTexture.cpp"

#include "../src/Graphics/Utils/MeshUtils.cpp"
#include "../src/Graphics/Utils/AtlasBuilder.cpp"

#include "../src/Graphics/Window/Window.cpp"
#include "../src/Graphics/Window/BasicWindowImpl.cpp"

#if defined(SR_USE_IMGUI)
    #include "../src/Graphics/Overlay/ImGuiOverlay.cpp"
#endif

#if defined(SR_USE_VULKAN)
    #include "../src/Graphics/Pipeline/Vulkan/VulkanPipeline.cpp"
    #include "../src/Graphics/Pipeline/Vulkan/VulkanMemory.cpp"
    #include "../src/Graphics/Pipeline/Vulkan/VulkanKernel.cpp"

    #if defined(SR_LINUX)
        #include "../src/Graphics/Pipeline/Vulkan/X11SurfaceInit.cpp"
    #endif

    #if defined(SR_USE_IMGUI)
        #include "../src/Graphics/Overlay/VulkanImGuiOverlay.cpp"
    #endif

    #if defined(SR_TRACY_ENABLE)
        #include "../src/Graphics/Pipeline/Vulkan/VulkanTracy.cpp"
    #endif
#endif

#if defined(SR_WIN32)
    #include "../src/Graphics/Window/Win32Window.cpp"
#endif

#if defined(SR_ANDROID)
    #include "../src/Graphics/Window/AndroidWindow.cpp"
#endif

#if defined(SR_LINUX)
    #include "../src/Graphics/Window/X11Window.cpp"
#endif