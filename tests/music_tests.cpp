// Tests of the station music: the score renders, loops without a seam, is not too loud or too quiet, and the mixer and
// the interface play it where they should.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "coriolis/music.h"
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

double Rms(const std::vector<std::int16_t>& s, std::size_t from, std::size_t to) {
  double sum = 0.0;
  for (std::size_t i = from; i < to; ++i) {
    sum += static_cast<double>(s[i]) * s[i];
  }
  return std::sqrt(sum / static_cast<double>(std::max<std::size_t>(1, to - from)));
}

void TheThemeIsALoopOfTheRightLengthAndLevel() {
  const std::vector<std::int16_t> theme = RenderStationTheme(22050);
  const double seconds = static_cast<double>(theme.size()) / 22050.0;
  CHECK(std::fabs(seconds - StationTheme::Seconds()) < 0.01);
  CHECK(seconds > 50.0 && seconds < 90.0);
  int peak = 0;
  for (const std::int16_t s : theme) {
    peak = std::max(peak, std::abs(static_cast<int>(s)));
  }
  // Full enough to hear, with room to spare; and music, not a click or a drone: it goes on, bar after bar
  CHECK(peak > 12000 && peak < 31000);
  const std::size_t bar = theme.size() / StationTheme::kBars;
  for (int b = 0; b < StationTheme::kBars; ++b) {
    const double level = Rms(theme, static_cast<std::size_t>(b) * bar, static_cast<std::size_t>(b + 1) * bar);
    CHECK(level > 800.0);
  }
  // The end runs into the beginning: no jump where it loops
  std::int64_t biggest_step = 0;
  for (std::size_t i = 1; i < 4000; ++i) {
    biggest_step = std::max<std::int64_t>(biggest_step, std::llabs(static_cast<std::int64_t>(theme[i]) - theme[i - 1]));
  }
  const std::int64_t seam = std::llabs(static_cast<std::int64_t>(theme.front()) - theme.back());
  CHECK(seam <= std::max<std::int64_t>(2 * biggest_step, 600));
}

void TheThemeIsTheSameEveryTimeAndAtAnyRate() {
  CHECK(RenderStationTheme(11025) == RenderStationTheme(11025));
  const std::size_t a = RenderStationTheme(11025).size();
  const std::size_t b = RenderStationTheme(22050).size();
  CHECK(std::llabs(static_cast<long long>(b) - 2 * static_cast<long long>(a)) < 4);
}

void TheMixerFadesTheMusicInAndOutAndStartsItOver() {
  SoundMixer mixer(11025, 1);
  std::vector<std::int16_t> out(11025);
  mixer.Mix(out.data(), out.size());
  CHECK(Rms(out, 0, out.size()) == 0.0);  // none until asked for
  mixer.SetMusic(0.5f);
  // It fades in over a second or two, then plays
  for (int i = 0; i < 4; ++i) {
    mixer.Mix(out.data(), out.size());
  }
  const double playing = Rms(out, 0, out.size());
  CHECK(playing > 300.0);
  CHECK(mixer.music_gain() > 0.4f);
  // Louder asked for, louder heard
  mixer.SetMusic(1.0f);
  for (int i = 0; i < 4; ++i) {
    mixer.Mix(out.data(), out.size());
  }
  CHECK(Rms(out, 0, out.size()) > 1.3 * playing);
  // Stopped, it fades out and is gone
  mixer.SetMusic(-1.0f);
  for (int i = 0; i < 12; ++i) {
    mixer.Mix(out.data(), out.size());
  }
  CHECK(Rms(out, 0, out.size()) == 0.0);
  CHECK(mixer.music_gain() == 0.0f);
  // Sound effects still play with the music
  mixer.SetMusic(0.5f);
  mixer.Play(SoundEffect::kExplosion);
  mixer.Mix(out.data(), out.size());
  CHECK(Rms(out, 0, out.size()) > 0.0);
}

void TheStationPlaysMusicAndTheFlightDoesNot() {
  GameUi ui("", 5);
  CHECK(ui.MusicLevel() < 0.0f);  // none on the title
  ui.Key(UiKey::kAccept);         // new game: in the station
  CHECK(ui.screen() == Screen::kStatus);
  CHECK(ui.MusicLevel() > 0.0f);
  // Through the trading, equipment and chart screens
  for (int i = 0; i < 6; ++i) {
    ui.Key(UiKey::kNextTab);
    CHECK(ui.MusicLevel() > 0.0f || ui.screen() == Screen::kFlight);
  }
  // It goes when the ship launches
  ui.Key(UiKey::kLaunch);
  CHECK(ui.screen() == Screen::kFlight);
  CHECK(ui.MusicLevel() < 0.0f);
}

}  // namespace

int main() {
  TheThemeIsALoopOfTheRightLengthAndLevel();
  TheThemeIsTheSameEveryTimeAndAtAnyRate();
  TheMixerFadesTheMusicInAndOutAndStartsItOver();
  TheStationPlaysMusicAndTheFlightDoesNot();
  if (g_failures == 0) {
    std::printf("all music tests passed\n");
    return 0;
  }
  std::fprintf(stderr, "%d music test checks failed\n", g_failures);
  return 1;
}
