#include "resourcePool.h"
#include <gtest/gtest.h>
using namespace MTX;

//resourcePool test with regular type
TEST(MTXTest, ResourcePoolTest) {
  ResourcePool<int>  intPool;
  ResourcePool<bool> boolPool;
};

TEST(MTXTest, ResourcePoolTest1) {
  ResourcePool<MtxTexture> intPool;

  for (int i = 0; i < 4097; ++i) {
    uuids::uuid_name_generator gen(
        uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43").value());

    std::string name = "name_;slkdjfg;slkdnmfglsjndgliklsdlohugpoasdnrlskjngrk,."
                       "sdjnfglksjdnflgksjndfglksjndflgkjshdlfoguihslkdfjglksjnfglskjnlk";
    name += std::to_string(i);
    auto uid = gen(name);
    //auto uid = uuids::uuid::from_string(name);
    std::string strUid = uuids::to_string(uid);
    auto        ptr = intPool.allocate(uid);
    // //assert(ptr != nullptr);
    // if (i >= MTX_MAX_POOL_CAPACITY) { assert(ptr == nullptr); }
  }
}