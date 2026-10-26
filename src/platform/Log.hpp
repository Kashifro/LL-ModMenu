#pragma once

#include "ll/api/io/Logger.h"
#include "ll/api/mod/NativeMod.h"

namespace modmenu {

inline ll::mod::NativeMod& Self() { return *ll::mod::NativeMod::current(); }
inline ll::io::Logger& Log() { return Self().getLogger(); }

}
