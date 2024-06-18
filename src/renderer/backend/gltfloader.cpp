#include <mc/renderer/backend/allocator.hpp>
#include <mc/renderer/backend/gltfloader.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>

#include <filesystem>

#include <fastgltf/core.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace renderer::backend
{
    namespace fs = std::filesystem;
    namespace fg = fastgltf;

    void RendererBackend::processGltf()
    {
#if 1
        fs::path gltfDir  = "../../khrSampleModels/2.0/AntiqueCamera/glTF";
        fs::path prevPath = fs::current_path();
        fs::current_path(gltfDir);
        fs::path path = fs::current_path() / "AntiqueCamera.gltf";
#elif 0
        fs::path gltfDir  = "../../khrSampleModels/2.0/AntiqueCamera/glTF";
        fs::path prevPath = fs::current_path();
        fs::current_path(gltfDir);
        fs::path path = fs::current_path() / "AntiqueCamera.gltf";

#else
        fs::path gltfDir  = "../../khrSampleModels/2.0/Sponza/glTF";
        fs::path prevPath = fs::current_path();
        fs::current_path(gltfDir);
        fs::path path = fs::current_path() / "Sponza.gltf";
#endif

        MC_ASSERT_MSG(fs::exists(path), "glTF file path does not exist: {}", path.string());

        SceneResources scene;

        fg::Parser parser;

        constexpr auto gltfOptions = fg::Options::DontRequireValidAssetMember | fg::Options::AllowDouble |
                                     fg::Options::LoadGLBBuffers | fg::Options::LoadExternalBuffers |
                                     fg::Options::LoadExternalImages | fg::Options::GenerateMeshIndices;

        auto gltfFile = fg::MappedGltfFile::FromPath(path);

        MC_ASSERT_MSG(gltfFile, "Failed to open glTF file '{}'", path.string());

        fg::Asset asset;

        {
            auto expected = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);

            MC_ASSERT_MSG(expected, "Failed to load glTF file '{}'", path.string());

            asset = std::move(expected.get());
        }

        auto& defaultMaterial           = m_materials.emplace_back();
        defaultMaterial.baseColorFactor = fastgltf::math::fvec4(1.0f);
        defaultMaterial.alphaCutoff     = 0.0f;
        defaultMaterial.flags           = 0;

        fs::current_path(prevPath);
    }

}  // namespace renderer::backend
