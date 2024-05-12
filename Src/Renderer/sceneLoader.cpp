#include "sceneLoader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"
#include "sceneGraph.h"
#include <filesystem>

namespace MTX {
void SceneLoader::destroy() {}

Mesh SceneLoader::convertAIMesh(aiMesh* mesh) {
  Mesh mtxMesh;
  bool hasTexCoords = mesh->HasTextureCoords(0);
  bool hasTagent = mesh->HasTangentsAndBitangents();
  // std::vector<float> srcVertices;
  // std::vector<u32>   srcIndices;
  // std::vector<Vertex> vertices;
  // vertices.reserve(mesh->mNumVertices);
  mtxMesh.vertexOffset = m_sceneVertices.size();
  mtxMesh.indexOffset = m_sceneIndices.size();
  for (int i = 0; i < mesh->mNumVertices; ++i) {
    const aiVector3D v = mesh->mVertices[i];
    const aiVector3D n = mesh->mNormals[i];
    const aiVector3D t = hasTexCoords ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f);
    const aiVector3D tangent = hasTagent ? mesh->mTangents[i] : aiVector3D(0.0f);
    Vertex           vert;
    vert.pos = glm::vec3(v.x, v.y, v.z);
    vert.normal = glm::vec3(n.x, n.y, n.z);
    vert.texCoord = glm::vec2(t.x, 1.0f - t.y);
    vert.tangent = glm::vec4(tangent.x, tangent.y, tangent.z, 1.0f);
    m_sceneVertices.push_back(vert);
  }

  for (size_t i = 0; i < mesh->mNumFaces; ++i) {
    if (mesh->mFaces[i].mNumIndices != 3) continue;
    for (size_t j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
      m_sceneIndices.push_back(mesh->mFaces[i].mIndices[j]);
    }
  }
  mtxMesh.indexCount = mesh->mNumFaces * 3;
  mtxMesh.vertexCount = mesh->mNumVertices;

  for (int i = 0; i < mesh->mNumFaces; ++i) {
    m_primitMatIndices.push_back(m_meshOffset + mesh->mMaterialIndex);
  }
  return mtxMesh;
}

glm::mat4 toMat4(const aiMatrix4x4& from) {
  glm::mat4 to;
  to[0][0] = (float) from.a1;
  to[0][1] = (float) from.b1;
  to[0][2] = (float) from.c1;
  to[0][3] = (float) from.d1;
  to[1][0] = (float) from.a2;
  to[1][1] = (float) from.b2;
  to[1][2] = (float) from.c2;
  to[1][3] = (float) from.d2;
  to[2][0] = (float) from.a3;
  to[2][1] = (float) from.b3;
  to[2][2] = (float) from.c3;
  to[2][3] = (float) from.d3;
  to[3][0] = (float) from.a4;
  to[3][1] = (float) from.b4;
  to[3][2] = (float) from.c4;
  to[3][3] = (float) from.d4;
  return to;
}

void SceneLoader::traverse(const aiScene* sourceScene, SceneGraph& sceneGraph, aiNode* node,
                           int parent, int level) {
  int newNodeId;
  //sceneGraph.maxLevel = std::max(sceneGraph.maxLevel, level);
  if (node->mName.length != 0) {
    newNodeId = sceneGraph.addNode(parent, level, std::string(node->mName.C_Str()));
  } else {
    newNodeId = sceneGraph.addNode(parent, level);
  }
  for (size_t i = 0; i < node->mNumMeshes; ++i) {
    int newSubNodeID = sceneGraph.addNode(
        newNodeId, level + 1, std::string(node->mName.C_Str()) + "_Mesh_" + std::to_string(i));
    int mesh = node->mMeshes[i];
    sceneGraph.m_meshMap[newSubNodeID] = m_meshOffset + mesh;
    sceneGraph.m_boundingBoxMap[newSubNodeID] = m_boundingBoxOffset + mesh;
    sceneGraph.m_materialMap[newSubNodeID] =
        sourceScene->mMeshes[mesh]->mMaterialIndex + m_materialOffset;
    sceneGraph.m_globalTransforms[newSubNodeID] = glm::mat4(1.0f);
    sceneGraph.m_localTransforms[newSubNodeID] = glm::mat4(1.0f);
  }
  sceneGraph.m_globalTransforms[newNodeId] = glm::mat4(1.0f);
  sceneGraph.m_localTransforms[newNodeId] = toMat4(node->mTransformation);
  glm::vec3& rotateRecord = sceneGraph.m_rotateRecord[newNodeId];
  glm::extractEulerAngleXYZ(sceneGraph.m_localTransforms[newNodeId], rotateRecord.x, rotateRecord.y,
                            rotateRecord.z);

  for (u32 n = 0; n < node->mNumChildren; ++n) {
    traverse(sourceScene, sceneGraph, node->mChildren[n], newNodeId, level + 1);
  }
}

