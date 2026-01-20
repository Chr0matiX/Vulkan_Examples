// 1. 剔除冷门 API（如 DDE, RPC, Shell），极大加快编译和 Clangd 索引速度
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// 2. 严禁定义 min/max 宏，防止破坏 std::min/max
#ifndef NOMINMAX
#define NOMINMAX
#endif
// 3. 强制类型检查，使 HWND、HINSTANCE 等句柄成为独立类型而非 void*
#ifndef STRICT
#define STRICT
#endif

#include <windows.h>