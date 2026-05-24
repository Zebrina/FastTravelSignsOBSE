#pragma once

#include "OBSE/OBSE.h"

#include <string_view>

using namespace std::literals;

namespace Plugin
{
    inline constexpr auto MODNAME = "Fast Travel Signs"sv;
    inline constexpr auto AUTHOR = "Zebrina"sv;
    inline constexpr auto VERSION = OBSE::Version{ 1, 0, 0, 0 };
    inline constexpr auto INTERNALNAME = "FastTravelSigns"sv;
    inline constexpr auto CONFIGFILE = "OBSE\\Plugins\\FastTravelSigns.toml"sv;
}
