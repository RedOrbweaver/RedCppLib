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

#define PACK __attribute__ ((packed))
#define ArraySize(a) (sizeof(a)/sizeof(a[0]))
#ifndef byte
using byte = uint8_t;
#endif

#define BEGIN_RED_NAMESPACE namespace Red {
#define END_RED_NAMESPACE }

#include "vec.hpp"
#include "Containers.hpp"
#include "ThreadPool.hpp"

namespace Red
{

}