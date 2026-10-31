// Tests of the station music: the score renders, loops without a seam, is not too loud or too quiet, and the mixer and
// the interface play it where they should.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "coriolis/blueprint.h"
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

void TheTitleFightAndJumpThemesAreLoopsToo() {
  const struct {
    MusicTrack track;
    double seconds;
    int bars;
  } pieces[] = {{MusicTrack::kTitle, TitleTheme::Seconds(), TitleTheme::kBars},
                {MusicTrack::kFight, FightTheme::Seconds(), FightTheme::kBars},
                {MusicTrack::kJump, JumpTheme::Seconds(), JumpTheme::kBars}};
  for (const auto& piece : pieces) {
    const std::vector<std::int16_t> theme = RenderMusicTrack(piece.track, 22050);
    CHECK(std::fabs(static_cast<double>(theme.size()) / 22050.0 - piece.seconds) < 0.01);
    int peak = 0;
    for (const std::int16_t s : theme) {
      peak = std::max(peak, std::abs(static_cast<int>(s)));
    }
    CHECK(peak > 12000 && peak < 31000);
    const std::size_t bar = theme.size() / static_cast<std::size_t>(piece.bars);
    for (int b = 0; b < piece.bars; ++b) {
      CHECK(Rms(theme, static_cast<std::size_t>(b) * bar, static_cast<std::size_t>(b + 1) * bar) > 800.0);
    }
    // The end runs into the beginning
    std::int64_t biggest_step = 0;
    for (std::size_t i = 1; i < 4000; ++i) {
      biggest_step = std::max<std::int64_t>(biggest_step, std::llabs(static_cast<std::int64_t>(theme[i]) - theme[i - 1]));
    }
    const std::int64_t seam = std::llabs(static_cast<std::int64_t>(theme.front()) - theme.back());
    CHECK(seam <= std::max<std::int64_t>(2 * biggest_step, 600));
    // The same every time and at any rate
    CHECK(RenderMusicTrack(piece.track, 11025) == RenderMusicTrack(piece.track, 11025));
    const std::size_t a = RenderMusicTrack(piece.track, 11025).size();
    const std::size_t b = RenderMusicTrack(piece.track, 22050).size();
    CHECK(std::llabs(static_cast<long long>(b) - 2 * static_cast<long long>(a)) < 4);
  }
  // They are different pieces
  CHECK(RenderMusicTrack(MusicTrack::kTitle, 11025) != RenderMusicTrack(MusicTrack::kStation, 11025));
  CHECK(RenderMusicTrack(MusicTrack::kFight, 11025).size() != RenderMusicTrack(MusicTrack::kJump, 11025).size());
}

void TheMixerCrossfadesBetweenTracks() {
  SoundMixer mixer(11025, 1);
  std::vector<std::int16_t> out(11025);
  mixer.SetMusic(MusicTrack::kTitle, 0.5f);
  for (int i = 0; i < 4; ++i) {
    mixer.Mix(out.data(), out.size());
  }
  CHECK(mixer.music_gain(MusicTrack::kTitle) > 0.4f);
  CHECK(mixer.music_gain(MusicTrack::kStation) == 0.0f);
  // Ask for the station theme and let the title go: for a moment both are heard, then only the new one
  mixer.SetMusic(MusicTrack::kStation, 0.5f);
  mixer.SetMusic(MusicTrack::kTitle, -1.0f);
  mixer.Mix(out.data(), 2000);
  CHECK(mixer.music_gain(MusicTrack::kTitle) > 0.0f && mixer.music_gain(MusicTrack::kStation) > 0.0f);
  for (int i = 0; i < 6; ++i) {
    mixer.Mix(out.data(), out.size());
  }
  CHECK(mixer.music_gain(MusicTrack::kTitle) == 0.0f);
  CHECK(mixer.music_gain(MusicTrack::kStation) > 0.4f);
  CHECK(Rms(out, 0, out.size()) > 300.0);
}