int      EmptyNameCount = 0;
Material SceneLoader::convertAIMaterialToDescription(const aiMaterial* aiMat,
                                                     std::string       basePath) {
  Material  mat;
  aiColor4D color;
  if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
    mat.materialUniform.emissiveFactor = glm::vec3(color.r, color.g, color.b);
  }
  if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
    mat.materialUniform.baseColorFactor = glm::vec4(color.r, color.g, color.b, color.a);
    if (mat.materialUniform.baseColorFactor.w > 1.0f) mat.materialUniform.baseColorFactor.w = 1.0f;
    if (glm::length(glm::vec3(color.r, color.g, color.b)) == 0.0f) {
      aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_SPECULAR, &color);
      mat.materialUniform.baseColorFactor = glm::vec4(color.r, color.g, color.b, color.a);
    }
  }
  if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &color) == aiReturn_SUCCESS) {
    mat.materialUniform.emissiveFactor.r += color.r;
    mat.materialUniform.emissiveFactor.g += color.g;
    mat.materialUniform.emissiveFactor.b += color.b;
  }
  float tmp;
  if (aiGetMaterialFloat(aiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &tmp)
      == AI_SUCCESS) {
    mat.materialUniform.mrFactor.x = tmp;
  }
  if (aiGetMaterialFloat(aiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &tmp)
      == AI_SUCCESS) {
    mat.materialUniform.mrFactor.y = tmp;
  }
  aiString         texPath;
  aiTextureMapping Mapping;
  unsigned int     UVIndex = 0;
  float            Blend = 1.0f;
  aiTextureOp      TextureOp = aiTextureOp_Add;
  aiTextureMapMode TextureMapMode[2] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
  unsigned int     TextureFlags = 0;

  MtxTextureAllocInfo texAllocInfo{};
  ::utils::Texture    utilTex;
  if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
    std::string diffusePath = basePath + '/' + std::string(texPath.C_Str());
    ::utils::LoadTexture(diffusePath, utilTex);
    texAllocInfo._desc =
        nri::Texture2D(utilTex.GetFormat(), utilTex.GetWidth(), utilTex.GetHeight(),
                       utilTex.GetMipNum(), 1, nri::TextureUsageBits::SHADER_RESOURCE);
    texAllocInfo._name = std::string(texPath.C_Str());
    texAllocInfo._sourceData = &utilTex;
    auto diffTex = m_interface->allocateTexture(texAllocInfo);
    m_sceneTextures.push_back(diffTex);
    mat.setDiffuseTexture(m_sceneTextures.size());
  }
  if (aiMat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
    std::string emissivePath = basePath + '/' + std::string(texPath.C_Str());
    ::utils::LoadTexture(emissivePath, utilTex);
    texAllocInfo._desc =
        nri::Texture2D(utilTex.GetFormat(), utilTex.GetWidth(), utilTex.GetHeight(),
                       utilTex.GetMipNum(), 1, nri::TextureUsageBits::SHADER_RESOURCE);
    texAllocInfo._name = std::string(texPath.C_Str());
    texAllocInfo._sourceData = &utilTex;
    auto diffTex = m_interface->allocateTexture(texAllocInfo);
    m_sceneTextures.push_back(diffTex);
    mat.setEmissiveTexture(m_sceneTextures.size());
  }
  if (aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS) {
    std::string mrTex = basePath + '/' + std::string(texPath.C_Str());
    ::utils::LoadTexture(mrTex, utilTex);
    texAllocInfo._desc =
        nri::Texture2D(utilTex.GetFormat(), utilTex.GetWidth(), utilTex.GetHeight(),
                       utilTex.GetMipNum(), 1, nri::TextureUsageBits::SHADER_RESOURCE);
    texAllocInfo._name = std::string(texPath.C_Str());
    texAllocInfo._sourceData = &utilTex;
    auto diffTex = m_interface->allocateTexture(texAllocInfo);
    m_sceneTextures.push_back(diffTex);
    mat.setMRTexture(m_sceneTextures.size());
  }
  if (aiMat->GetTexture(aiTextureType_LIGHTMAP, 0, &texPath) == AI_SUCCESS) {
    std::string oaTex = basePath + '/' + std::string(texPath.C_Str());
    ::utils::LoadTexture(oaTex, utilTex);
    texAllocInfo._desc =
        nri::Texture2D(utilTex.GetFormat(), utilTex.GetWidth(), utilTex.GetHeight(),
                       utilTex.GetMipNum(), 1, nri::TextureUsageBits::SHADER_RESOURCE);
    texAllocInfo._name = std::string(texPath.C_Str());
    texAllocInfo._sourceData = &utilTex;
    auto diffTex = m_interface->allocateTexture(texAllocInfo);
    m_sceneTextures.push_back(diffTex);
    mat.setAoTexture(m_sceneTextures.size());
  }
  if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
    std::string normPath = basePath + '/' + std::string(texPath.C_Str());
    ::utils::LoadTexture(normPath, utilTex);
    texAllocInfo._desc =
        nri::Texture2D(utilTex.GetFormat(), utilTex.GetWidth(), utilTex.GetHeight(),
                       utilTex.GetMipNum(), 1, nri::TextureUsageBits::SHADER_RESOURCE);
    texAllocInfo._name = std::string(texPath.C_Str());
    texAllocInfo._sourceData = &utilTex;
    auto diffTex = m_interface->allocateTexture(texAllocInfo);
    m_sceneTextures.push_back(diffTex);
    mat.setNormalTexture(m_sceneTextures.size());
  }
  return mat;
}

