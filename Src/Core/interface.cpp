#include "interface.h"
#include "log.h"

namespace MTX {
nri::CommandBuffer* MTXInterface::getInstantCommandBuffer() {
  std::lock_guard<std::mutex> lock(_mutex);
  nri::CommandBuffer*         cmdBuf = nullptr;
  auto                        res = CreateCommandBuffer(*_instantCommandAllocator, cmdBuf);
  MTX_ASSERT(res == nri::Result::SUCCESS);
  return cmdBuf;
}
}// namespace MTX