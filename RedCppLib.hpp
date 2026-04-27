#pragma once

#include <stdio.h>
#include <locale>
#include <math.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>
#include <deque>
#include <functional>
#include <cstring>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <malloc.h>
#include <thread>
#include <any>
#include <mutex>
#include <optional>
#include <queue>
#include <list>
#include <condition_variable>
#include <concepts>
#include <cstddef>
#include <type_traits>

#if __cpp_exceptions
#include <exception>
#endif

#if USE_PACKED
#define PACK __attribute__ ((packed))
#else
#define PACK
#endif

#define ArraySize(a) (sizeof(a)/sizeof(a[0]))
#ifndef byte
using byte = uint8_t;
#endif

#define BEGIN_RED_NAMESPACE namespace Red {
#define END_RED_NAMESPACE }

#include "Assert.hpp"
#include "vec.hpp"
#include "Containers.hpp"
#include "ThreadPool.hpp"