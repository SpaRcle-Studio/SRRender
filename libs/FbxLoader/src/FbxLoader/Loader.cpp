//
// Created by Monika on 26.07.2021.
//

#include <FbxLoader/Loader.h>
#include <FbxLoader/Utils.h>
#include <FbxLoader/Parser.h>
#include <FbxLoader/MD5Hash.h>
#include <unordered_set>
#include <inc/tiny_obj_loader.h>

FbxLoader::RawFbx FbxLoader::Loader::Load(
    const std::string& converter,
    const std::string& cache,
    const std::string& models,
    const std::string& path,
    bool optimizeGeometry
) {
    if (!Debug::IsInit())
        return {};

    const std::string name      = Tools::BackReadTo(path, '/', 1);
    const std::string ext       = Tools::BackReadTo(name, '.', 1);
    const std::string dir       = Tools::ReadToLast(path, '/', 1);
    const std::string ascii     = Tools::FixPath(cache + "/fbx_ascii/" + dir);
    const std::string model     = models + "/" + path;
    const std::string hashPath  = ascii + name + ".hash";
    const std::string cacheFile = ascii + name + ".cache";

    if (!Tools::FileExists(model)) {
        FBX_ERROR("Loader::Load() : file not exists! Path: " + model);
        return {};
    }

    const std::string hash = Tools::GetHash(model);

    if (Tools::FileExists(cacheFile) && Tools::LoadHash(hashPath) == hash) {
        RawFbx fbx;
        fbx.LoadFrom(cacheFile);
        return fbx;
    }
    else {
        Tools::CreatePath(ascii);
        if (ext == "fbx") {
            /// TODO: make cross-platform
            const std::string args = converter + " -c " + model + " -o " + ascii + name + " -ascii";
            system(args.c_str());

            if (!Tools::FileExists(ascii + name)) {
                FBX_ERROR("Loader::Load() : failed to convert binary to ascii!");
                return {};
            }
        }
        Tools::SaveHash(hashPath, hash);
    }

    if (ext == "fbx") {
        return LoadFbx(ascii + name, cacheFile, optimizeGeometry);
    }
    else if (ext == "obj") {
        return LoadObj(model, cacheFile, optimizeGeometry);
    }

    FBX_ERROR("Loader::Load() : unknown extension: " + ext + "\n\tPath: " + path);
    return RawFbx();
}

FbxLoader::Objects FbxLoader::Loader::GetObjects(FbxLoader::Parser::Node *node) {
    auto node_objects = node->Find("Objects");
    if (!node_objects)
        return {};

    Objects objects = {};

    for (auto object : node_objects->nodes) {
        if (object->value == "Geometry") {
            if (auto geometry = ParseGeometry(object); geometry.Valid())
                objects.geometries.emplace_back(std::move(geometry));
        }
        else if (object->value == "Model") {
            if (auto model = ParseModel(object); model.Valid())
                objects.models.emplace_back(std::move(model));
        }
    }

    return objects;
}

std::vector<FbxLoader::Vertex> FbxLoader::Loader::GetVertices(FbxLoader::Parser::Node *object, const std::vector<uint32_t> &indices) {
    auto&& vertices_node = object->Find("Vertices")->Get2SubNode();

    const auto normals_node = [object]() -> Parser::Node* {
        if (auto v = object->Find("LayerElementNormal"); v) return v->Find("Normals")->Get2SubNode(); return nullptr;
    }();

    const auto binormals_node = [object]() -> Parser::Node* {
        if (auto v = object->Find("LayerElementBinormal"); v) return v->Find("Binormals")->Get2SubNode(); return nullptr;
    }();

    const auto tangents_node = [object]() -> Parser::Node* {
        if (auto v = object->Find("LayerElementTangent"); v) return v->Find("Tangents")->Get2SubNode(); return nullptr;
    }();

    const auto uvs_node = [object]() -> Parser::Node* {
        if (auto v = object->Find("LayerElementUV"); v) return v->Find("UV")->Get2SubNode(); return nullptr;
    }();

    const auto uvIndices_node = [object]() -> Parser::Node* {
        if (auto layerUV = object->Find("LayerElementUV"); layerUV)
            if (auto uvIndex = layerUV->Find("UVIndex"); uvIndex)
                return uvIndex->Get2SubNode();
        return nullptr;
    }();

    return MakeVertices(
            indices,
            vertices_node  ? Tools::SplitAndCastToFloat(vertices_node->value, ',')   : std::vector<float_t>(),
            normals_node   ? Tools::SplitAndCastToFloat(normals_node->value, ',')    : std::vector<float_t>(),
            binormals_node ? Tools::SplitAndCastToFloat(binormals_node->value, ',')  : std::vector<float_t>(),
            tangents_node  ? Tools::SplitAndCastToFloat(tangents_node->value, ',')   : std::vector<float_t>(),
            uvs_node       ? Tools::SplitAndCastToFloat(uvs_node->value, ',')        : std::vector<float_t>(),
            uvIndices_node ? Tools::SplitAndCastToUInt32(uvIndices_node->value, ',') : std::vector<uint32_t>()
    );
}

