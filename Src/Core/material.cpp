#include "material.h"
#include "sceneLoader.h"
namespace MTX {
Material::Material()
    : m_textureMap({{"DiffuseTexture", {-1, 0}},
                    {"MetallicRoughnessTexture", {-1, 1}},
                    {"AOTexture", {-1, 2}},
                    {"NormalTexture", {-1, 3}},
                    {"EmissiveTexture", {-1, 4}},
                    {"EnvTexture", {-1, 5}}}) {}
void Material::setDiffuseTexture(int textureIdx) {
  m_textureMap["DiffuseTexture"].textureHandle = textureIdx;
}
void Material::setNormalTexture(int textureIdx) {
  m_textureMap["NormalTexture"].textureHandle = textureIdx;
}
void Material::setMRTexture(int textureIdx) {
  m_textureMap["MetallicRoughnessTexture"].textureHandle = textureIdx;
}
void Material::setEmissiveTexture(int textureIdx) {
  m_textureMap["EmissiveTexture"].textureHandle = textureIdx;
}
void Material::setAoTexture(int textureIdx) {
  m_textureMap["AOTexture"].textureHandle = textureIdx;
}
void Material::setEnvTexture(int textureIdx) {
  m_textureMap["EnvTexture"].textureHandle = textureIdx;
}
}// namespace MTX