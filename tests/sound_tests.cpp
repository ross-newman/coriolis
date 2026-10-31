// Tests of the sound: the FM synthesizer's effects, the mixer, and the cues the game gives for what happens.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "coriolis/adlib_synth.h"
#include "coriolis/sound.h"
#include "coriolis/ui.h"

namespace {

int g_failures = 0;

#define CHECK(condition)                                                                 \
  do {                                                                                   \
    if (!(condition)) {                                                                  \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
      ++g_failures;                                                                      \
    }                                                                                    \
  } while (false)

using namespace coriolis;

int Peak(const std::vector<std::int16_t>& samples) {
  int peak = 0;
  for (const std::int16_t s : samples) {
    peak = std::max(peak, std::abs(static_cast<int>(s)));
  }
  return peak;
}

/// Sign changes in a stretch of samples: a rough measure of pitch
int Crossings(const std::vector<std::int16_t>& samples, std::size_t from, std::size_t to) {
  int n = 0;
  for (std::size_t i = from + 1; i < std::min(to, samples.size()); ++i) {
    n += ((samples[i - 1] < 0) != (samples[i] < 0)) ? 1 : 0;
  }
  return n;
}

void EveryEffectMakesSound() {
  AdLibSynth synth(1, 44100);
  for (int i = 0; i < kSoundEffectCount; ++i) {
    const SoundEffect effect = static_cast<SoundEffect>(i);
    const std::vector<std::int16_t> samples = synth.Render(effect);
    CHECK(samples.size() > 1000);
    CHECK(samples.size() < 44100 * 8);
    // Loud enough to hear, and not just a click
    CHECK(Peak(samples) > 3000);
    CHECK(Crossings(samples, 0, samples.size()) > 10);
    CHECK(std::string(SoundEffectName(effect)) != "?");
  }
}

void TheOriginalEffectsKeepTheirLengths() {
  AdLibSynth synth(1, 44100);
  // The lengths of the effects of the original synthesizer, in seconds
  const struct {
    SoundEffect effect;
    double seconds;
  } originals[] = {
      {SoundEffect::kFire, 0.08},        {SoundEffect::kEnemyHit, 0.15},   {SoundEffect::kEnemyHitBig, 0.4},
      {SoundEffect::kPlayerHit, 0.3},    {SoundEffect::kPowerUp, 0.25},    {SoundEffect::kWaveComplete, 0.6},
      {SoundEffect::kGameOver, 1.2},     {SoundEffect::kMenuSelect, 0.1},  {SoundEffect::kMenuMove, 0.05},
      {SoundEffect::kBossAlarm, 0.8},    {SoundEffect::kExplosion, 0.5},   {SoundEffect::kShield, 0.3},
  };
  for (const auto& o : originals) {
    const std::size_t expected = static_cast<std::size_t>(o.seconds * 44100.0);
    CHECK(synth.Render(o.effect).size() == expected);
  }
}

void TheSameSeedGivesTheSameSound() {
  AdLibSynth a(7, 44100);
  AdLibSynth b(7, 44100);
  AdLibSynth c(8, 44100);
  for (const SoundEffect effect : {SoundEffect::kFire, SoundEffect::kExplosion, SoundEffect::kEnemyHit, SoundEffect::kMissile}) {
    CHECK(a.Render(effect) == b.Render(effect));
  }
  // The effects with noise in them differ with the seed, the pure tones do not
  CHECK(!(a.Render(SoundEffect::kExplosion) == c.Render(SoundEffect::kExplosion)));
  AdLibSynth d(7, 44100);
  AdLibSynth e(8, 44100);
  CHECK(d.Render(SoundEffect::kFire) == e.Render(SoundEffect::kFire));
}

void TheSampleRateScalesTheLength() {
  AdLibSynth low(1, 22050);
  AdLibSynth high(1, 44100);
  CHECK(low.sample_rate() == 22050);
  const std::size_t a = low.Render(SoundEffect::kMenuSelect).size();
  const std::size_t b = high.Render(SoundEffect::kMenuSelect).size();
  CHECK(b > a * 19 / 10 && b < a * 21 / 10);
}

void TheWaveformsAreTheOpl2Ones() {
  const double pi = 3.14159265358979323846;
  CHECK(std::fabs(AdLibSynth::Waveform(0, pi / 2.0) - 1.0) < 1e-9);
  // Half sine: the second half is silent
  CHECK(AdLibSynth::Waveform(1, pi / 2.0) > 0.99 && AdLibSynth::Waveform(1, 1.5 * pi) == 0.0);
  // Absolute sine: never negative
  CHECK(AdLibSynth::Waveform(2, 1.5 * pi) > 0.99);
  // Quarter sine pulses: sound in the first quarter and the third, silence in the second and fourth
  CHECK(AdLibSynth::Waveform(3, pi / 4.0) > 0.5 && AdLibSynth::Waveform(3, 0.75 * pi) == 0.0);
  CHECK(AdLibSynth::Waveform(3, 1.25 * pi) < -0.5 && AdLibSynth::Waveform(3, 1.75 * pi) == 0.0);
  // Square, saw and triangle
  CHECK(AdLibSynth::Waveform(4, 1.0) == 1.0 && AdLibSynth::Waveform(4, 4.0) == -1.0);
  CHECK(std::fabs(AdLibSynth::Waveform(5, 0.0) + 1.0) < 1e-9 && AdLibSynth::Waveform(5, pi) > -1e-9);
  CHECK(std::fabs(AdLibSynth::Waveform(6, pi / 2.0)) < 1e-9 && AdLibSynth::Waveform(6, pi) > 0.99);
  // Phases go round and round
  CHECK(std::fabs(AdLibSynth::Waveform(0, 0.3) - AdLibSynth::Waveform(0, 0.3 + 4.0 * pi)) < 1e-9);
}

void TheLaserFallsInPitch() {
  AdLibSynth synth(1, 44100);
  const std::vector<std::int16_t> fire = synth.Render(SoundEffect::kFire);
  const std::size_t quarter = fire.size() / 4;
  // The same length of sound, early and late: the early part has more cycles
  CHECK(Crossings(fire, 0, quarter) > Crossings(fire, 2 * quarter, 3 * quarter));
  // And the hyperdrive climbs
  const std::vector<std::int16_t> jump = synth.Render(SoundEffect::kHyperspace);
  const std::size_t part = jump.size() / 5;
  CHECK(Crossings(jump, 3 * part, 4 * part) > 3 * Crossings(jump, 0, part));
}

void TheMixerPlaysAndFinishes() {
  SoundMixer mixer(44100, 1);
  std::vector<std::int16_t> block(441);
  mixer.Mix(block.data(), block.size());
  CHECK(Peak(block) == 0);  // nothing playing: silence
  mixer.Play(SoundEffect::kExplosion);
  CHECK(mixer.active_voices() == 1);
  int loudest = 0;
  int blocks = 0;
  while (mixer.active_voices() > 0 && blocks < 200) {
    mixer.Mix(block.data(), block.size());
    loudest = std::max(loudest, Peak(block));
    ++blocks;
  }
  CHECK(loudest > 3000);
  // The explosion lasts half a second, which is 50 blocks of a hundredth
  CHECK(blocks >= 50 && blocks <= 52);
  CHECK(std::fabs(mixer.Duration(SoundEffect::kExplosion) - 0.5) < 0.01);
}

void TheMixerBlockSizeDoesNotMatter() {
  SoundMixer one(44100, 1);
  SoundMixer many(44100, 1);
  one.Play(SoundEffect::kPowerUp);
  many.Play(SoundEffect::kPowerUp);
  std::vector<std::int16_t> whole(11025);
  one.Mix(whole.data(), whole.size());
  std::vector<std::int16_t> pieces(11025);
  for (std::size_t at = 0; at < pieces.size(); at += 441) {
    many.Mix(pieces.data() + at, std::min<std::size_t>(441, pieces.size() - at));
  }
  CHECK(whole == pieces);
}

void VolumeAndGain() {
  SoundMixer loud(44100, 1);
  SoundMixer quiet(44100, 1);
  loud.Play(SoundEffect::kFire, 1.0f);
  quiet.Play(SoundEffect::kFire, 0.25f);
  std::vector<std::int16_t> a(2000), b(2000);
  loud.Mix(a.data(), a.size());
  quiet.Mix(b.data(), b.size());
  CHECK(Peak(b) > 0 && Peak(b) < Peak(a) / 2);

  SoundMixer off(44100, 1);
  off.set_master_volume(0.0f);
  off.Play(SoundEffect::kFire);
  off.Mix(a.data(), a.size());
  CHECK(Peak(a) == 0);
  // A sound with no loudness is not started
  SoundMixer silent(44100, 1);
  silent.Play(SoundEffect::kFire, 0.0f);
  CHECK(silent.active_voices() == 0);
  silent.set_master_volume(5.0f);
  CHECK(silent.master_volume() == 1.0f);
}

void TheMixerLimitsTheVoices() {
  SoundMixer mixer(44100, 1);
  for (int i = 0; i < 100; ++i) {
    mixer.Play(SoundEffect::kFire);
  }
  CHECK(mixer.active_voices() == SoundMixer::kMaxPerEffect);
  for (int i = 0; i < kSoundEffectCount; ++i) {
    for (int k = 0; k < 10; ++k) {
      mixer.Play(static_cast<SoundEffect>(i));
    }
  }
  CHECK(mixer.active_voices() <= SoundMixer::kMaxVoices);
  // A great many sounds at once never go past the range of the samples
  std::vector<std::int16_t> block(4410);
  for (int n = 0; n < 20; ++n) {
    mixer.Mix(block.data(), block.size());
    CHECK(Peak(block) <= 32767);
  }
  mixer.Stop();
  CHECK(mixer.active_voices() == 0);
}

bool Has(const std::vector<SoundCue>& cues, SoundEffect effect) {
  return std::any_of(cues.begin(), cues.end(), [&](const SoundCue& c) { return c.effect == effect; });
}

void TheGameGivesCues() {
  GameUi ui("", 5);
  // Menu clicks
  ui.Key(UiKey::kDown);
  CHECK(Has(ui.TakeSounds(), SoundEffect::kUiMove));
  CHECK(ui.TakeSounds().empty());  // taking them clears them
  ui.Key(UiKey::kUp);
  ui.TakeSounds();
  ui.Key(UiKey::kAccept);  // a new game
  CHECK(Has(ui.TakeSounds(), SoundEffect::kTitleSting));
  // Launching
  ui.Key(UiKey::kLaunch);
  CHECK(Has(ui.TakeSounds(), SoundEffect::kLaunchBay));
  // Out of the bay, firing the lasers
  for (int i = 0; i < 60 * 7; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
  }
  ui.TakeSounds();
  FlightInput fire;
  fire.held.set(static_cast<std::size_t>(Action::kFireLasers));
  bool fired = false;
  for (int i = 0; i < 60 && !fired; ++i) {
    ui.Update(fire, 1.0 / 60.0);
    fired = Has(ui.TakeSounds(), SoundEffect::kLaserPulse);
  }
  CHECK(fired);
  // Arrow keys in flight are flight controls and do not click
  ui.TakeSounds();
  ui.Key(UiKey::kLeft);
  CHECK(ui.TakeSounds().empty());
  // Every cue is a real effect at a sensible volume
  FlightInput ecm;
  ecm.pressed.set(static_cast<std::size_t>(Action::kEcm));
  ui.game().commander().equipment.ecm = true;
  ui.game().player()->has_ecm = true;
  ui.Update(ecm, 1.0 / 60.0);
  for (const SoundCue& cue : ui.TakeSounds()) {
    CHECK(static_cast<int>(cue.effect) >= 0 && static_cast<int>(cue.effect) < kSoundEffectCount);
    CHECK(cue.gain > 0.0f && cue.gain <= 1.0f);
  }
}

void FarAwayEventsAreQuieterThanNearOnes() {
  GameUi ui("", 5);
  ui.Key(UiKey::kAccept);
  ui.Key(UiKey::kLaunch);
  for (int i = 0; i < 60 * 7; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
  }
  ui.TakeSounds();
  // A pirate fires a laser close to us, then another far off
  Game& game = ui.game();
  const Vec3d at = game.player()->position;
  const EntityId near_ship = game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice, at + Vec3d{0, 0, 500}, Quat{});
  const EntityId far_ship = game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice, at + Vec3d{0, 0, 7000}, Quat{});
  for (const EntityId id : {near_ship, far_ship}) {
    Entity* e = game.world().Find(id);
    e->manual = true;
    e->controls.fire = true;
    e->laser_damage = 10.0;
  }
  float near_gain = 0.0f;
  float far_gain = 0.0f;
  for (int i = 0; i < 120; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
    for (const SoundCue& cue : ui.TakeSounds()) {
      if (cue.effect == SoundEffect::kEnemyLaser) {
        // The near ship's shots are louder than the far one's; both are quieter than the player's own
        near_gain = std::max(near_gain, cue.gain);
        if (cue.gain < 0.3f) {
          far_gain = std::max(far_gain, cue.gain);
        }
      }
    }
  }
  CHECK(near_gain > 0.0f);
  CHECK(far_gain > 0.0f && far_gain < near_gain);
}