void TheUiPicksTheMusicForTheMoment() {
  const auto only = [](const GameUi& ui, MusicTrack wanted) {
    for (const MusicTrack t : {MusicTrack::kStation, MusicTrack::kTitle, MusicTrack::kFight, MusicTrack::kJump}) {
      if ((ui.MusicLevel(t) > 0.0f) != (t == wanted)) {
        return false;
      }
    }
    return true;
  };
  GameUi ui("", 5);
  CHECK(only(ui, MusicTrack::kTitle));
  ui.Key(UiKey::kAccept);  // a new game, in the station
  CHECK(only(ui, MusicTrack::kStation));
  ui.Key(UiKey::kLaunch);
  CHECK(ui.screen() == Screen::kFlight);
  ui.Update(FlightInput(), 0.1);
  // Quiet in the first flight: no enemies, no music
  for (const MusicTrack t : {MusicTrack::kStation, MusicTrack::kTitle, MusicTrack::kFight, MusicTrack::kJump}) {
    CHECK(ui.MusicLevel(t) < 0.0f);
  }
  // A pirate close by: the fight theme, which holds for a few seconds after the pirate has gone
  Game& game = ui.game();
  const Blueprint* mamba = FindBuiltInBlueprint("mamba");
  CHECK(mamba != nullptr);
  const EntityId pirate =
      game.world().SpawnShip(*mamba, Team::kPirate, game.player()->position + Vec3d{0, 0, 3000}, Quat{});
  ui.Update(FlightInput(), 0.1);
  CHECK(only(ui, MusicTrack::kFight));
  game.world().Find(pirate)->alive = false;
  ui.Update(FlightInput(), 0.1);
  CHECK(only(ui, MusicTrack::kFight));
  for (int i = 0; i < 12; ++i) {
    ui.Update(FlightInput(), 1.0);
  }
  CHECK(ui.MusicLevel(MusicTrack::kFight) < 0.0f);
  // And again
  const EntityId again =
      game.world().SpawnShip(*mamba, Team::kPirate, game.player()->position + Vec3d{0, 0, 3000}, Quat{});
  ui.Update(FlightInput(), 0.1);
  const float loud = ui.MusicLevel(MusicTrack::kFight);
  CHECK(loud > 0.0f);
  game.world().Find(again)->alive = false;

  // Hyperspace has its own theme, and it takes over from the fight
  game.world().SpawnShip(*mamba, Team::kPirate, game.player()->position + Vec3d{0, 0, 3000}, Quat{});
  ui.Update(FlightInput(), 0.1);
  CHECK(only(ui, MusicTrack::kFight));
  game.player()->position = Vec3d{0, 1.0e6, 0};
  for (Entity& e : game.world().entities()) {
    if (e.kind == EntityKind::kShip && !e.is_player) {
      e.alive = false;
      e.silent_removal = true;
    }
  }
  game.world().Step(1.0 / 60.0);
  const std::vector<int> near = game.galaxy().InRange(game.commander().system, 7.0);
  CHECK(!near.empty());
  game.SelectSystem(near.front());
  CHECK(game.StartJump());
  ui.Update(FlightInput(), 0.1);
  CHECK(only(ui, MusicTrack::kJump));
}

}  // namespace

int main() {
  TheThemeIsALoopOfTheRightLengthAndLevel();
  TheThemeIsTheSameEveryTimeAndAtAnyRate();
  TheMixerFadesTheMusicInAndOutAndStartsItOver();
  TheStationPlaysMusicAndTheFlightDoesNot();
  TheTitleFightAndJumpThemesAreLoopsToo();
  TheMixerCrossfadesBetweenTracks();
  TheUiPicksTheMusicForTheMoment();
  if (g_failures == 0) {
    std::printf("all music tests passed\n");
    return 0;
  }
  std::fprintf(stderr, "%d music test checks failed\n", g_failures);
  return 1;
}
