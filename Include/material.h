#ifndef MATERIAL_H
#define MATERIAL_H
#include "renderer.h"
#include <glm/glm.hpp>

namespace MTX {
const uint32_t MAX_MATERIAL_TEXTURE_CNT = 32;
class Material {
 public:
  struct alignas(16) aliInt {
    int a;
  };

  struct TextureBind {
    int textureHandle = -1;
    int bindIdx = -1;
  };

  struct alignas(16) MaterialUniform {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::vec4 baseColorFactor = glm::vec4(glm::vec3(0.3f), 1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f);

    //envrotate, envExposure, envGamma
    alignas(16) glm::vec3 envFactor = glm::vec3(1.0f, .5f, 1.0f);
    //metallic, roughness, ao intensity
    alignas(16) glm::vec3 mrFactor = glm::vec3(1.0f, 1.0f, 1.0f);
    //normal intensity,emissive intensity,transMission
    alignas(16) glm::vec4 intensity = glm::vec4(1.0f);
    //use albedo,normal/mrao/emissive
    int textureUseSetting[4] = {1, 1, 1, 1};
    int textureIndices[MAX_MATERIAL_TEXTURE_CNT];
  } materialUniform;

  Material();
  void addTexture(int textureIdx);
  void deleteTexture(int bindIdx);
  void updateProgram();
  void setDiffuseTexture(int textureIdx);
  void setNormalTexture(int textureIdx);
  void setMRTexture(int textureIdx);
  void setEmissiveTexture(int textureIdx);
  void setAoTexture(int textureIdx);
  void setEnvTexture(int textureIdx);

  MTXRenderer*                                 m_renderer;
  std::unordered_map<std::string, TextureBind> m_textureMap;
};
}// namespace MTX

#endif