/// Play a game up to the point the ship is out of the bay and flying free
void FlyFree(GameUi& ui) {
  ui.Key(UiKey::kAccept);
  ui.Key(UiKey::kLaunch);
  for (int i = 0; i < 60 * 7; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
  }
  ui.TakeSounds();
}

void TheDesignedEffectsHaveTheirLengthsAndLevels() {
  AdLibSynth synth(1, 44100);
  // The length in seconds and the loudest sample, as a fraction of full scale, that each was designed with
  const struct {
    SoundEffect effect;
    double seconds;
    double peak;
  } designs[] = {
      {SoundEffect::kLaserPulse, 0.22, 0.8},    {SoundEffect::kLaserBeam, 0.5, 0.8},     {SoundEffect::kLaserMilitary, 0.55, 0.85},
      {SoundEffect::kEnemyLaser, 0.3, 0.75},    {SoundEffect::kShieldHit, 0.55, 0.8},    {SoundEffect::kHullHit, 0.5, 0.9},
      {SoundEffect::kShieldsDown, 1.0, 0.8},    {SoundEffect::kShipExplode, 2.4, 1.0},   {SoundEffect::kRockBreak, 0.8, 0.85},
      {SoundEffect::kMissileLaunch, 1.3, 0.85}, {SoundEffect::kMissileBlast, 1.4, 0.95}, {SoundEffect::kMissileLock, 0.9, 0.6},
      {SoundEffect::kTargetBlip, 0.1, 0.45},    {SoundEffect::kEcmPulse, 1.4, 0.8},      {SoundEffect::kEnergyBomb, 3.2, 1.0},
      {SoundEffect::kLaunchBay, 7.0, 0.9},      {SoundEffect::kDocking, 2.8, 0.85},      {SoundEffect::kHyperCharge, 4.4, 0.85},
      {SoundEffect::kHyperExit, 2.0, 0.95},     {SoundEffect::kCargoScoop, 1.0, 0.8},    {SoundEffect::kDockComputerOn, 0.4, 0.5},
      {SoundEffect::kDockComputerOff, 0.4, 0.5}, {SoundEffect::kMassLock, 0.7, 0.7},     {SoundEffect::kLowEnergy, 0.4, 0.45},
      {SoundEffect::kHullAlarm, 1.2, 0.75},     {SoundEffect::kPoliceSiren, 1.6, 0.75},  {SoundEffect::kError, 0.35, 0.6},
      {SoundEffect::kTradeBuy, 0.6, 0.6},       {SoundEffect::kTradeSell, 0.7, 0.65},    {SoundEffect::kEquipmentFit, 1.1, 0.7},
      {SoundEffect::kUiMove, 0.08, 0.35},       {SoundEffect::kUiSelect, 0.2, 0.45},     {SoundEffect::kUiBack, 0.2, 0.4},
      {SoundEffect::kBounty, 0.8, 0.65},        {SoundEffect::kRankUp, 2.0, 0.85},       {SoundEffect::kDeath, 3.4, 0.95},
      {SoundEffect::kTitleSting, 3.0, 0.85},
  };
  for (const auto& d : designs) {
    const std::vector<std::int16_t> samples = synth.Render(d.effect);
    CHECK(samples.size() == static_cast<std::size_t>(d.seconds * 44100.0));
    // Scaled to the planned loudest sample, so the effects sit at the levels that were chosen for them
    CHECK(std::fabs(Peak(samples) / 32767.0 - d.peak) < 0.01);
    // The ends are faded, so there is no click
    CHECK(std::abs(samples.back()) < 400);
    // Alive all the way: not a short sound padded with silence
    std::size_t loud = 0;
    for (const std::int16_t s : samples) {
      loud += std::abs(s) > 300 ? 1 : 0;
    }
    CHECK(loud > samples.size() / 20);
    CHECK(std::string(SoundEffectName(d.effect)) != "?");
  }
  // Every effect has its own name
  for (int i = 0; i < kSoundEffectCount; ++i) {
    for (int k = i + 1; k < kSoundEffectCount; ++k) {
      CHECK(std::string(SoundEffectName(static_cast<SoundEffect>(i))) != std::string(SoundEffectName(static_cast<SoundEffect>(k))));
    }
  }
}