std::shared_ptr<SceneGraph> SceneLoader::loadScene(std::string path) {
  if (!std::filesystem::exists(path)) { MTX_ERROR("The file :{} is not exist, check again", path); }
  if (!m_sceneGraph) {
    m_sceneGraph = std::make_shared<SceneGraph>();
    int rootNodeId = m_sceneGraph->addNode(-1, 0, "RootSceneNode");
  }
  const size_t      pathSeparator = path.find_last_of("/\\");
  const std::string basePath =
      (pathSeparator != std::string::npos) ? path.substr(0, pathSeparator + 1) : "";
  if (basePath.empty()) MTX_WARN("Model base path is null, this may lead to potential mistake");
  const unsigned int loadFlags =
      0 | aiProcess_GenNormals | aiProcess_GenBoundingBoxes | aiProcess_FindInvalidData;
  MTX_INFO("Loading model from path {}", path);
  const aiScene* scene = m_modelImporter.ReadFile(path.c_str(), loadFlags);
  if (!scene || !scene->HasMeshes()) {
    MTX_WARN("This model maybe invalid or have no meshes in the scene");
    return nullptr;
  }

  {
    this->m_meshOffset = m_meshes.size();
    this->m_materialOffset = m_materials.size();
    this->m_boundingBoxOffset = m_boundingBoxes.size();
  }
  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    Mesh mesh = convertAIMesh(scene->mMeshes[i]);
    m_meshes.push_back(mesh);
    auto& AABB = scene->mMeshes[i]->mAABB;
    m_boundingBoxes.emplace_back(glm::vec3(AABB.mMax.x, AABB.mMax.y, AABB.mMax.z),
                                 glm::vec3(AABB.mMin.x, AABB.mMin.y, AABB.mMin.z));
  }

  for (u32 i = 0; i < scene->mNumMaterials; ++i) {
    aiMaterial* assimpMat = scene->mMaterials[i];
    m_materials.push_back(convertAIMaterialToDescription(assimpMat, basePath));
  }

  traverse(scene, *m_sceneGraph, scene->mRootNode, 0, 1);
  return m_sceneGraph;
}

}// namespace MTX