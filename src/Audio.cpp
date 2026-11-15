#include "Audio.hpp"
#include "miniaudio.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <vector>

namespace audio {
namespace {
constexpr unsigned sampleRate = 44100;
constexpr float tau = 6.28318530718f;
using Samples = std::vector<float>;
std::array<Samples, static_cast<std::size_t>(Sound::Count)> bank;
struct Voice { const Samples* sound = nullptr; std::size_t offset = 0; float left = 1.0f, right = 1.0f; };
std::array<Voice, 16> voices{};
std::mutex mutex;
ma_device device{};
bool ready = false, silent = false;

void generate(Sound which, float seconds, float low, float high, float noiseAmount, float decay,
              float volume, bool pulse = false, bool chime = false) {
    auto& out = bank[static_cast<std::size_t>(which)];
    out.resize(static_cast<std::size_t>(seconds * sampleRate));
    std::uint32_t rng = 0x12345678u + static_cast<unsigned>(which)*2027u;
    float phase = 0.0f, filtered = 0.0f;
    for (std::size_t i = 0; i < out.size(); ++i) {
        const float t = static_cast<float>(i) / sampleRate, u = t / seconds;
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        const float noise = static_cast<float>(rng & 65535u)/32767.5f - 1.0f;
        filtered += 0.24f * (noise - filtered);
        const float f = chime ? low * (u < 0.33f ? 1.0f : u < 0.66f ? 1.26f : 1.5f) : low + (high-low)*u;
        phase += tau*f/sampleRate;
        const float wave = std::sin(phase) * 0.75f + std::sin(phase*2.0f)*0.15f;
        float env = std::min(1.0f,t/0.003f)*std::min(1.0f,(seconds-t)/0.018f)*std::exp(-decay*u);
        if (pulse) env *= (0.30f + 0.70f * std::pow(std::max(0.0f,std::cos(tau*u*2.0f)),3.0f));
        out[i] = volume * env * ((1-noiseAmount)*wave+noiseAmount*(0.58f*noise+0.42f*filtered));
    }
}
void callback(ma_device*, void* output, const void*, ma_uint32 frames) {
    auto* out = static_cast<float*>(output);
    std::fill_n(out, static_cast<std::size_t>(frames)*2, 0.0f);
    std::lock_guard<std::mutex> lock(mutex);
    if (silent) return;
    for (auto& v : voices) {
        if (!v.sound) continue;
        const auto count=std::min(static_cast<std::size_t>(frames),v.sound->size()-v.offset);
        for (std::size_t i=0;i<count;++i) {
            const float sample = (*v.sound)[v.offset+i];
            out[2*i] += sample*v.left;
            out[2*i+1] += sample*v.right;
        }
        v.offset += count;
        if (v.offset==v.sound->size()) v={};
    }
    for (std::size_t i=0;i<static_cast<std::size_t>(frames)*2;++i) out[i]=std::clamp(out[i],-0.9f,0.9f);
}
}
bool initialize() {
    if (ready) return true;
    generate(Sound::Pistol,.18f,210,55,.76f,6,.75f);
    generate(Sound::Shotgun,.36f,120,35,.88f,5,.92f);
    generate(Sound::Dry,.06f,900,220,.72f,7,.22f);
    generate(Sound::Reload,.38f,230,600,.73f,1.6f,.32f,true);
    generate(Sound::Switch,.16f,600,210,.60f,4,.25f,true);
    generate(Sound::Step,.12f,105,58,.82f,5,.22f);
    generate(Sound::Jump,.15f,130,250,.22f,3,.25f);
    generate(Sound::Land,.17f,95,42,.73f,6,.37f);
    generate(Sound::Slide,.32f,130,65,.90f,3,.22f);
    generate(Sound::Door,.40f,100,210,.63f,2,.32f,true);
    generate(Sound::Pickup,.21f,440,790,.05f,1.3f,.29f,false,true);
    generate(Sound::Hit,.09f,300,70,.55f,5,.27f);
    generate(Sound::Kill,.36f,190,37,.50f,3,.42f);
    generate(Sound::Hurt,.20f,155,60,.42f,3,.40f);
    generate(Sound::Death,.66f,110,29,.52f,2.4f,.52f);
    generate(Sound::EnemyFire,.22f,320,90,.36f,3,.35f);
    generate(Sound::Locked,.23f,140,100,.12f,2,.30f,true);
    generate(Sound::Clear,.48f,380,700,.04f,1.5f,.30f,false,true);
    generate(Sound::Finish,.85f,330,660,.02f,1.3f,.34f,false,true);
    // Distinct monster voices: rough growls, high-pitched flying screeches and low mechanical rumbles.
    generate(Sound::DemonGrowl,.65f,105,68,.42f,1.5f,.44f,true);
    generate(Sound::DemonAttack,.30f,170,62,.59f,2.5f,.57f,true);
    generate(Sound::DemonPain,.24f,260,110,.35f,2.8f,.49f);
    generate(Sound::DemonDeath,.75f,155,28,.53f,2.1f,.62f,true);
    generate(Sound::FlyHiss,.46f,580,310,.65f,1.6f,.31f,true);
    generate(Sound::FlyPain,.20f,920,290,.26f,3.5f,.38f);
    generate(Sound::FlyDeath,.63f,720,48,.46f,2.5f,.46f,true);
    generate(Sound::HeavyGrowl,.87f,61,38,.34f,1.2f,.53f,true);
    generate(Sound::HeavyAttack,.46f,90,24,.75f,3.0f,.75f);
    generate(Sound::HeavyPain,.31f,104,56,.68f,3.2f,.57f,true);
    generate(Sound::HeavyDeath,1.02f,83,19,.65f,2.0f,.76f,true);
    generate(Sound::HeavyStep,.22f,64,25,.52f,5.5f,.48f);
    auto config = ma_device_config_init(ma_device_type_playback);
    config.playback.format=ma_format_f32;
    config.playback.channels=2;
    config.sampleRate=sampleRate;
    config.dataCallback=callback;
    if (ma_device_init(nullptr,&config,&device)!=MA_SUCCESS) return false;
    if (ma_device_start(&device)!=MA_SUCCESS) { ma_device_uninit(&device); return false; }
    ready=true;
    return true;
}
void play(Sound sound,float gain,float pan) {
    if (!ready) return;
    std::lock_guard<std::mutex> lock(mutex);
    if (silent) return;
    auto slot=std::find_if(voices.begin(),voices.end(),[](const Voice& v){return !v.sound;});
    if (slot==voices.end()) slot=std::min_element(voices.begin(),voices.end(),[](const Voice& a,const Voice& b){
        return a.sound->size()-a.offset < b.sound->size()-b.offset;
    });
    const float volume=std::clamp(gain,0.0f,1.0f);
    pan=std::clamp(pan,-1.0f,1.0f);
    *slot={&bank[static_cast<std::size_t>(sound)],0,
           volume*std::sqrt(0.5f*(1.0f-pan)),volume*std::sqrt(0.5f*(1.0f+pan))};
}
void toggleMute() {
    std::lock_guard<std::mutex> lock(mutex);
    silent=!silent;
    if(silent) voices={};
}
bool muted() { std::lock_guard<std::mutex> lock(mutex); return silent; }
void shutdown() { if (ready) { ma_device_uninit(&device); ready=false; } }
}
