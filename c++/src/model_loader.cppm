module;

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>

export module model_loader;

export struct ObjVertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  bool operator==(const ObjVertex& other) const {
    return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
  }
};

namespace {
struct ObjVertexHash {
  std::size_t operator()(const ObjVertex& v) const noexcept {
    return ((std::hash<glm::vec3>()(v.pos) ^ (std::hash<glm::vec3>()(v.color) << 1)) >> 1) ^
           (std::hash<glm::vec2>()(v.tex_coord) << 1);
  }
};
}  // namespace

export struct ObjModel {
  std::vector<ObjVertex> vertices;
  std::vector<std::uint32_t> indices;
};

export ObjModel LoadObjModel(const std::string& path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
    throw std::runtime_error(warn + err);
  }

  ObjModel model;
  std::unordered_map<ObjVertex, std::uint32_t, ObjVertexHash> unique_vertices;

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      ObjVertex vertex{};
      vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2],
      };
      vertex.tex_coord = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
      };
      vertex.color = {1.0f, 1.0f, 1.0f};

      auto it = unique_vertices.find(vertex);
      if (it == unique_vertices.end()) {
        auto new_index = static_cast<std::uint32_t>(model.vertices.size());
        unique_vertices.emplace(vertex, new_index);
        model.vertices.push_back(vertex);
        model.indices.push_back(new_index);
      } else {
        model.indices.push_back(it->second);
      }
    }
  }

  return model;
}