void TheDesignedEffectsMoveInPitchAsDesigned() {
  AdLibSynth synth(1, 44100);
  const auto third = [](const std::vector<std::int16_t>& s, int which) {
    return Crossings(s, s.size() * static_cast<std::size_t>(which) / 3, s.size() * static_cast<std::size_t>(which + 1) / 3);
  };
  // The pulse laser falls through the sound, and the enemy's too
  const std::vector<std::int16_t> pulse = synth.Render(SoundEffect::kLaserPulse);
  CHECK(Crossings(pulse, 0, pulse.size() / 4) > 2 * Crossings(pulse, pulse.size() / 2, pulse.size() * 3 / 4));
  const std::vector<std::int16_t> enemy = synth.Render(SoundEffect::kEnemyLaser);
  CHECK(Crossings(enemy, 0, enemy.size() / 4) > Crossings(enemy, enemy.size() / 2, enemy.size() * 3 / 4));
  // The hyperdrive climbs for the whole of the countdown
  const std::vector<std::int16_t> charge = synth.Render(SoundEffect::kHyperCharge);
  CHECK(third(charge, 1) > 2 * third(charge, 0));
  // The arrival falls
  const std::vector<std::int16_t> exit = synth.Render(SoundEffect::kHyperExit);
  CHECK(Crossings(exit, 0, exit.size() / 8) > 2 * Crossings(exit, exit.size() / 4, exit.size() * 3 / 8));
  // The bay's turbine winds up: the end is much higher than the start
  const std::vector<std::int16_t> bay = synth.Render(SoundEffect::kLaunchBay);
  CHECK(third(bay, 2) > third(bay, 0));
  // The dock computer goes up when it comes on and down when it goes off
  const std::vector<std::int16_t> on = synth.Render(SoundEffect::kDockComputerOn);
  const std::vector<std::int16_t> off = synth.Render(SoundEffect::kDockComputerOff);
  CHECK(Crossings(on, on.size() / 2, on.size()) > Crossings(on, 0, on.size() / 2) / 2);
  CHECK(Crossings(on, 0, on.size() / 4) < Crossings(off, 0, off.size() / 4));
  // Explosions are low: far fewer crossings a second than a laser, and the buzzer is lower than the bell
  const std::vector<std::int16_t> boom = synth.Render(SoundEffect::kHullHit);
  CHECK(Crossings(boom, 0, 4410) < Crossings(pulse, 0, 4410));
  CHECK(Crossings(synth.Render(SoundEffect::kError), 0, 4410) < Crossings(synth.Render(SoundEffect::kTradeBuy), 0, 4410));
}

