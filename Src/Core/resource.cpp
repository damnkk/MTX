#include "resource.h"
#include "Interface.h"
#include "log.h"

namespace MTX {
uuids::uuid EMPTY_UID;

void MtxTexture::destroy(MTXInterface* interface) {
  MTX_ASSERT(interface != nullptr);
  if (nullptr != tex && nullptr != mem) {
    interface->DestroyTexture(*tex);
    interface->FreeMemory(*mem);
  }
  if (imageView != nullptr) { interface->DestroyDescriptor(*imageView); }
}

void MtxBuffer::destroy(MTXInterface* interface) {
  MTX_ASSERT(interface != nullptr);
  if (nullptr != buf && nullptr != mem) {
    interface->DestroyBuffer(*buf);
    interface->FreeMemory(*mem);
  }
  if (bufView != nullptr) { interface->DestroyDescriptor(*bufView); }
}

void MtxAcceStructure::destroy(MTXInterface* interface) {
  MTX_ASSERT(interface != nullptr);
  if (nullptr != acc && nullptr != mem) {
    interface->DestroyAccelerationStructure(*acc);
    interface->FreeMemory(*mem);
  }
  if (accView != nullptr) { interface->DestroyDescriptor(*accView); }
}

void MtxPipeline::destroy(MTXInterface* interface) {
  interface->DestroyPipeline(*pipeline);
  interface->DestroyPipelineLayout(*pipelineLayout);
};

void FrameResource::destroy(MTXInterface* interface) {
  interface->DestroyFence(*_fence);
  for (auto& cmd : _commandBuffers) {
    if (cmd != nullptr) interface->DestroyCommandBuffer(*cmd);
  }
  for (auto& cmdPool : _commandPools) { interface->DestroyCommandAllocator(*cmdPool); }
  _frameTexture.destroy(interface);
}

}// namespace MTX