#ifndef MTXCAMERA_H
#define MTXCAMERA_H
#include "resource.h"
#include "Camera.h"
#include "mtxUtils.h"

namespace MTX {
class MtxCamera : public Camera {
 public:
  void updateState(const CameraDesc& desc, uint32_t frameIndex, const float deltaTime);
  std::shared_ptr<MtxBuffer> camUniformBuffer;
  CameraState                dirtyState;
  bool                       isDirty = true;
};

}// namespace MTX
#endif//MTXCAMERA_H