void TheDesignedEffectsAreDeterministicAndVaryWithTheSeedOnlyIfNoisy() {
  AdLibSynth a(5, 44100);
  AdLibSynth b(5, 44100);
  AdLibSynth c(6, 44100);
  for (int i = static_cast<int>(SoundEffect::kLaserPulse); i < kSoundEffectCount; ++i) {
    CHECK(a.Render(static_cast<SoundEffect>(i)) == b.Render(static_cast<SoundEffect>(i)));
  }
  // Noise and crackle change with the seed; pure bells and tones do not
  CHECK(!(a.Render(SoundEffect::kShipExplode) == c.Render(SoundEffect::kShipExplode)));
  AdLibSynth d(5, 44100);
  AdLibSynth e(6, 44100);
  CHECK(d.Render(SoundEffect::kDockComputerOn) == e.Render(SoundEffect::kDockComputerOn));
  CHECK(d.Render(SoundEffect::kUiSelect) == e.Render(SoundEffect::kUiSelect));
}

void TheEngineDroneFollowsTheThrottle() {
  SoundMixer mixer(44100, 1);
  std::vector<std::int16_t> block(4410);
  // Off: silence
  mixer.Mix(block.data(), block.size());
  CHECK(Peak(block) == 0);
  CHECK(mixer.engine_gain() == 0.0f);
  // On: it fades in over about half a second, rather than starting with a click
  mixer.SetEngine(0.5f);
  mixer.Mix(block.data(), block.size());
  const int first = Peak(block);
  for (int i = 0; i < 20; ++i) {
    mixer.Mix(block.data(), block.size());
  }
  const int settled = Peak(block);
  CHECK(first > 0 && first < settled);
  CHECK(mixer.engine_gain() > 0.95f);
  // Louder and higher with the throttle
  SoundMixer idle(44100, 1);
  SoundMixer full(44100, 1);
  idle.SetEngine(0.0f);
  full.SetEngine(1.0f);
  std::vector<std::int16_t> a(44100), b(44100);
  for (int i = 0; i < 3; ++i) {
    idle.Mix(a.data(), a.size());
    full.Mix(b.data(), b.size());
  }
  double rms_a = 0.0;
  double rms_b = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    rms_a += static_cast<double>(a[i]) * a[i];
    rms_b += static_cast<double>(b[i]) * b[i];
  }
  CHECK(rms_a > 0.0 && rms_b > 1.5 * rms_a);
  // It fades away again when told to
  mixer.SetEngine(-1.0f);
  for (int i = 0; i < 40; ++i) {
    mixer.Mix(block.data(), block.size());
  }
  CHECK(mixer.engine_gain() < 0.001f);
  CHECK(Peak(block) < 50);
  // It sits under the effects: with the drone going, an effect still plays and nothing goes past full scale
  mixer.SetEngine(1.0f);
  mixer.Play(SoundEffect::kShipExplode);
  for (int i = 0; i < 30; ++i) {
    mixer.Mix(block.data(), block.size());
    CHECK(Peak(block) <= 32767);
  }
}

