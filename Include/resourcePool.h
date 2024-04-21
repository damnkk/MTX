#include "NRIFramework.h"
#include "extensions/NRIRayTracing.h"
#include "interface.h"
#include <array>
#include <log.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <uuid.h>
#include <vector>

namespace MTX {
using MtxTextureDesc = nri::TextureDesc;
using MtxBufferDesc = nri::BufferDesc;

const uint32_t MTX_MAX_POOL_CAPACITY = 4096;
using uid = uuids::uuid;

struct Object {
  Object(uid& uid) : _uid(uid){};
  Object(const Object& obj) = delete;
  virtual void destroy(MTXInterface* interface) = 0;
  const uid    _uid;
};

struct MtxTexture : public Object {
  MtxTexture(uid& uid) : Object(uid){};
  MtxTexture(const MtxTexture&) = delete;
  nri::Texture*         tex = nullptr;
  nri::Memory*          mem;
  nri::TextureDesc      desc;
  void                  destroy(MTXInterface* interface) override;
  nri::Dim_t            width() { return desc.width; }
  nri::Dim_t            height() { return desc.height; }
  nri::Format           format() { return desc.format; }
  nri::TextureType      type() { return desc.type; }
  nri::Dim_t            depth() { return desc.depth; }
  nri::Dim_t            mipNum() { return desc.mipNum; }
  nri::TextureUsageBits usage() { return desc.usageMask; }
  bool                  isValid() { return tex != nullptr; }
};

struct MtxBuffer : public Object {
  MtxBuffer(uid& uid) : Object(uid) {}
  MtxBuffer(const MtxBuffer&) = delete;
  nri::Buffer*         buf = nullptr;
  nri::Memory*         mem = nullptr;
  nri::BufferDesc      desc;
  nri::Buffer&         getBuf() { return *buf; }
  void                 destroy(MTXInterface* interface) override;
  uint64_t             size() { return desc.size; }
  nri::BufferUsageBits usage() { return desc.usageMask; }
  bool                 isValid() { return buf != nullptr; }
};
class TextureAllocator;
class BufferAllocator;

template<typename T>
class ResourcePool {
 public:
  ResourcePool(){};
  ~ResourcePool();
  std::shared_ptr<T> allocate(uid& uid);
  void               release(const uid& uid);
  std::shared_ptr<T> accessObject(const uid& uid);
  size_t             size() { return _objects.size(); }

  void setInterFace(MTXInterface* interface) { _interface = interface; }

 private:
  std::unordered_map<uid, std::shared_ptr<T>> _objects;
  MTXInterface*                               _interface = nullptr;
  std::mutex                                  _mutex;
};
template<typename T>
ResourcePool<T>::~ResourcePool() {

  // for (auto [first, second] : _objects) second->destroy(_interface);
  _objects.clear();
}

template<typename T>
std::shared_ptr<T> ResourcePool<T>::allocate(uid& uid) {
  std::unique_lock<std::mutex> lock(_mutex);
  if (_objects.size() >= MTX_MAX_POOL_CAPACITY) {
    MTX_WARN("Resource Pool can not alloc more object, check your code.")
    return nullptr;
  }
  if (_objects.find(uid) != _objects.end()) {
    MTX_INFO("There is already a {} with the same uuid, a duplicate object will not allocated.",
             typeid(T).name())
    return _objects[uid];
  }
  auto ptr = std::make_shared<T>(uid);
  _objects[uid] = ptr;
  return ptr;
}

template<typename T>
void ResourcePool<T>::release(const uid& uid) {
  std::unique_lock<std::mutex> lock(_mutex);
  if (_objects.find(uid) == _objects.end()) {
    MTX_WARN("You are trying to release an invalid {} object", typeid(T).name())
    return;
  }
  //_objects[uid]->destroy(_interface);
  _objects.erase(uid);
  return;
}

template<typename T>
std::shared_ptr<T> ResourcePool<T>::accessObject(const uid& uid) {
  return _objects[uid];
}

class BaseAllocator {
 public:
  BaseAllocator() {}
  virtual ~BaseAllocator() = 0;

 private:
};

class TextureAllocator : public BaseAllocator {

 public:
  TextureAllocator(MTXInterface* interface);
  ~TextureAllocator() override;
  std::shared_ptr<MtxTexture> allocateTexture(std::string path);
  std::shared_ptr<MtxTexture> allocateTexture(MtxTextureDesc& desc, std::string name = "");
  void                        releaseTexture(std::shared_ptr<MtxTexture> pTexture);
  void                        releaseTexture(uuids::uuid uuid);

 private:
  uuids::uuid_name_generator _uuidCreater;
  ResourcePool<MtxTexture>   _pool;
  MTXInterface*              _gfxInterface;
};

class BufferAllocator : public BaseAllocator {
 public:
  BufferAllocator(MTXInterface* interface);
  ~BufferAllocator() override;
  std::shared_ptr<MtxBuffer> allocateBuffer(MtxBufferDesc& desc, bool deviceOnly,
                                            std::string name = "", void* data = nullptr);
  void                       releaseBuffer(std::shared_ptr<MtxBuffer> pBuffer);
  void                       releaseBuffer(uuids::uuid iuid);

 private:
  uuids::uuid_name_generator _uuidCreater;
  ResourcePool<MtxBuffer>    _pool;
  MTXInterface*              _gfxInterface;
};

}// namespace MTX