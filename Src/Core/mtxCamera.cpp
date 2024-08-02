// clang-format off
// #include "NRIFramework.h"
#include "mtxCamera.h"
// clang-format on

namespace MTX {

void MtxCamera::updateState(const CameraDesc& desc, uint32_t frameIndex, float deltaTime) {
  this->Update(desc, frameIndex);
  if (any(dirtyState.globalPosition != this->state.globalPosition)
      || any(dirtyState.rotation != this->state.rotation)) {
    isDirty = true;
  } else {
    isDirty = false;
  }
  dirtyState = this->state;
}

};//namespace MTX