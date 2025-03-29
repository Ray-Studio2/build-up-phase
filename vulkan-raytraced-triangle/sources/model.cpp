#include "model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
using std::cout, std::endl;

Model::Model() { }
Model::~Model() noexcept { }

void Model::load(const string& filePath) {
    mFilePath = filePath;

    tinyobj::attrib_t attribs;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;

    string err;
    if (!tinyobj::LoadObj(&attribs, &shapes, &materials, &err, filePath.c_str()))
        cout << err << endl;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            if (index.vertex_index >= 0) {
                vertex.pos.x = attribs.vertices[3 * index.vertex_index];
                vertex.pos.y = attribs.vertices[3 * index.vertex_index + 1];
                vertex.pos.z = attribs.vertices[3 * index.vertex_index + 2];
            }
            if (index.normal_index >= 0) {
                vertex.normal.x = attribs.normals[3 * index.normal_index];
                vertex.normal.y = attribs.normals[3 * index.normal_index + 1];
                vertex.normal.z = attribs.normals[3 * index.normal_index + 2];
            }
            if (index.texcoord_index >= 0) {
                // teapot model의 경우 min = 0, max = 2 이므로 임시로 보간

                vertex.tex.u = attribs.texcoords[2 * index.texcoord_index] / 2.0f;
                vertex.tex.v = attribs.texcoords[2 * index.texcoord_index + 1] / 2.0f;
            }

            mVertices.push_back(vertex);
            mIndices.push_back(static_cast<uint32_t>(mIndices.size()));
        }
    }

    /* // debug
    cout << "Vertices: " << mVertices.size() << endl;
    cout << "Faces: " << mIndices.size() / 3 << endl; */
}

vector<VkVertexInputBindingDescription> Model::bindingDesc() {
    return {
        {
            .binding = 0,
            .stride = sizeof(Model::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };
}
vector<VkVertexInputAttributeDescription> Model::attribDesc() {
    return {
        {   // Pos
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        },
        {   // Color (현재 임시로 Normal을 Color로 사용)
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = sizeof(Vec3)
        }
    };
}

vector<Model::Vertex> Model::vertices() const { return mVertices; }
vector<unsigned int> Model::indices() const { return mIndices; }

Transform& Model::transform() { return mTransform; }