#include "NRIFramework.h"
#include "interface.h"
#include "resource.h"
#include <array>
#include <log.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace MTX {
using uid = uuids::uuid;
using MtxTextureDesc = nri::TextureDesc;
using MtxBufferDesc = nri::BufferDesc;

const uint32_t MTX_MAX_POOL_CAPACITY = 4096;

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
  _objects[uid]->destroy(_interface);
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
  virtual ~BaseAllocator() = 0 {};

 private:
};

class TextureAllocator : public BaseAllocator {

 public:
  TextureAllocator();
  TextureAllocator(MTXInterface* interface);
  ~TextureAllocator() override;
  void setInterface(MTXInterface* interface) {
    MTX_ASSERT(interface);
    _gfxInterface = interface;
    _pool.setInterFace(interface);
  }
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
  BufferAllocator();
  BufferAllocator(MTXInterface* interface);
  ~BufferAllocator() override;
  void setInterface(MTXInterface* interface) {
    MTX_ASSERT(interface);
    _gfxInterface = interface;
    _pool.setInterFace(interface);
  }
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