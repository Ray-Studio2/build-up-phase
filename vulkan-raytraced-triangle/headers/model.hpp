#pragma once

#include "components/transform.hpp"
#include "math/vec2.hpp"

#include "vulkan/vulkan.hpp"
#include <vector>
#include <string>

using std::vector;
using std::string;

class Model {
    Model(const Model&) = delete;
    Model(Model&&) noexcept = delete;

    Model& operator=(const Model&) = delete;
    Model& operator=(Model&&) noexcept = delete;

    public:
        struct alignas(16) Vertex {
            alignas(16) Vec3 pos;
            alignas(16) Vec3 normal;
            alignas(8)  Vec2 tex;
        };

    public:
        Model();
        ~Model() noexcept;

        void load(const string&);

        vector<VkVertexInputBindingDescription> bindingDesc();
        vector<VkVertexInputAttributeDescription> attribDesc();

        vector<Vertex> vertices() const;
        vector<unsigned int> indices() const;

        Transform& transform();

    private:
        vector<Vertex> mVertices;
        vector<unsigned int> mIndices;

        Transform mTransform;

        string mFilePath;
};