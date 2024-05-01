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

std::shared_ptr<MtxTexture>
TextureAllocator::allocateTexture(const MtxTextureAllocInfo& allocInfo) {
  // add a file existing validation
  MTX_ASSERT(!allocInfo._uid.is_nil());
  auto texture = _pool.allocate(allocInfo._uid);
  if (texture->isValid()) { return texture; }
  _gfxInterface->CreateTexture(_gfxInterface->getDevice(), allocInfo._desc, texture->tex);
  texture->desc = allocInfo._desc;
  nri::ResourceGroupDesc resourceGroupDesc{};
  resourceGroupDesc.textureNum = 1;
  resourceGroupDesc.textures = &(texture->tex);
  resourceGroupDesc.memoryLocation = nri::MemoryLocation::DEVICE;
  auto res = _gfxInterface->AllocateAndBindMemory(_gfxInterface->getDevice(), resourceGroupDesc,
                                                  &(texture->mem));
  MTX_CHECK(res);
  if (allocInfo._sourceData != nullptr) {
    std::array<nri::TextureSubresourceUploadDesc, 16> subresources;
    for (uint32_t mip = 0; mip < allocInfo._sourceData->mipNum; ++mip) {
      allocInfo._sourceData->GetSubresource(subresources[mip], mip);
    }

    nri::TextureUploadDesc textureData = {};
    textureData.texture = texture->tex;
    textureData.subresources = subresources.data();
    textureData.after = {nri::AccessBits::SHADER_RESOURCE, nri::Layout::SHADER_RESOURCE};
    _gfxInterface->UploadData(*(_gfxInterface->_transferQueue), &textureData, 1, nullptr, 0);
  }

  std::string realName = "name" + std::to_string(_pool.size());
  _gfxInterface->SetTextureDebugName(
      *(texture->tex), allocInfo._name.empty() ? realName.c_str() : allocInfo._name.c_str());
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

std::shared_ptr<MtxBuffer> BufferAllocator::allocateBuffer(const MtxBufferAllocInfo& allocInfo) {
  std::string tempName = allocInfo._name;
  if (allocInfo._name.empty()) { tempName = "name" + std::to_string(_pool.size()); }
  auto uuid = allocInfo._uid;
  MTX_ASSERT(!uuid.is_nil());
  std::shared_ptr<MtxBuffer> buffer = _pool.allocate(uuid);
  buffer->desc = allocInfo._desc;
  _gfxInterface->CreateBuffer(_gfxInterface->getDevice(), buffer->desc, buffer->buf);
  nri::ResourceGroupDesc resourceGroupDesc = {};
  resourceGroupDesc.bufferNum = 1;
  resourceGroupDesc.buffers = &(buffer->buf);
  auto res = _gfxInterface->AllocateAndBindMemory(_gfxInterface->getDevice(), resourceGroupDesc,
                                                  &(buffer->mem));
  if (allocInfo._data) {
    nri::BufferUploadDesc uploadDesc{};
    uploadDesc.buffer = buffer->buf;
    uploadDesc.bufferOffset = 0;
    uploadDesc.data = allocInfo._data;
    uploadDesc.dataSize = allocInfo._desc.size;
    if (allocInfo._haveAccessStage) {
      uploadDesc.after = allocInfo.getAccessStage();
    } else {
      uploadDesc.after = {utils::bufferUsageToAccess(allocInfo._desc.usageMask)};
    }
  };
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