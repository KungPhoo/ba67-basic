#pragma once
#include "os.h"
#include <vector>

namespace SoLoud {
class Soloud;
class Sfxr;
class Wav;
}; // namespace SoLoud

class SoundSystemSoLoud : public SoundSystem {
public:
    SoundSystemSoLoud();
    virtual ~SoundSystemSoLoud();
    bool SOUND(int voice, const std::string& parameters) override;
    bool PLAY(const std::string& music) override;
    const size_t maxVoiceCount = 64;

private:
    SoLoud::Soloud* soloud;
    std::vector<SoLoud::Sfxr*> sfxr; // [voice 0..63]
    SoLoud::Wav* wav;
};