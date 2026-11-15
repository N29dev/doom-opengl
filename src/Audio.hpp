#pragma once
namespace audio {
enum class Sound { Pistol, Shotgun, Dry, Reload, Switch, Step, Jump, Land, Slide,
                   Door, Pickup, Hit, Kill, Hurt, Death, EnemyFire, Locked, Clear, Finish,
                   DemonGrowl, DemonAttack, DemonPain, DemonDeath,
                   FlyHiss, FlyPain, FlyDeath,
                   HeavyGrowl, HeavyAttack, HeavyPain, HeavyDeath, HeavyStep, Count };
bool initialize();
void play(Sound sound, float gain = 1.0f, float pan = 0.0f);
void toggleMute();
bool muted();
void shutdown();
}