void TheGameGivesTheNewCues() {
  GameUi ui("", 5);
  ui.Key(UiKey::kAccept);
  // Buying and selling, and refusals
  ui.Key(UiKey::kNextTab);  // the market
  ui.TakeSounds();
  ui.game().commander().credits = 0;
  ui.Key(UiKey::kAccept);  // cannot afford it
  std::vector<SoundCue> cues = ui.TakeSounds();
  CHECK(Has(cues, SoundEffect::kError) && !Has(cues, SoundEffect::kUiSelect));
  ui.game().commander().credits = 100000;
  bool bought = false;
  for (int row = 0; row < 17 && !bought; ++row) {
    ui.Key(UiKey::kAccept);
    bought = Has(ui.TakeSounds(), SoundEffect::kTradeBuy);
    if (!bought) {
      ui.Key(UiKey::kDown);
    }
  }
  CHECK(bought);
  ui.Key(UiKey::kAlternate);
  CHECK(Has(ui.TakeSounds(), SoundEffect::kTradeSell));
  ui.Key(UiKey::kNextTab);  // the equipment
  ui.TakeSounds();
  ui.game().commander().fuel = 2.0;
  ui.Key(UiKey::kAccept);  // fuel
  CHECK(Has(ui.TakeSounds(), SoundEffect::kEquipmentFit));
  ui.Key(UiKey::kBack);
  CHECK(Has(ui.TakeSounds(), SoundEffect::kUiBack));
}

