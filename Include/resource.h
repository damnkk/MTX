#ifndef RESOURCE_H
#define RESOURCE_H
#include "glm/glm.hpp"
#include "Interface.h"
#include "uuid.h"
#include "NRIFramework.h"

namespace MTX {
struct MTXInterface;
using uid = uuids::uuid;
struct Vertex {
  glm::vec3 pos = glm::vec3(0.0f);
  alignas(16) glm::vec3 normal = glm::vec3(0.0f);
  alignas(16) glm::vec4 tangent = glm::vec4(0.0f);
  alignas(16) glm::vec2 texCoord = glm::vec2(0.0f);
  Vertex& operator=(Vertex& vert) {
    this->pos = vert.pos;
    this->normal = vert.normal;
    this->tangent = vert.tangent;
    this->texCoord = vert.texCoord;
    return *this;
  }
};
struct Object {
  Object() {}
  Object(const uid& uid) : _uid(uid){};
  virtual void destroy(MTXInterface* interface) = 0;
  const uid    _uid;
  std::string  name;
};

struct MtxTextureAllocInfo {
  nri::TextureDesc  _desc;
  std::string       _name;
  ::utils::Texture* _sourceData;
};

struct MtxTexture : public Object {
  MtxTexture() {}
  MtxTexture(const uid& uid) : Object(uid){};
  nri::Texture*    tex = nullptr;
  nri::Memory*     mem;
  nri::Descriptor* imageView = nullptr;
  nri::TextureDesc desc;

  void                  destroy(MTXInterface* interface) override;
  nri::Memory&          getMem() { return *mem; }
  nri::Dim_t            width() { return desc.width; }
  nri::Dim_t            height() { return desc.height; }
  nri::Format           format() { return desc.format; }
  nri::TextureType      type() { return desc.type; }
  nri::Dim_t            depth() { return desc.depth; }
  nri::Dim_t            mipNum() { return desc.mipNum; }
  nri::TextureUsageBits usage() { return desc.usageMask; }
  bool                  isValid() { return tex != nullptr; }
};

struct MtxAcceStructure : public Object {
  MtxAcceStructure() {}
  MtxAcceStructure(const uid& uid) : Object(uid){};
  void                            destroy(MTXInterface* interface) override;
  nri::AccelerationStructureDesc* accDesc = nullptr;
  nri::Memory*                    mem = nullptr;
  nri::AccelerationStructure*     acc = nullptr;
  nri::Descriptor*                accView = nullptr;
};
struct MtxBufferAllocInfo {
  nri::BufferDesc     _desc;
  std::string         _name;
  bool                _haveAccessStage = false;
  nri::MemoryLocation _memLocation = nri::MemoryLocation::DEVICE;
  void*               _data;
  void                setAccessStage(nri::AccessStage acst) {
    _accessStage = acst;
    _haveAccessStage = true;
  }
  nri::AccessStage getAccessStage() const { return _accessStage; }

 private:
  nri::AccessStage _accessStage = {};
};

struct MtxBuffer : public Object {
  MtxBuffer() {}
  MtxBuffer(const uid& uid) : Object(uid) {}
  nri::Buffer*         buf = nullptr;
  nri::Memory*         mem = nullptr;
  nri::Descriptor*     bufView = nullptr;
  nri::BufferDesc      desc;
  void                 destroy(MTXInterface* interface) override;
  nri::Buffer&         getBuf() { return *buf; }
  nri::Memory&         getMem() { return *mem; }
  uint64_t             size() { return desc.size; }
  nri::BufferUsageBits usage() { return desc.usageMask; }
  bool                 isValid() { return buf != nullptr; }
};

enum PipelineType { Graphics, Compute, RayTracing };

struct MtxPipelineAllocateInfo {
  void*        pipelineDesc;
  PipelineType pipelineType;
  std::string  name;
};

struct MtxPipeline : public Object {
  MtxPipeline() {}
  MtxPipeline(const uid& uid) : Object(uid) {}
  nri::Pipeline*       pipeline = nullptr;
  nri::PipelineLayout* pipelineLayout;
  void*                desc = nullptr;
  PipelineType         type = PipelineType::Graphics;
  void                 destroy(MTXInterface* interface) override;
  nri::Pipeline&       getPipeline() { return *pipeline; }
  bool                 isValid() { return pipeline != nullptr; }
};

struct FrameResource {
  void                                destroy(MTXInterface* interface);
  std::vector<nri::CommandAllocator*> _commandPools;
  nri::Fence*                         _fence = nullptr;
  std::vector<nri::CommandBuffer*>    _commandBuffers;
  MtxTexture                          _frameTexture;
};
}// namespace MTX

#endif//RESOURCE_H