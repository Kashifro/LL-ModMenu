
#pragma once

#include "ll/api/io/Logger.h"
#include "ll/api/mod/NativeMod.h"

namespace nugget {

inline ll::io::Logger& Log() { return ll::mod::NativeMod::current()->getLogger(); }

} 
