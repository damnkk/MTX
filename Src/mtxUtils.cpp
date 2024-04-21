#include "mtxUtils.h"

namespace MTX {
namespace utils {

static nri::AccessBits bufferUsageToAccess(nri::BufferUsageBits usage) {
  nri::AccessStage test;
  nri::AccessBits  res = nri::AccessBits::UNKNOWN;
  if (usage & nri::BufferUsageBits::VERTEX_BUFFER) { res |= nri::AccessBits::VERTEX_BUFFER; }
  if (usage & nri::BufferUsageBits::INDEX_BUFFER) { res |= nri::AccessBits::INDEX_BUFFER; }
  if (usage & nri::BufferUsageBits::SHADER_RESOURCE) { res |= nri::AccessBits::SHADER_RESOURCE; }
  if (usage & nri::BufferUsageBits::SHADER_RESOURCE_STORAGE) {
    res |= nri::AccessBits::SHADER_RESOURCE_STORAGE;
  }
  if (usage & nri::BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_READ) {
    res |= nri::AccessBits::ACCELERATION_STRUCTURE_READ;
  }
  return res;
}
}// namespace utils
}// namespace MTX