#include "resourcePool.h"
#include "mtxUtils.h"

namespace MTX {
std::atomic_uint64_t UUID_COUNT = 0;

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
  std::string tempName = allocInfo._name;
  if (tempName.empty()) { tempName = "texture" + std::to_string(_pool.size()); }
  auto uid = _uuidCreater(std::to_string(UUID_COUNT++).c_str());
  MTX_ASSERT(!uid.is_nil());
  auto texture = _pool.allocate(uid);
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

  _gfxInterface->SetTextureDebugName(*(texture->tex), tempName.c_str());
  texture->name = tempName;
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
  auto uid = _uuidCreater(std::to_string(UUID_COUNT++).c_str());
  MTX_ASSERT(!uid.is_nil());
  std::shared_ptr<MtxBuffer> buffer = _pool.allocate(uid);
  buffer->desc = allocInfo._desc;
  _gfxInterface->CreateBuffer(_gfxInterface->getDevice(), buffer->desc, buffer->buf);
  // nri::ResourceGroupDesc resourceGroupDesc = {};
  // resourceGroupDesc.bufferNum = 1;
  // resourceGroupDesc.buffers = &(buffer->buf);
  // resourceGroupDesc.memoryLocation = allocInfo._memLocation;
  // auto res = _gfxInterface->AllocateAndBindMemory(_gfxInterface->getDevice(), resourceGroupDesc,
  //                                                 &(buffer->mem));
  nri::MemoryDesc bufferMemoryDesc = {};
  _gfxInterface->GetBufferMemoryDesc(_gfxInterface->getDevice(),allocInfo._desc,allocInfo._memLocation,bufferMemoryDesc);
  MTX_CHECK(_gfxInterface->AllocateMemory(_gfxInterface->getDevice(), bufferMemoryDesc.type,
                                          bufferMemoryDesc.size, buffer->mem));
  const nri::BufferMemoryBindingDesc bufferMemoryBindingDesc = {buffer->mem, buffer->buf, 0};
  MTX_CHECK(
      _gfxInterface->BindBufferMemory(_gfxInterface->getDevice(), &bufferMemoryBindingDesc, 1));

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
    _gfxInterface->UploadData(_gfxInterface->getTransferQueue(), nullptr, 0, &uploadDesc, 1);
  };
  _gfxInterface->SetBufferDebugName(buffer->getBuf(), tempName.c_str());
  buffer->name = tempName;
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

AcceStructureAllocator::AcceStructureAllocator()
    : _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {}

AcceStructureAllocator::AcceStructureAllocator(MTXInterface* interface)
    : _gfxInterface(interface),
      _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {
  _gfxInterface = interface;
  _pool.setInterFace(interface);
}
AcceStructureAllocator::~AcceStructureAllocator() {}

std::shared_ptr<MtxAcceStructure>
AcceStructureAllocator::allocateAcceStructure(const nri::AccelerationStructureDesc& desc) {
  std::string name = "AccesStructure" + std::to_string(_pool.size());
  auto        uid = _uuidCreater(std::to_string(UUID_COUNT++).c_str());
  MTX_ASSERT(!uid.is_nil());
  std::shared_ptr<MtxAcceStructure> accePtr = _pool.allocate(uid);
  MTX_CHECK(
      _gfxInterface->CreateAccelerationStructure(_gfxInterface->getDevice(), desc, accePtr->acc));
  nri::MemoryDesc memoryDesc = {};
  _gfxInterface->GetAccelerationStructureMemoryDesc(*(accePtr->acc), memoryDesc);
  nri::Memory* asMemory = nullptr;
  _gfxInterface->AllocateMemory(_gfxInterface->getDevice(), memoryDesc.type, memoryDesc.size,
                                asMemory);
  nri::AccelerationStructureMemoryBindingDesc memBindDesc = {asMemory, accePtr->acc};
  _gfxInterface->BindAccelerationStructureMemory(_gfxInterface->getDevice(), &memBindDesc, 1);
  _gfxInterface->SetAccelerationStructureDebugName(*(accePtr->acc), name.c_str());
  accePtr->accDesc = const_cast<nri::AccelerationStructureDesc*>(&desc);
  accePtr->mem = asMemory;

  return accePtr;
}

PipelineAllocator::PipelineAllocator()
    : _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {}
PipelineAllocator::PipelineAllocator(MTXInterface* interface)
    : _uuidCreater(uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value()) {
  setInterface(interface);
}

PipelineAllocator::~PipelineAllocator() {}

std::shared_ptr<MtxPipeline>
PipelineAllocator::allocatePipeline(const MtxPipelineAllocateInfo& allocInfo) {
  std::string tempName = allocInfo.name;
  if (allocInfo.name.empty()) { tempName = "Pipeline" + std::to_string(_pool.size()); }
  auto uid = _uuidCreater(std::to_string(UUID_COUNT++).c_str());
  auto pipelinePtr = _pool.allocate(uid);

  pipelinePtr->name = tempName;
  pipelinePtr->pipelineLayout = const_cast<nri::PipelineLayout*>(
      ((nri::GraphicsPipelineDesc*) (allocInfo.pipelineDesc))->pipelineLayout);
  pipelinePtr->desc = allocInfo.pipelineDesc;
  pipelinePtr->type = allocInfo.pipelineType;

  switch (allocInfo.pipelineType) {
    case MTX::PipelineType::Graphics: {
      MTX_CHECK(_gfxInterface->CreateGraphicsPipeline(
          _gfxInterface->getDevice(), *((nri::GraphicsPipelineDesc*) allocInfo.pipelineDesc),
          pipelinePtr->pipeline));
    }
    case MTX::PipelineType::Compute: {
      MTX_CHECK(_gfxInterface->CreateComputePipeline(
          _gfxInterface->getDevice(), *((nri::ComputePipelineDesc*) allocInfo.pipelineDesc),
          pipelinePtr->pipeline));
    }
    case MTX::PipelineType::RayTracing: {
      MTX_CHECK(_gfxInterface->CreateRayTracingPipeline(
          _gfxInterface->getDevice(), *((nri::RayTracingPipelineDesc*) allocInfo.pipelineDesc),
          pipelinePtr->pipeline));
    }
  }
  _gfxInterface->SetPipelineDebugName(pipelinePtr->getPipeline(), tempName.c_str());
  return pipelinePtr;
}

}// namespace MTX