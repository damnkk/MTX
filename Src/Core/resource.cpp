#include "resource.h"
#include "Interface.h"
#include "log.h"

namespace MTX {

void MtxTexture::destroy(MTXInterface* interface) {
  MTX_ASSERT(interface != nullptr);
  if (nullptr != tex && nullptr != mem) {
    interface->DestroyTexture(*tex);
    interface->FreeMemory(*mem);
  }
}

void MtxBuffer::destroy(MTXInterface* interface) {
  MTX_ASSERT(interface != nullptr);
  if (nullptr != buf && nullptr != mem) {
    interface->DestroyBuffer(*buf);
    interface->FreeMemory(*mem);
  }
}

void MtxAcceStructure::destroy(MTXInterface* interface) {
  MTX_ASSERT(interface != nullptr);
  if (nullptr != acc && nullptr != mem) {
    interface->DestroyAccelerationStructure(*acc);
    interface->FreeMemory(*mem);
  }
}

}// namespace MTX