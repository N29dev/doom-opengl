#pragma once
namespace audio {
enum class Sound { Pistol, Shotgun, Dry, Reload, Switch, Step, Jump, Land, Slide,
                   Door, Pickup, Hit, Kill, Hurt, Death, EnemyFire, Locked, Clear, Finish, Count };
bool initialize();
void play(Sound sound, float gain = 1.0f);
void toggleMute();
bool muted();
void shutdown();
}
