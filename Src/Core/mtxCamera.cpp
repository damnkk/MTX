// clang-format off
#include "NRIFramework.h"
#include "mtxCamera.h"
#include "iostream"
// clang-format on

namespace MTX {

void MtxCamera::updateState(const CameraDesc& desc, uint32_t frameIndex, float deltaTime) {
  this->Update(desc, frameIndex);
  if ((dirtyState.globalPosition != this->state.globalPosition)
      || (dirtyState.rotation != this->state.rotation)) {
    isDirty = true;
  } else {
    isDirty = false;
  }
  dirtyState = this->state;
}

};//namespace MTX