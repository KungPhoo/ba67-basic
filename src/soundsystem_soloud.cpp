#include "soundsystem_soloud.h"

#include "soloud.h"
#include "soloud_sfxr.h"
#include "soloud_wav.h"
#include "string_helper.h"
#include "unicode.h"
#include <filesystem>
#include <stdexcept>

#ifdef _WIN32
    #include <tchar.h>
#endif

SoundSystemSoLoud::SoundSystemSoLoud() {
    soloud = new SoLoud::Soloud();
    soloud->init(SoLoud::Soloud::CLIP_ROUNDOFF,
#ifdef BA67_SOUND_BACKEND_DEFAULT
                 SoLoud::Soloud::AUTO,
#else
                 SoLoud::Soloud::NOSOUND,
#endif
                 SoLoud::Soloud::AUTO,
                 SoLoud::Soloud::AUTO,
                 2 /*channels*/);

    sfxr.resize(1 + maxVoiceCount, nullptr);
    for (size_t i = 1; i < sfxr.size(); ++i) {
        sfxr[i] = new SoLoud::Sfxr();
    }
    wav = new SoLoud::Wav;
}

SoundSystemSoLoud::~SoundSystemSoLoud() {
    soloud->deinit();

    for (auto*& p : sfxr) {
        delete p;
        p = nullptr;
    }
    sfxr.clear();
    delete wav;
    wav = nullptr;
}

bool SoundSystemSoLoud::SOUND(int voice, const std::string& parameters) {
    /*
wave_type       :

p_base_freq     :
p_freq_limit    :
p_freq_ramp     :
p_freq_dramp    :
p_duty          :
p_duty_ramp     :

p_vib_strength  :
p_vib_speed     :
p_vib_delay     :

p_env_attack    :
p_env_sustain   :
p_env_decay     :
p_env_punch     :

filter_on       :
p_lpf_resonance :
p_lpf_freq      :
p_lpf_ramp      :
p_hpf_freq      :
p_hpf_ramp      :

p_pha_offset    :
p_pha_ramp      :

p_repeat_speed  :

p_arp_speed     :
p_arp_mod       :

master_vol      :

sound_vol       :
*/

    if (voice < 1 || voice > maxVoiceCount) {
        throw std::runtime_error("bad voice number");
    }

    SoLoud::Sfxr* fx = sfxr[voice];
    fx->stop();
    fx->resetParams();

    std::vector<std::string> toks = StringHelper::split(parameters, " \t:;,");

    for (size_t i = 0; i + 1 < toks.size(); ++i) {
        std::string& tok = toks[i];
        float value      = float(atof(toks[i + 1].c_str()));
        if (tok == "wave_type") {
            // 0 = square
            // 1 = sawtooth
            // 2 = sine
            // 3 = noise
            fx->mParams.wave_type = int(value + 0.444445);
            if (fx->mParams.wave_type > 3) {
                throw std::runtime_error("bad value");
            }
            continue;
        } else if (tok == "filter_on") {
            fx->mParams.filter_on = value > 0.00001;
            continue;
        }

        // next all floats -1..1
        auto rng = [value](float f0, float f1) {if (value < f0 - 0.001 || value >= f1 + 0.001) { throw std::runtime_error("out of range"); } };
        // next: all floats 0..1
        if (value < -0.01f || value > 1.01f) {
            throw std::runtime_error("bad value");
        }
        rng(0.0f, 1.0f);
        if (tok == "base_freq") {
            rng(0.0f, 1.0f);
            fx->mParams.p_base_freq = value;
        } else if (tok == "freq_limit") {
            rng(0.0f, 1.0f);
            fx->mParams.p_freq_limit = value;
        } else if (tok == "freq_ramp") {
            rng(-1.0f, 1.0f);
            fx->mParams.p_freq_ramp = value;
        } else if (tok == "freq_dramp") {
            rng(-1.0f, 1.0f);
            fx->mParams.p_freq_dramp = value;
        } else if (tok == "duty") {
            rng(0.0f, 1.0f);
            fx->mParams.p_duty = value;
        } else if (tok == "duty_ramp") {
            rng(-1.0f, 1.0f);
            fx->mParams.p_duty_ramp = value;
        } else if (tok == "vib_strength") {
            rng(0.0f, 1.0f);
            fx->mParams.p_vib_strength = value;
        } else if (tok == "vib_speed") {
            rng(0.0f, 1.0f);
            fx->mParams.p_vib_speed = value;
        } else if (tok == "vib_delay") {
            rng(0.0f, 1.0f);
            fx->mParams.p_vib_delay = value;
        } else if (tok == "env_attack") {
            rng(0.0f, 1.0f);
            fx->mParams.p_env_attack = value;
        } else if (tok == "env_sustain") {
            rng(0.0f, 1.0f);
            fx->mParams.p_env_sustain = value;
        } else if (tok == "env_decay") {
            rng(0.0f, 1.0f);
            fx->mParams.p_env_decay = value;
        } else if (tok == "env_punch") {
            rng(0.0f, 1.0f);
            fx->mParams.p_env_punch = value;
        } else if (tok == "lpf_resonance") {
            rng(0.0f, 1.0f);
            fx->mParams.p_lpf_resonance = value;
        } else if (tok == "lpf_freq") {
            rng(0.0f, 1.0f);
            fx->mParams.p_lpf_freq = value;
        } else if (tok == "lpf_ramp") {
            rng(0.0f, 1.0f);
            fx->mParams.p_lpf_ramp = value;
        } else if (tok == "hpf_freq") {
            rng(0.0f, 1.0f);
            fx->mParams.p_hpf_freq = value;
        } else if (tok == "hpf_ramp") {
            rng(0.0f, 1.0f);
            fx->mParams.p_hpf_ramp = value;
        } else if (tok == "pha_offset") {
            rng(-1.0f, 1.0f);
            fx->mParams.p_pha_offset = value;
        } else if (tok == "pha_ramp") {
            rng(-1.0f, 1.0f);
            fx->mParams.p_pha_ramp = value;
        } else if (tok == "repeat_speed") {
            rng(0.0f, 1.0f);
            fx->mParams.p_repeat_speed = value;
        } else if (tok == "arp_speed") {
            rng(0.0f, 1.0f);
            fx->mParams.p_arp_speed = value;
        } else if (tok == "arp_mod") {
            rng(-1.0f, 1.0f);
            fx->mParams.p_arp_mod = value;
            //        } else if (tok == "master_vol") { // master and sound vol are simply multiplied. Defaults: master 0.05, sample 0.50
            //            fx->mParams.master_vol = value;
        } else if (tok == "volume") {
            rng(0.0f, 1.0f);
            fx->mParams.sound_vol = value;
        } else {
            throw std::runtime_error("unkown parameter name");
        }
    }

    soloud->play(*fx);

    return true;
}