void TheLaserSoundsFollowTheLaserFitted() {
  const struct {
    int level;
    SoundEffect effect;
  } lasers[] = {{0, SoundEffect::kLaserPulse}, {1, SoundEffect::kLaserBeam}, {2, SoundEffect::kLaserMilitary}};
  for (const auto& l : lasers) {
    GameUi ui("", 5);
    ui.Key(UiKey::kAccept);
    ui.game().commander().equipment.laser_level = l.level;
    FlyFree(ui);
    FlightInput fire;
    fire.held.set(static_cast<std::size_t>(Action::kFireLasers));
    bool heard = false;
    for (int i = 0; i < 60 && !heard; ++i) {
      ui.Update(fire, 1.0 / 60.0);
      heard = Has(ui.TakeSounds(), l.effect);
    }
    CHECK(heard);
  }
}

void TheShipTellsYouWhatIsWrong() {
  GameUi ui("", 5);
  FlyFree(ui);
  Game& game = ui.game();
  const auto press = [&](Action a) {
    FlightInput in;
    in.pressed.set(static_cast<std::size_t>(a));
    ui.Update(in, 1.0 / 60.0);
    return ui.TakeSounds();
  };
  // Hyperspace with nowhere to go is refused with the buzzer
  CHECK(Has(press(Action::kHyperspace), SoundEffect::kError));
  // With somewhere to go, the station's mass locks it
  const int destination = game.galaxy().InRange(game.commander().system, 7.0).front();
  game.SelectSystem(destination);
  CHECK(game.MassLocked());
  CHECK(Has(press(Action::kHyperspace), SoundEffect::kMassLock));
  // The docking computer chimes on and off
  game.commander().equipment.docking_computer = true;
  CHECK(Has(press(Action::kDockingComputer), SoundEffect::kDockComputerOn));
  CHECK(Has(press(Action::kDockingComputer), SoundEffect::kDockComputerOff));
  // A target chosen, a missile locked
  game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice, game.player()->position + Vec3d{0, 0, 3000}, Quat{});
  CHECK(Has(press(Action::kNextTarget), SoundEffect::kTargetBlip));
  CHECK(Has(press(Action::kTargetMissile), SoundEffect::kMissileLock));
  // A failing hull sounds the alarm, and keeps sounding it
  game.player()->hull = 0.1 * game.player()->hull_max;
  bool alarm = false;
  for (int i = 0; i < 10 && !alarm; ++i) {
    FlightInput none;
    ui.Update(none, 1.0 / 60.0);
    alarm = Has(ui.TakeSounds(), SoundEffect::kHullAlarm);
  }
  CHECK(alarm);
  int alarms = 0;
  for (int i = 0; i < 60 * 6; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
    alarms += Has(ui.TakeSounds(), SoundEffect::kHullAlarm) ? 1 : 0;
  }
  CHECK(alarms >= 2 && alarms <= 3);
  // Out of energy, it is the soft double beep
  game.player()->hull = game.player()->hull_max;
  game.player()->energy = 0.05 * game.player()->energy_max;
  bool low = false;
  for (int i = 0; i < 200 && !low; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
    low = Has(ui.TakeSounds(), SoundEffect::kLowEnergy);
  }
  CHECK(low);
}

