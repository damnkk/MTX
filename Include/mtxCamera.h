#ifndef MTXCAMERA_H
#define MTXCAMERA_H
#include "Camera.h"
#include "mtxUtils.h"
#include "resource.h"

namespace MTX {
class MtxCamera : public Camera {
 public:
  std::shared_ptr<MtxBuffer> camUniformBuffer;
  CameraDesc                 desc;
};

}// namespace MTX
#endif//MTXCAMERA_H