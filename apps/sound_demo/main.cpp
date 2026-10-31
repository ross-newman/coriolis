// sound_demo: write every sound effect as a WAV file, so they can be listened to without running the game.
//
//   sound_demo [--out DIRECTORY] [--rate HZ] [--seed N]

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "open_elite/adlib_synth.h"
#include "open_elite/music.h"

using namespace open_elite;

namespace {

void Put16(std::FILE* f, unsigned v) {
  std::fputc(static_cast<int>(v & 0xFF), f);
  std::fputc(static_cast<int>((v >> 8) & 0xFF), f);
}

void Put32(std::FILE* f, unsigned v) {
  Put16(f, v & 0xFFFF);
  Put16(f, v >> 16);
}

bool WriteWav(const std::string& path, const std::vector<std::int16_t>& samples, int rate) {
  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) {
    return false;
  }
  const unsigned bytes = static_cast<unsigned>(samples.size() * 2);
  std::fputs("RIFF", f);
  Put32(f, 36 + bytes);
  std::fputs("WAVEfmt ", f);
  Put32(f, 16);
  Put16(f, 1);  // PCM
  Put16(f, 1);  // mono
  Put32(f, static_cast<unsigned>(rate));
  Put32(f, static_cast<unsigned>(rate) * 2);
  Put16(f, 2);
  Put16(f, 16);
  std::fputs("data", f);
  Put32(f, bytes);
  for (const std::int16_t s : samples) {
    Put16(f, static_cast<std::uint16_t>(s));
  }
  return std::fclose(f) == 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string out = ".";
  int rate = 44100;
  unsigned seed = 1;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const bool has_value = i + 1 < argc;
    if (arg == "--out" && has_value) {
      out = argv[++i];
    } else if (arg == "--rate" && has_value) {
      rate = std::atoi(argv[++i]);
    } else if (arg == "--seed" && has_value) {
      seed = static_cast<unsigned>(std::strtoul(argv[++i], nullptr, 10));
    } else {
      std::fprintf(stderr, "usage: sound_demo [--out DIRECTORY] [--rate HZ] [--seed N]\n");
      return 2;
    }
  }
  AdLibSynth synth(seed, rate);
  for (int i = 0; i < kSoundEffectCount; ++i) {
    const SoundEffect effect = static_cast<SoundEffect>(i);
    const std::vector<std::int16_t> samples = synth.Render(effect);
    std::string name = SoundEffectName(effect);
    for (char& c : name) {
      if (c == ' ') {
        c = '_';
      }
    }
    const std::string path = out + "/" + name + ".wav";
    if (!WriteWav(path, samples, synth.sample_rate())) {
      std::fprintf(stderr, "cannot write %s\n", path.c_str());
      return 1;
    }
    std::printf("%-16s %5.2f s  %s\n", SoundEffectName(effect), static_cast<double>(samples.size()) / synth.sample_rate(),
                path.c_str());
  }
  // The music of the station, the loop that plays while docked
  const std::vector<std::int16_t> theme = RenderStationTheme(rate);
  const std::string theme_path = out + "/station_theme.wav";
  if (!WriteWav(theme_path, theme, rate)) {
    std::fprintf(stderr, "cannot write %s\n", theme_path.c_str());
    return 1;
  }
  std::printf("%-16s %5.2f s  %s\n", "STATION THEME", static_cast<double>(theme.size()) / rate, theme_path.c_str());
  return 0;
}
