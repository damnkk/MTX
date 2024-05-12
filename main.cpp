#include <iostream>

#include "NRIFramework.h"
#include "renderer.h"
using namespace MTX;

int main(int argc, char** argv) {
  SampleBase* rayTracingRenderer = new MTXRenderer;
  std::cout << "test" << std::endl;
  bool result = rayTracingRenderer->Create(argc, argv, "MTXRenderer");
  if (result) { rayTracingRenderer->RenderLoop(); }

  delete rayTracingRenderer;
  rayTracingRenderer = nullptr;
  return result ? 0 : 1;
}