bool FbxLoader::Loader::OptimizeGeometry(FbxLoader::RawGeometry *geometry) {
    if (!geometry)
        return false;

    std::vector<uint32_t> indices  = {};
    std::vector<Vertex>   vertices = {};

    std::unordered_map<Vertex, uint32_t> uniqueVertices {};

    for (const auto& vertex : geometry->vertices) {
        if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }

        indices.push_back(uniqueVertices[vertex]);
    }

    geometry->vertices = vertices;
    geometry->indices  = indices;

    return true;
}

std::vector<FbxLoader::MaterialRange> FbxLoader::Loader::GetMaterialRanges(FbxLoader::Parser::Node *object) {
    std::vector<MaterialRange> materials;

    auto materials_node = [object]() -> Parser::Node* {
        if (auto v = object->Find("LayerElementMaterial"); v) return v->Find("Materials")->Get2SubNode(); return nullptr;
    }();

    if (!materials_node)
        return {};

    std::istringstream ss(materials_node->value);
    std::string value;
    MaterialRange material;

    uint32_t last = 0;
    uint32_t counter = 0;
    uint32_t counterPrev = 0;
    while (getline(ss, value, ',')) {
        auto id = static_cast<uint32_t>(std::stoi(value));

        if (id != last) {
            last = id;

            material.first = counterPrev;
            material.second = counter;

            materials.emplace_back(material);

            counterPrev = counter;
        }

        ++counter;
    }

    material.first = counterPrev;
    material.second = counter;

    materials.emplace_back(material);

    return materials;
}

std::vector<FbxLoader::RawGeometry> FbxLoader::Loader::SplitByMaterials(RawGeometry&& geometry) {
    std::vector<RawGeometry> geometries;

    if (geometry.materials.size() <= 1) {
        geometries.emplace_back(std::move(geometry));
        return geometries;
    }

    uint32_t materialIndex= 0;
    for (const auto& material : geometry.materials) {
        RawGeometry newGeometry;

        newGeometry.materials = { material };
        newGeometry.name = geometry.name + " (" + std::to_string(materialIndex++) + ")";
        newGeometry.type = geometry.type;
        newGeometry.id   = geometry.id;

        std::vector<Vertex> vertices;
        std::unordered_map<Vertex, uint32_t> unique;

        for (auto vertexIndex : std::vector<uint32_t>(
                geometry.indices.begin() + material.first * 3,
                geometry.indices.begin() + material.second * 3))
        {
            auto&& vertex = geometry.vertices[vertexIndex];

            if (unique.count(vertex) == 0) {
                unique[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            newGeometry.indices.push_back(unique[vertex]);
        }

        newGeometry.vertices = vertices;

        if (newGeometry.Valid()) {
            geometries.emplace_back(std::move(newGeometry));
        }
        else {
            FBX_WARN("Loader::SplitByMaterials() : invalid \"" + newGeometry.name + "\" mesh!");
        }
    }

    return geometries;
}

FbxLoader::RawFbx FbxLoader::Loader::LoadFbx(const std::string &ascii, const std::string &cache, bool needOptimize) {
    if (auto text = Tools::ReadAllText(ascii); text.empty()) {
        FBX_ERROR("FbxLoader::Load() : failed to read file! \n\tPath: " + ascii);
        return {};
    }
    else if (auto nodes = Parser::Parse(text); nodes) {
        RawFbx fbx = {};

        if (fbx.objects = GetObjects(nodes); !fbx.objects.Ready()) {
            delete nodes;
            FBX_ERROR("FbxLoader::Load() : failed to get objects!");
            return {};
        }

        if (needOptimize)
            for (auto& geometry : fbx.objects.geometries)
                if (!OptimizeGeometry(&geometry))
                    FBX_ERROR("FbxLoader::Load() : failed to optimize \"" + geometry.name + "\" geometry!");

        if (!fbx.objects.geometries.empty()) {
            auto source = std::exchange(fbx.objects.geometries, {});
            for (auto&& src : source) {
                for (auto &&geometry : SplitByMaterials(std::move(src)))
                    fbx.objects.geometries.emplace_back(std::move(geometry));
            }
        }

        delete nodes;

        fbx.SaveTo(cache);

        return fbx;
    }
    else {
        FBX_ERROR("FbxLoader::Load() : failed to parse file!");
        return {};
    }
}

FbxLoader::RawFbx FbxLoader::Loader::LoadObj(const std::string &path, const std::string &cache, bool needOptimize) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    RawFbx fbx;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        FBX_ERROR("Loader::LoadObj() : failed to open file!\n\tWarn: " + warn + "\n\tError: " + err);
        return fbx;
    }

    for (auto& shape : shapes) {
        RawGeometry geometry;
        geometry.name = shape.name;

        std::unordered_map<Vertex, uint32_t> uniqueVertices {};

        /// prepare indices
        if (shape.mesh.indices.empty()) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(attrib.vertices.size() / 3); ++i) {
                tinyobj::index_t index = {};

                index.vertex_index = i;

                shape.mesh.indices.emplace_back(index);
            }
        }

        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;

            vertex.pos = vec3 {
                    (float_t)attrib.vertices[3 * index.vertex_index + 0],
                    (float_t)attrib.vertices[3 * index.vertex_index + 1],
                    (float_t)attrib.vertices[3 * index.vertex_index + 2]
            };

            if (!attrib.texcoords.empty()) {
                vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            if (!attrib.normals.empty()) {
                vertex.norm = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                };
            }

            vertex.tang = { 0, 0, 0 }; /// TODO: add tangent matching!

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(geometry.vertices.size());
                geometry.vertices.push_back(vertex);
            }

            geometry.indices.push_back(uniqueVertices[vertex]);
        }

        fbx.objects.geometries.emplace_back(std::move(geometry));
    }

    fbx.SaveTo(cache);

    return fbx;
}

