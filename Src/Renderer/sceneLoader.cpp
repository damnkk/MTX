#include "sceneLoader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"
#include "sceneGraph.h"
#include <filesystem>
#include "Detex/detex.h"
#define M_PI       3.14159265358979323846 
namespace MTX {
void SceneLoader::destroy() {}

Mesh SceneLoader::convertAIMesh(aiMesh* mesh) {
  Mesh mtxMesh;
  mtxMesh.modelIdx = m_modelOffset;
  bool hasTexCoords = mesh->HasTextureCoords(0);
  bool hasTagent = mesh->HasTangentsAndBitangents();
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
    mat.setDiffuseTexture(m_sceneTextures.size());
    m_sceneTextures.push_back(diffTex);
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
    mat.setEmissiveTexture(m_sceneTextures.size());
    m_sceneTextures.push_back(diffTex);
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
    mat.setMRTexture(m_sceneTextures.size());
    m_sceneTextures.push_back(diffTex);
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
    mat.setAoTexture(m_sceneTextures.size());
    m_sceneTextures.push_back(diffTex);
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
    mat.setNormalTexture(m_sceneTextures.size());
    m_sceneTextures.push_back(diffTex);
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
  const unsigned int loadFlags = 0 | aiProcess_GenNormals | aiProcess_GenBoundingBoxes
      | aiProcess_CalcTangentSpace | aiProcess_FindInvalidData;
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
  m_modelOffset++;
  return m_sceneGraph;
}

inline float luminance(const uint8_t* color){
  return float(color[0]/255.0) * 0.2126f + float(color[1]/255.0) * 0.7512f + float(color[2]/255.0) * 0.0722f;
}

float buildAliasmap(const std::vector<float>& data,std::vector<EnvAccel>& accel){
  auto size = static_cast<uint32_t>(data.size());
  float sum = std::accumulate(data.begin(),data.end(),0.0f);
  auto fSize =  static_cast<float>(size);
  float inverseAverage = fSize/sum;
  for(uint32_t i = 0;i<size;++i){
    accel[i].q = data[i]*inverseAverage;
    accel[i].alias = i;
  }
  std::vector<uint32_t> partitionTable(size);
  uint32_t s = 0u;
  uint32_t large = size;
  for(uint32_t i = 0;i<size;++i){
    if(accel[i].q<1.0f)
    partitionTable[s++] = i;
    else
    partitionTable[--large] = i;
  }

  for(s = 0;s<large&&large<size;++s){
    const uint32_t smallEnergyIndex = partitionTable[s];
    const uint32_t highEnergyIndex = partitionTable[large];
    accel[smallEnergyIndex].alias = highEnergyIndex;
    const float differenceWithAverage = 1.0f-accel[smallEnergyIndex].q;
    accel[highEnergyIndex].q -=differenceWithAverage;
    if(accel[highEnergyIndex].q<1.0f){
      large++;
    }
  }
  return sum;
}

std::vector<EnvAccel> createEnvironmentAccel(uint8_t* pixels,uint32_t imageWidth,uint32_t imageHeight) {
  const uint32_t rx = imageWidth;
  const uint32_t ry = imageHeight;
  std::vector<EnvAccel> envAccel(rx*ry);
  std::vector<float> importanceData(rx*ry);
  float cosTheta0 = 1.0f;
  const float stepPhi = float(2.0*M_PI)/float(rx);
  const float stepTheta = float(M_PI)/float(ry);
  double total = 0;
  for(uint32_t y = 0;y<ry;++y){
    const float theta1 = float(y+1)*stepTheta; // 每个像素在y轴所映射的角度
    const float cosTheta1 = std::cos(theta1);// 该角度的余弦
    /*
    下面的代码计算的是立体角,可以理解为单位球面上一个极小的可微矩形区域占整个球面半径平方的比值(但也不能单纯理解为面积,因为具体是用
    这些角度所对应的余弦值在进行计算).这个小矩形的宽很好理解,就是一圈360度被HDR贴图的宽度像素个数所均分,而高则是y轴映射角度所对应的余弦值的变
    化量,这里你可以看到cosTheat0初值为1.0,而后不断被赋予cosTheta1的值,相当于每一行像素的y轴映射角度余弦和前一行像素的y轴映射角度余弦计算差值
    数学推导看:https://juejin.cn/post/7141771746562015246
    */
    const float area = (cosTheta0-cosTheta1)*stepPhi; // 立体角
    cosTheta0 = cosTheta1;
    for(uint32_t x = 0;x<rx;++x){
      const uint32_t idx  = y*rx+x;
      const uint32_t idx4 = idx*4;
      float cieLuminance = luminance(&pixels[idx4]);
      importanceData[idx] = area* std::max(float(pixels[idx4]/255.0), std::max(float(pixels[idx4 + 1]/255.0), float(pixels[idx4 + 2]/255.0)));
      total += cieLuminance;
    }
  }
  float average = static_cast<float>(total)/static_cast<float>(rx*ry);
  float integral = buildAliasmap(importanceData,envAccel);
  const float invEnvIntegral = 1.0f/integral;
  for(uint32_t i = 0;i<rx*ry;++i){
    const uint32_t idx4 = i*4;
    envAccel[i].pdf = std::max(pixels[idx4]/255.0,std::max(pixels[idx4+1]/255.0,pixels[idx4+2]/255.0))*invEnvIntegral;
  }
  for(uint32_t i = 0;i<rx*ry;++i){
    const uint32_t aliasIdx = envAccel[i].alias;
    envAccel[i].aliasPdf = envAccel[aliasIdx].pdf;
  }
  return envAccel;
}

void SceneLoader::addEnvTexture(std::string path) {
  if (path.empty() || !(std::filesystem::exists(path))) {
    MTX_WARN("Invalid ENV texture, check file path again!!")
    return;
  }
  MtxTextureAllocInfo texAllocInfo{};
  ::utils::Texture    utilTex;
  ::utils::LoadTexture(path, utilTex);
  texAllocInfo._name = utilTex.name;
  texAllocInfo._desc =
      nri::Texture2D(utilTex.GetFormat(), utilTex.GetWidth(), utilTex.GetHeight(),
                     utilTex.GetMipNum(), 1, nri::TextureUsageBits::SHADER_RESOURCE);
  texAllocInfo._sourceData = &utilTex;
  auto envTex = m_interface->allocateTexture(texAllocInfo);
  m_envTextures.push_back(envTex);

  // precalculat env map
  std::vector<EnvAccel> accel = createEnvironmentAccel(((detexTexture**)(utilTex.mips))[0]->data, utilTex.GetWidth(), utilTex.GetHeight());
  MtxBufferAllocInfo bufferInfo{};
  bufferInfo._data = accel.data();
  bufferInfo._memLocation = nri::MemoryLocation::DEVICE;
  bufferInfo._name = "envPdfMap";
  bufferInfo._desc.size = sizeof(EnvAccel)*accel.size();
  bufferInfo._desc.usageMask = nri::BufferUsageBits::SHADER_RESOURCE_STORAGE;
  bufferInfo._desc.structureStride = sizeof(EnvAccel);
  auto envPdfBuffer = m_interface->allocateBuffer(bufferInfo);
  m_envPdfBuffers.push_back(envPdfBuffer);
}

}// namespace MTX