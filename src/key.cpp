#include "mc/key.hpp"

#include <magic_enum.hpp>

auto operator&(int bits, ModifierKey mod) -> int
{
    return bits & magic_enum::enum_integer(mod);
}

auto modifiersToStr(int mods) -> std::string
{
    std::string str;

    auto modifier_enum_entries = magic_enum::enum_entries<ModifierKey>();

    for (auto const& pair : modifier_enum_entries)
    {
        if ((mods & pair.first) != 0)
        {
            str += pair.second.data();
            str += ", ";
        }
    }

    return str.substr(0, str.length() - 2);
}
