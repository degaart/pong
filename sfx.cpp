#include "sfx.hpp"

#include "stb_vorbis.h"
#include <fmt/format.h>
#include <utility>

Sfx::Sfx()
    : _samples(nullptr)
{
}

Sfx::Sfx(const std::filesystem::path& filename)
{
    if (!load(filename))
    {
        fmt::println(stderr, "Failed to load file {}", filename.string());
        abort();
    }
}

Sfx::Sfx(Sfx&& other) noexcept
{
    moveFrom(other);
}

Sfx& Sfx::operator=(Sfx&& other) noexcept
{
    if (&other != this)
    {
        free(_samples);
        moveFrom(other);
    }
    return *this;
}

Sfx::~Sfx()
{
    if (_samples)
    {
        free(_samples);
    }
}

void Sfx::moveFrom(Sfx& other) noexcept
{
    _samples = std::exchange(other._samples, nullptr);
    _sampleCount = other._sampleCount;
    _channels = other._channels;
    _sampleRate = other._sampleRate;
    _size = other._size;
}

bool Sfx::load(const std::filesystem::path& filename)
{
    if (_samples)
    {
        free(_samples);
        _samples = nullptr;
    }

    auto ret = stb_vorbis_decode_filename(filename.string().c_str(), &_channels, &_sampleRate, &_samples);
    if (ret == -1)
    {
        return false;
    }
    _sampleCount = ret;
    return true;
}

const int16_t* Sfx::samples() const
{
    return _samples;
}

size_t Sfx::sampleCount() const
{
    return _sampleCount;
}

int Sfx::channels() const
{
    return _channels;
}

int Sfx::sampleRate() const
{
    return _sampleRate;
}

size_t Sfx::size() const
{
    return _sampleCount * _channels * sizeof(int16_t);
}

