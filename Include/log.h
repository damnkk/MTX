#include <spdlog/spdlog.h>

#define MTX_INFO(format, ...)  spdlog::info(format, ##__VA_ARGS__);
#define MTX_WARN(format, ...)  spdlog::warn(format, ##__VA_ARGS__);
#define MTX_ERROR(format, ...) spdlog::error(format, ##__VA_ARGS__);
#define MTX_ASSERT(condition)                                                                      \
  if (!(condition)) {                                                                              \
    spdlog::error("Assert fail in File::{}->Line::{}", __FILE__, __LINE__);                        \
    exit(-1);                                                                                      \
  }\
