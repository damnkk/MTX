#ifndef MTXUTILS_H
#define MTXUTILS_H
#include "NRIFramework.h"
namespace MTX {
const uint32_t MTX_MAX_FRAME_COUNT = 3;
namespace utils {
nri::AccessBits bufferUsageToAccess(nri::BufferUsageBits usage);
}// namespace utils
}// namespace MTX

#endif//MTXUTILS_H