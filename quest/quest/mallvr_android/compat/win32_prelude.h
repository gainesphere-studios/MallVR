/* MallVR Quest: forced first include of vpix_render.cpp on Android. The C++ library headers come in BEFORE _WIN32 is defined
   (with _WIN32 set, libc++ would pick its Windows locale code), then _WIN32 turns on vpix_render.cpp's Windows code paths,
   which compat/windows.h maps to EGL / gl4es / POSIX. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>
#include <numeric>
#include <limits>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <sstream>
#include <fstream>
#include <iostream>
#include <type_traits>
#ifndef _WIN32
#define _WIN32 1
#endif
/* vpix_menu.h (in-game A + B menu, 10-25) talks UDP to the launcher with Winsock names; on Windows <windows.h> brings them in */
#include "winsock2.h"
