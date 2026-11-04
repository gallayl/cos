#pragma once

#include <cstdlib>
#include <memory>

using BerrySourceBuffer = std::unique_ptr<char, decltype(&std::free)>;

template <typename Compile> int compile_and_release_source(BerrySourceBuffer &source, Compile compile)
{
    const int result = compile(source.get());
    source.reset();
    return result;
}
