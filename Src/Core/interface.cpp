#include "interface.h"
#include "log.h"

namespace MTX {
nri::CommandBuffer* MTXInterface::getInstanceCommandBuffer() {
  std::lock_guard<std::mutex> lock(_mutex);
  nri::CommandBuffer*         cmdBuf = nullptr;
  auto                        res = CreateCommandBuffer(*_instanceCommandAllocator, cmdBuf);
  MTX_ASSERT(res == nri::Result::SUCCESS);
  return cmdBuf;
}
}// namespace MTX