void TheHyperdriveSoundsAndTheEngineLevel() {
  GameUi ui("", 5);
  CHECK(ui.EngineLevel() < 0.0f);  // no engine on the title
  ui.Key(UiKey::kAccept);
  CHECK(ui.EngineLevel() < 0.0f);  // or in the station
  ui.Key(UiKey::kLaunch);
  // Through the launch the drone runs up
  const float start = ui.EngineLevel();
  for (int i = 0; i < 60 * 3; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
  }
  const float middle = ui.EngineLevel();
  CHECK(start >= 0.25f && middle > start && middle <= 1.0f);
  for (int i = 0; i < 60 * 5; ++i) {
    ui.Update(FlightInput{}, 1.0 / 60.0);
  }
  CHECK(!ui.game().launching());
  // Then it is the throttle
  FlightInput full;
  full.pressed.set(static_cast<std::size_t>(Action::kThrottleFull));
  ui.Update(full, 1.0 / 60.0);
  CHECK(ui.EngineLevel() > 0.99f);
  FlightInput zero;
  zero.pressed.set(static_cast<std::size_t>(Action::kThrottleZero));
  ui.Update(zero, 1.0 / 60.0);
  CHECK(ui.EngineLevel() < 0.01f && ui.EngineLevel() >= 0.0f);
  // Quiet when paused
  ui.Key(UiKey::kBack);
  CHECK(ui.EngineLevel() < 0.0f);
  ui.Key(UiKey::kBack);
  CHECK(ui.EngineLevel() >= 0.0f);
}

}  // namespace

int main() {
  TheDesignedEffectsHaveTheirLengthsAndLevels();
  TheDesignedEffectsMoveInPitchAsDesigned();
  TheDesignedEffectsAreDeterministicAndVaryWithTheSeedOnlyIfNoisy();
  TheEngineDroneFollowsTheThrottle();
  TheGameGivesTheNewCues();
  TheLaserSoundsFollowTheLaserFitted();
  TheShipTellsYouWhatIsWrong();
  TheHyperdriveSoundsAndTheEngineLevel();
  EveryEffectMakesSound();
  TheOriginalEffectsKeepTheirLengths();
  TheSameSeedGivesTheSameSound();
  TheSampleRateScalesTheLength();
  TheWaveformsAreTheOpl2Ones();
  TheLaserFallsInPitch();
  TheMixerPlaysAndFinishes();
  TheMixerBlockSizeDoesNotMatter();
  VolumeAndGain();
  TheMixerLimitsTheVoices();
  TheGameGivesCues();
  FarAwayEventsAreQuieterThanNearOnes();
  if (g_failures == 0) {
    std::printf("all sound tests passed\n");
    return 0;
  }
  std::fprintf(stderr, "%d sound test checks failed\n", g_failures);
  return 1;
}
