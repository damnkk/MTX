#ifndef SCENELOADER_H
#define SCENELOADER_H
#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/pbrmaterial.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "glm/glm.hpp"
#include "material.h"
#include "resource.h"
#include <filesystem>
#include <vector>

namespace MTX {
struct SceneGraph;

const uint32_t MAX_SCENE_TEXTURE_CNT = 4096;
using u32 = uint32_t;

struct BoundingBox {
  glm::vec3 m_min = glm::vec3(0.0f);
  glm::vec3 m_max = glm::vec3(0.0f);
  BoundingBox() = default;
  BoundingBox(glm::vec3 min, glm::vec3 max)
      : m_min(glm::min(min, max)), m_max(glm::max(min, max)) {}
  BoundingBox(glm::vec3* point, u32 pointCount) {
    glm::vec3 vmax(std::numeric_limits<float>::max());
    glm::vec3 vmin(std::numeric_limits<float>::min());
    for (u32 i = 0; i < pointCount; ++i) {
      vmin = glm::min(vmin, point[i]);
      vmax = glm::max(vmax, point[i]);
    }
    m_min = vmin;
    m_max = vmax;
  }

  glm::vec3 getSize() const {
    return glm::vec3(m_max.x - m_min.x, m_max.y - m_min.y, m_max.z - m_min.z);
  }

  glm::vec3 getCenter() const {
    return 0.5f * glm::vec3(m_max.x + m_min.x, m_max.y + m_min.y, m_max.z + m_min.z);
  }

  void transform(const glm::mat4& t) {
    glm::vec3 corners[] = {
        glm::vec3(m_min.x, m_min.y, m_min.z), glm::vec3(m_min.x, m_max.y, m_min.z),
        glm::vec3(m_min.x, m_min.y, m_max.z), glm::vec3(m_min.x, m_max.y, m_max.z),
        glm::vec3(m_max.x, m_min.y, m_min.z), glm::vec3(m_max.x, m_max.y, m_min.z),
        glm::vec3(m_max.x, m_min.y, m_max.z), glm::vec3(m_max.x, m_max.y, m_max.z),
    };
    for (auto& v : corners) { v = glm::vec3(t * glm::vec4(v, 1.0f)); }
    *this = BoundingBox(corners, 8);
  }

  BoundingBox getTransformed(const glm::mat4& t) {
    BoundingBox b = *this;
    b.transform(t);
    return b;
  }

  void combinePoint(const glm::vec3& p) {
    m_max = glm::max(m_max, p);
    m_min = glm::min(m_min, p);
  }
};

struct Mesh {
  u32                        modelIdx = 0;
  u32                        indexOffset = 0;
  u32                        vertexOffset = 0;
  u32                        vertexCount = 0;
  u32                        indexCount = 0;
  std::shared_ptr<MtxBuffer> matUniformBuffer;
  //for ray tracing
};
class SceneLoader {
 public:
  SceneLoader(){};
  SceneLoader(MTX::MTXInterface* interface) : m_interface(interface) {}
  void                        destroy();
  std::shared_ptr<SceneGraph> loadScene(std::string path);
  Material convertAIMaterialToDescription(const aiMaterial* material, std::string basePath);

  Mesh convertAIMesh(aiMesh* mesh);
  void executeScene(std::shared_ptr<SceneGraph> scene);
  void executeSceneRT(std::shared_ptr<SceneGraph> scene);

 public:
  std::vector<std::shared_ptr<MtxTexture>>& getSceneTextures() { return m_sceneTextures; }
  std::shared_ptr<SceneGraph>               getSceneGraph() { return m_sceneGraph; }
  std::vector<Vertex>&                      getVertices() { return m_sceneVertices; }
  std::vector<uint32_t>&                    getIndices() { return m_sceneIndices; }
  std::vector<Mesh>&                        getMeshes() { return m_meshes; }
  std::vector<Material>&                    getMaterials() { return m_materials; }
  std::vector<uint32_t>                     getPrimitMatIndices() { return m_primitMatIndices; }

 protected:
  void traverse(const aiScene* sourceScene, SceneGraph& sceneGraph, aiNode* node, int parent,
                int level);

 private:
  Assimp::Importer                         m_modelImporter;
  std::shared_ptr<SceneGraph>              m_sceneGraph;
  std::vector<Mesh>                        m_meshes;
  int                                      m_meshOffset = 0;
  std::vector<Material>                    m_materials;
  int                                      m_materialOffset = 0;
  std::vector<std::shared_ptr<MtxTexture>> m_sceneTextures;
  int                                      m_sceneTextureOffset = 0;
  //geom data
  /*
  *定义一个modelOffset,创建GPU数据的时候将属于同一个模型的mesh整体创建为一个整体,但是不同模型的vertexOffset 
  *和idx应该重新计算.
  */
  uint32_t              m_modelOffset = 0;
  std::vector<Vertex>   m_sceneVertices;
  uint32_t              m_vertexOffset = 0;
  std::vector<uint32_t> m_sceneIndices;
  uint32_t              m_indexOffset = 0;
  std::vector<uint32_t> m_primitMatIndices;

  std::vector<BoundingBox> m_boundingBoxes;
  int                      m_boundingBoxOffset = 0;
  MTXInterface*            m_interface;
};
}// namespace MTX

#endif//SCENELOADER_H