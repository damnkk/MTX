#ifndef RESOURCE_H
#define RESOURCE_H
#include "NRIFramework.h"
#include "uuid.h"
namespace MTX {
struct MTXInterface;
using uid = uuids::uuid;

struct Object {
  Object() {}
  Object(const uid& uid) : _uid(uid){};
  virtual void destroy(MTXInterface* interface) = 0;
  const uid    _uid;
};

struct MtxTextureAllocInfo {
  uuids::uuid      _uid;
  nri::TextureDesc _desc;
  std::string      _name;
  utils::Texture*  _sourceData;
};

struct MtxTexture : public Object {
  MtxTexture() {}
  MtxTexture(const uid& uid) : Object(uid){};
  nri::Texture*    tex = nullptr;
  nri::Memory*     mem;
  nri::Descriptor* imageView;
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

struct MtxBufferAllocInfo {
  uuids::uuid     _uid;
  nri::BufferDesc _desc;
  std::string     _name;
  bool            _haveAccessStage = false;
  bool            _deviceOnly = true;
  void*           _data;
  void            setAccessStage(nri::AccessStage acst) {
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
  nri::BufferDesc      desc;
  void                 destroy(MTXInterface* interface) override;
  nri::Buffer&         getBuf() { return *buf; }
  nri::Memory&         getMem() { return *mem; }
  uint64_t             size() { return desc.size; }
  nri::BufferUsageBits usage() { return desc.usageMask; }
  bool                 isValid() { return buf != nullptr; }
};

struct FrameResource {
  void destroy(MTXInterface* interface);

  nri::CommandAllocator*           _commandPool = nullptr;
  nri::Fence*                      _fence = nullptr;
  std::vector<nri::CommandBuffer*> _commandBuffers;
  MtxTexture                       _frameTexture;
};
}// namespace MTX

#endif//RESOURCE_H