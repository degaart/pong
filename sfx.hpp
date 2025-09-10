#pragma once

#include <stdint.h>
#include <stddef.h>
#include <filesystem>

class Sfx
{
public:
    Sfx();
    explicit Sfx(const std::filesystem::path& filename);
    Sfx(Sfx&& other) noexcept;
    Sfx(const Sfx& other) = delete;
    Sfx& operator=(Sfx&& other) noexcept;
    Sfx& operator=(const Sfx&) = delete;
    ~Sfx();

    bool load(const std::filesystem::path& filename);
    const int16_t* samples() const;
    size_t sampleCount() const;
    int channels() const;
    int sampleRate() const;
    size_t size() const;
private:
    int16_t* _samples;
    size_t _sampleCount;
    int _channels;
    int _sampleRate;
    size_t _size;

    void moveFrom(Sfx& other) noexcept;
};