#include "abcmusic.h"
bool SoundSystemSoLoud::PLAY(const std::string& music) {
    wav->stop();

    std::string pathAbc2Midi("abc2midi");
    std::string fluidsynth("fluidsynth");
    std::string soundfont; // use default soundfont on linux ("FluidR3_GM.sf2");

    std::string temp = (const char*)std::filesystem::temp_directory_path().u8string().c_str();

    std::string tempfileAbc(temp + "temp.abc");
    std::string tempfileMidi(temp + "temp.mid");
    std::string tempfileWav(temp + "temp.wav");

#ifdef _WIN32
    WCHAR exepath[1024];
    if (GetModuleFileNameW(NULL, exepath, 1023) < 1023) {
        WCHAR* pEnd = wcsrchr(exepath, L'\\');
        if (pEnd != nullptr) {
            *pEnd       = L'\0';
            WCHAR* pEnd = wcsrchr(exepath, L'\\');
            if (pEnd != nullptr) {
                *pEnd            = L'\0';
                std::wstring abc = exepath;
                // for (auto& c : abc) {
                //     if (c == L'\\') { c = L'/'; }
                // }
                std::string binpath = Unicode::toUtf8String(reinterpret_cast<const char16_t*>(abc.c_str()));
                pathAbc2Midi        = binpath;
                pathAbc2Midi += "\\3rd-party\\abcMIDI\\bin\\abc2midi.exe";
                fluidsynth = binpath;
                fluidsynth += "\\3rd-party\\fluidsynth\\bin\\fluidsynth.exe";
                soundfont = binpath;
                soundfont += "\\3rd-party\\fluid-soundfont-3.1\\FluidR3_GM.sf2";
            }
        }
    }
#endif
    // substitute ';' with '\n'
    std::string ms = "X:1\nT:Song\nM:4/4\nL:1/4\nK:C\n" + music;
    for (auto& c : ms) {
        if (c == ';') {
            c = '\n';
        }
    }

    if (os->doesFileExist(pathAbc2Midi) && os->doesFileExist(pathAbc2Midi) && os->doesFileExist(pathAbc2Midi)) {
        FILE* pf = fopen(tempfileAbc.c_str(), "wb");
        if (!pf) {
            throw std::runtime_error("can't write temp ABC file");
        }

        fprintf(pf, "%s\n", ms.c_str());
        fclose(pf);
        pf = nullptr;

        bool printOutput = true; // true for debugging if PLAY fails
        std::string cmd  = std::string("\"") + pathAbc2Midi + "\" \"" + tempfileAbc + "\" -o \"" + tempfileMidi + "\"";
        os->systemCall(cmd.c_str(), printOutput);
        cmd = std::string("\"") + fluidsynth + "\" -ni \"" + soundfont + "\" \"" + tempfileMidi + "\" -F \"" + tempfileWav + "\" -r 44100";
        os->systemCall(cmd.c_str(), printOutput);
#if defined(NDEBUG)
        _unlink(tempfileAbc.c_str());
        _unlink(tempfileMidi.c_str());
#endif
    } else {
        AbcMusic abc;
        abc.generate(ms, tempfileWav);
    }

    bool rv = true;
    if (wav->load(tempfileWav.c_str()) != 0) {
        rv = false;
    } else {
        soloud->play(*wav);
    }

#if defined(NDEBUG)
    _unlink(tempfileAbc.c_str());
    _unlink(tempfileMidi.c_str());
    _unlink(tempfileWav.c_str());
#endif
    return rv;
}