FbxLoader::RawGeometry FbxLoader::Loader::ParseGeometry(FbxLoader::Parser::Node *node) {
    auto info = Tools::Split(node->subData, ',');
    if (info.size() != 3) {
        FBX_ERROR("FbxLoader::ParseGeometry() : failed to get geometry info!");
        return {};
    }

    RawGeometry geometry = { };
    geometry.id = std::stoll(info[0]);
    geometry.name = Tools::GetBetween(info[1], '\"');
    geometry.type = Tools::GetBetween(info[2], '\"');

    if (geometry.type == "Shape") {
        FBX_WARN("FbxLoader::ParseGeometry() : shape \"" + geometry.name + "\" ignored.")
        return RawGeometry();
    }

    if (auto indices = node->Find("PolygonVertexIndex"); indices) {
        geometry.indices = FixIndices(Tools::SplitAndCastToInt32(indices->Get2SubNode()->value, ','));
        if (geometry.indices.empty()) {
            FBX_ERROR("FbxLoader::ParseGeometry() : failed to parse indices!");
            return {};
        }

        if (geometry.vertices = GetVertices(node, geometry.indices); geometry.vertices.empty()) {
            FBX_ERROR("FbxLoader::ParseGeometry() : failed parse vertices!");
            return {};
        }

        geometry.materials = GetMaterialRanges(node);
    }

    if (geometry.vertices.empty()) {
        FBX_WARN("FbxLoader::ParseGeometry() : geometry \"" + geometry.name + "\" have not vertices!");
    }

    if (geometry.indices.empty()) {
        FBX_WARN("FbxLoader::ParseGeometry() : geometry \"" + geometry.name + "\" have not indices!");
    }

    return geometry;
}

FbxLoader::RawModel FbxLoader::Loader::ParseModel(FbxLoader::Parser::Node *node) {
    FbxLoader::RawModel model;

    int32_t version = INT32_MAX;

    if (auto&& versionNode = node->Find("Version"); versionNode) {
        version = std::stoi(versionNode->subData);
    }
    else {
        FBX_ERROR("Loader::ParseModel() : model have not version!");
        return {};
    }

    switch (version) {
        case 232: {
            if (auto properties = node->Find("Properties70")) {
                for (auto property : properties->nodes) {
                    const auto info = Tools::Split(property->subData, ',');

                    if (info.empty())
                        continue;

                    if (info[0] == "Lcl Translation") {
                        model.Translation = vec3(Tools::CastToFloat(info[4]), Tools::CastToFloat(info[5]), Tools::CastToFloat(info[6]));
                    }
                    else if (info[0] == "Lcl Rotation") {
                        model.Rotation = vec3(Tools::CastToFloat(info[4]), Tools::CastToFloat(info[5]), Tools::CastToFloat(info[6]));
                    }
                    else if (info[0] == "Lcl Scaling") {
                        model.Scale = vec3(Tools::CastToFloat(info[4]), Tools::CastToFloat(info[5]), Tools::CastToFloat(info[6]));
                    }
                }
            }
            else
                FBX_ERROR("Loader::ParseModel() : model have not \"Properties70\" node!");

            break;
        }
        default:
            FBX_ERROR("Loader::ParseModel() : unsupported model version! Version: " + std::to_string(version));
            return {};
    }

    return model;
}

FbxLoader::NodeAttribute FbxLoader::Loader::ParseNodeAttribute(FbxLoader::Parser::Node *node) {
    return FbxLoader::NodeAttribute();
}

