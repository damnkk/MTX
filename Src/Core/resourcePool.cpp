#include "resourcePool.h"
#include "mtxUtils.h"

namespace MTX {

TextureAllocator::TextureAllocator()
    : _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {}

TextureAllocator::TextureAllocator(MTXInterface* interface)
    : _gfxInterface(interface),
      _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {
  _gfxInterface = interface;
  _pool.setInterFace(interface);
}

TextureAllocator::~TextureAllocator() {}

std::shared_ptr<MtxTexture> TextureAllocator::allocateTexture(std::string path) {
  // add a file existing validation
  auto uuid = _uuidCreater(path);
  MTX_ASSERT(!uuid.is_nil());
  auto texture = _pool.allocate(uuid);
  if (texture->isValid()) { return texture; }
  nri::TextureDesc desc{};
  ::utils::Texture tes;
  ::utils::LoadTexture(path, tes);
  if (tes.depth == 1) {
    desc = nri::Texture2D(tes.format, tes.width, tes.height, tes.mipNum, tes.arraySize);
  } else if (tes.depth > 1) {
    desc = nri::Texture3D(tes.format, tes.width, tes.height, tes.depth, tes.mipNum);
  }
  _gfxInterface->CreateTexture(_gfxInterface->getDevice(), desc, texture->tex);
  texture->desc = desc;

  nri::ResourceGroupDesc resourceGroupDesc{};
  resourceGroupDesc.textureNum = 1;
  resourceGroupDesc.textures = &(texture->tex);
  resourceGroupDesc.memoryLocation = nri::MemoryLocation::DEVICE;
  auto res = _gfxInterface->AllocateAndBindMemory(_gfxInterface->getDevice(), resourceGroupDesc,
                                                  &(texture->mem));
  MTX_ASSERT(res == nri::Result::SUCCESS);

  std::array<nri::TextureSubresourceUploadDesc, 16> subresources;
  for (uint32_t mip = 0; mip < tes.mipNum; ++mip) { tes.GetSubresource(subresources[mip], mip); }

  nri::TextureUploadDesc textureData = {};
  textureData.texture = texture->tex;
  textureData.subresources = subresources.data();
  textureData.after = {nri::AccessBits::SHADER_RESOURCE, nri::Layout::SHADER_RESOURCE};

  _gfxInterface->UploadData(*(_gfxInterface->_transferQueue), &textureData, 1, nullptr, 0);
  _gfxInterface->SetTextureDebugName(*(texture->tex), path.c_str());
  return texture;
}

std::shared_ptr<MtxTexture> TextureAllocator::allocateTexture(MtxTextureDesc& desc,
                                                              std::string     name) {
  std::string tempName = name;
  if (name.empty()) { tempName = "name" + std::to_string(_pool.size()); }
  auto uuid = _uuidCreater(name);
  MTX_ASSERT(!uuid.is_nil());
  auto texture = _pool.allocate(uuid);
  if (texture->isValid()) { return texture; }
  _gfxInterface->CreateTexture(_gfxInterface->getDevice(), desc, texture->tex);
  texture->desc = desc;
  _gfxInterface->SetTextureDebugName(*(texture->tex), name.c_str());
  return texture;
}

void TextureAllocator::releaseTexture(std::shared_ptr<MtxTexture> texture) {
  //texture->destroy(_gfxInterface);
  _pool.release(texture->_uid);
}

void TextureAllocator::releaseTexture(uuids::uuid uuid) {
  // auto resPtr = _pool.accessObject(uuid);
  // resPtr->destroy(_gfxInterface);
  _pool.release(uuid);
}

BufferAllocator::BufferAllocator()
    : _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {}

BufferAllocator::BufferAllocator(MTXInterface* interface)
    : _gfxInterface(interface),
      _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {
  _gfxInterface = interface;
  _pool.setInterFace(interface);
}
BufferAllocator::~BufferAllocator() {}

std::shared_ptr<MtxBuffer> BufferAllocator::allocateBuffer(MtxBufferDesc& desc, bool deviceOnly,
                                                           std::string name, void* data) {
  std::string tempName = name;
  if (name.empty()) { tempName = "name" + std::to_string(_pool.size()); }
  auto uuid = _uuidCreater(name);
  MTX_ASSERT(!uuid.is_nil());
  std::shared_ptr<MtxBuffer> buffer = _pool.allocate(uuid);
  buffer->desc = desc;
  _gfxInterface->CreateBuffer(_gfxInterface->getDevice(), desc, buffer->buf);
  nri::MemoryDesc memDesc = {};
  _gfxInterface->GetBufferMemoryInfo(
      *(buffer->buf), deviceOnly ? nri::MemoryLocation::DEVICE : nri::MemoryLocation::HOST_UPLOAD,
      memDesc);
  auto res = _gfxInterface->AllocateMemory(_gfxInterface->getDevice(), memDesc.type, memDesc.size,
                                           buffer->mem);
  MTX_ASSERT(res == nri::Result::SUCCESS);
  nri::BufferMemoryBindingDesc bindDesc{buffer->mem, buffer->buf};
  res = _gfxInterface->BindBufferMemory(_gfxInterface->getDevice(), &bindDesc, 1);
  MTX_ASSERT(res == nri::Result::SUCCESS);
  if (data) {
    if (!deviceOnly) {
      void* mappedMem = _gfxInterface->MapBuffer(*(buffer->buf), 0, buffer->size());
      memcpy(mappedMem, data, buffer->size());
      _gfxInterface->UnmapBuffer(buffer->getBuf());
    } else {
      nri::BufferUploadDesc uploadDesc = {};
      uploadDesc.buffer = buffer->buf;
      uploadDesc.bufferOffset = 0;
      uploadDesc.data = data;
      uploadDesc.dataSize = desc.size;
      uploadDesc.after = {nri::AccessBits::UNKNOWN};
      _gfxInterface->UploadData(_gfxInterface->getComputeQueue(), nullptr, 0, &uploadDesc, 1);
    }
  }
  _gfxInterface->SetBufferDebugName(buffer->getBuf(), tempName.c_str());
  return buffer;
}

void BufferAllocator::releaseBuffer(uuids::uuid uid) {
  auto resPtr = _pool.accessObject(uid);
  //resPtr->destroy(_gfxInterface);
  _pool.release(uid);
}

void BufferAllocator::releaseBuffer(std::shared_ptr<MtxBuffer> mtxBuffer) {
  //mtxBuffer->destroy(_gfxInterface);
  _pool.release(mtxBuffer->_uid);
}

}// namespace MTX