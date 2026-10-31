#include "open_elite/music.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace open_elite {

namespace {

constexpr double kTwoPi = 6.283185307179586;

double Hz(double midi) { return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0); }

/// A little noise from a fixed seed, so the music is the same every time
struct Noise {
  std::uint32_t state = 12345;
  float Next() {
    state = state * 1664525U + 1013904223U;
    return static_cast<float>(state >> 8) / 8388608.0f - 1.0f;
  }
  /// 0 to 1
  double Unit() { return 0.5 + 0.5 * static_cast<double>(Next()); }
};

/// A track of samples with room past the end for notes that are still ringing. Folding the room back onto the start
/// makes the loop join up with no seam.
class Track {
 public:
  Track(std::size_t length, std::size_t tail) : length_(length), data_(length + tail, 0.0f) {}

  void Add(std::size_t index, float value) {
    if (index < data_.size()) {
      data_[index] += value;
    }
  }

  std::size_t length() const { return length_; }
  std::size_t size() const { return data_.size(); }

  std::vector<float> Folded() const {
    std::vector<float> out(data_.begin(), data_.begin() + static_cast<std::ptrdiff_t>(length_));
    for (std::size_t i = length_; i < data_.size(); ++i) {
      out[(i - length_) % length_] += data_[i];
    }
    return out;
  }

 private:
  std::size_t length_;
  std::vector<float> data_;
};

/// One cycle of a waveform made of harmonics, to be read at any pitch: far cheaper than adding the harmonics up for
/// every sample of every voice of a long pad
class Wavetable {
 public:
  Wavetable(std::initializer_list<double> partials) : table_(kSize + 1, 0.0f) {
    for (std::size_t i = 0; i < kSize; ++i) {
      double sum = 0.0;
      int k = 1;
      for (const double w : partials) {
        sum += w * std::sin(kTwoPi * k * static_cast<double>(i) / kSize);
        ++k;
      }
      table_[i] = static_cast<float>(sum);
    }
    table_[kSize] = table_[0];
    float peak = 1e-9f;
    for (const float v : table_) {
      peak = std::max(peak, std::fabs(v));
    }
    for (float& v : table_) {
      v /= peak;
    }
  }

  /// The value at a phase in cycles (any real number)
  float At(double cycles) const {
    const double x = (cycles - std::floor(cycles)) * kSize;
    const std::size_t i = static_cast<std::size_t>(x);
    const float f = static_cast<float>(x - static_cast<double>(i));
    return table_[i] + f * (table_[i + 1] - table_[i]);
  }

 private:
  static constexpr std::size_t kSize = 4096;
  std::vector<float> table_;
};

/// A pad voice: three slightly detuned copies of a soft, hollow wave, which swell in over a couple of seconds, breathe
/// slowly, and fade out over several, so that one chord melts into the next
void PadNote(Track& track, const Wavetable& wave, std::size_t start, double midi, double seconds, double level,
             double rate, double breath_phase) {
  const double f = Hz(midi);
  const double attack = 2.4;
  const double release = 3.2;
  const std::size_t count = static_cast<std::size_t>((seconds + 2.5 * release) * rate);
  double p0 = 0.0;
  double p1 = 0.31;
  double p2 = 0.67;
  for (std::size_t i = 0; i < count; ++i) {
    const double t = static_cast<double>(i) / rate;
    double a = std::min(1.0, t / attack);
    a = a * a * (3.0 - 2.0 * a);
    if (t > seconds) {
      a *= std::exp(-(t - seconds) / (release * 0.45));
    }
    const double breath = 0.72 + 0.28 * std::sin(kTwoPi * 0.07 * t + breath_phase);
    const double vibrato = 1.0 + 0.0025 * std::min(1.0, t / 1.5) * std::sin(kTwoPi * 4.3 * t);
    p0 += f * vibrato / rate;
    p1 += f * 1.0035 * vibrato / rate;
    p2 += f * 0.9965 * vibrato / rate;
    const float v = (wave.At(p0) + wave.At(p1) + wave.At(p2)) * (1.0f / 3.0f);
    track.Add(start + i, static_cast<float>(level * a * breath) * v);
  }
}

/// A plucked string, by Karplus-Strong: a burst of noise goes round a delay line that smooths it a little each time
/// round, so the highs die away first, as on a real string. Played softly and left to ring, it is a harp heard from
/// the next room.
void Pluck(Track& track, std::size_t start, double freq, double level, double decay, double rate, Noise& rng) {
  const double period = rate / freq;
  const int length = std::max(2, static_cast<int>(period - 0.5));
  const double frac = period - 0.5 - length;
  const float c = static_cast<float>((1.0 - frac) / (1.0 + frac));

  std::vector<float> line(static_cast<std::size_t>(length));
  float low = 0.0f;
  for (float& v : line) {
    low += 0.55f * (rng.Next() - low);
    v = low;
  }
  const int pick = std::max(1, static_cast<int>(length * 0.22));
  for (int i = length - 1; i >= pick; --i) {
    line[static_cast<std::size_t>(i)] -= 0.8f * line[static_cast<std::size_t>(i - pick)];
  }
  float energy = 0.0f;
  for (const float v : line) {
    energy += v * v;
  }
  const float norm = energy > 0.0f ? 0.5f / std::sqrt(energy / static_cast<float>(length)) : 1.0f;

  const float gain = static_cast<float>(std::min(0.9995, std::exp(-6.9 / (decay * freq))));
  const std::size_t count = static_cast<std::size_t>(decay * 1.3 * rate);
  std::size_t pos = 0;
  float previous = 0.0f;
  float ap_in = 0.0f;
  float ap_out = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    const float current = line[pos];
    const float average = 0.5f * (current + previous);
    previous = current;
    const float tuned = c * average + ap_in - c * ap_out;
    ap_in = average;
    ap_out = tuned;
    line[pos] = gain * tuned;
    pos = (pos + 1) % line.size();
    track.Add(start + i, static_cast<float>(level) * norm * current);
  }
}

/// A glass bell by FM: an inharmonic modulator, whose sparkle fades faster than the carrier, and a long ring
void Bell(Track& track, std::size_t start, double midi, double level, double ring, double rate) {
  const double f = Hz(midi);
  const std::size_t count = static_cast<std::size_t>(ring * 3.4 * rate);
  for (std::size_t i = 0; i < count; ++i) {
    const double t = static_cast<double>(i) / rate;
    const double index = 2.2 * std::exp(-t / (ring * 0.3));
    const double v = std::sin(kTwoPi * f * t + index * std::sin(kTwoPi * f * 3.5 * t));
    const double env = std::min(1.0, t / 0.003) * std::exp(-t / ring);
    track.Add(start + i, static_cast<float>(level * env * v));
  }
}

/// A bell played backwards, so that it swells out of nothing and ends in a sudden silence at `end`
void ReverseBell(Track& track, std::size_t end, double midi, double level, double ring, double rate) {
  const std::size_t count = static_cast<std::size_t>(ring * 3.4 * rate);
  Track scratch(count, 0);
  Bell(scratch, 0, midi, level, ring, rate);
  const std::vector<float> forward = scratch.Folded();
  // Only the first part of the ring is worth having backwards: the rest is too quiet to hear
  const std::size_t used = std::min(count, static_cast<std::size_t>(ring * 1.8 * rate));
  for (std::size_t i = 0; i < used; ++i) {
    const std::size_t from = used - 1 - i;
    const double fade_in = std::min(1.0, static_cast<double>(i) / (0.02 * rate));
    track.Add(end - used + i, forward[from] * static_cast<float>(fade_in));
  }
}

/// A ghostly voice, like a theremin: a pure tone with a little of its harmonics that slides up to each note from the
/// one before, comes in slowly, wavers more as it goes on, and has a breath of air in it, doubled by a second voice a
/// hair out of tune so that it shimmers
void Ghost(Track& track, std::size_t start, double from_midi, double midi, double seconds, double level, double rate,
           Noise& rng) {
  const double attack = 0.4;
  const double release = 0.8;
  const double glide = 0.09;
  const std::size_t count = static_cast<std::size_t>((seconds + 3.0 * release) * rate);
  double phase = 0.0;
  double phase2 = 0.19;
  float breath = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    const double t = static_cast<double>(i) / rate;
    const double pitch = midi + (from_midi - midi) * std::exp(-t / glide);
    const double f = Hz(pitch);
    const double wobble = 0.011 * std::min(1.0, t / 0.9) * std::sin(kTwoPi * 4.7 * t);
    phase += kTwoPi * f * (1.0 + wobble) / rate;
    phase2 += kTwoPi * f * 1.0045 * (1.0 + wobble) / rate;
    double env = std::min(1.0, t / attack);
    env *= env;
    if (t > seconds) {
      env *= std::exp(-(t - seconds) / (release * 0.4));
    }
    breath += 0.15f * (rng.Next() - breath);
    const double v = 0.6 * (std::sin(phase) + 0.2 * std::sin(2.0 * phase) + 0.06 * std::sin(3.0 * phase)) +
                     0.4 * std::sin(phase2) + 0.07 * breath;
    track.Add(start + i, static_cast<float>(level * env * v));
  }
}

/// One thump of the heartbeat: a low sine that falls in pitch and dies quickly
void Thump(Track& track, std::size_t start, double level, double rate) {
  const std::size_t count = static_cast<std::size_t>(0.5 * rate);
  double phase = 0.0;
  for (std::size_t i = 0; i < count; ++i) {
    const double t = static_cast<double>(i) / rate;
    const double f = 36.0 + 30.0 * std::exp(-t / 0.05);
    phase += kTwoPi * f / rate;
    const double env = std::min(1.0, t / 0.004) * std::exp(-t / 0.11);
    track.Add(start + i, static_cast<float>(level * env * std::sin(phase)));
  }
}

/// A sonar ping: a sine that comes in sharply and rings away
void Ping(Track& track, std::size_t start, double midi, double level, double rate) {
  const double f = Hz(midi);
  const std::size_t count = static_cast<std::size_t>(1.2 * rate);
  for (std::size_t i = 0; i < count; ++i) {
    const double t = static_cast<double>(i) / rate;
    const double env = std::min(1.0, t / 0.006) * std::exp(-t / 0.22);
    track.Add(start + i, static_cast<float>(level * env * std::sin(kTwoPi * f * t)));
  }
}

/// A resonant band pass filter whose centre can be moved from sample to sample
struct Band {
  float low = 0.0f;
  float band = 0.0f;
  float Process(float input, float f, float q) {
    low += f * band;
    const float high = input - low - q * band;
    band += f * high;
    return band;
  }
};

/// Wind in the corridors: noise through two slow, resonant band pass filters whose centres wander in cycles that fit the
/// loop exactly, so it comes round without a jump. An extra stretch is made and mixed in at the start with equal power
/// so that the join cannot be heard either.
std::vector<float> Wind(std::size_t length, double rate, Noise& rng) {
  const std::size_t over = static_cast<std::size_t>(2.0 * rate);
  std::vector<float> raw(length + over);
  Band low;
  Band high;
  const double n = static_cast<double>(length);
  for (std::size_t i = 0; i < raw.size(); ++i) {
    const double x = static_cast<double>(i);
    const double fc1 = 320.0 + 200.0 * std::sin(kTwoPi * 4.0 * x / n + 0.7);
    const double fc2 = 1250.0 + 450.0 * std::sin(kTwoPi * 7.0 * x / n + 2.1);
    const float f1 = static_cast<float>(2.0 * std::sin(3.14159265358979 * fc1 / rate));
    const float f2 = static_cast<float>(2.0 * std::sin(3.14159265358979 * fc2 / rate));
    const double gust = 0.55 + 0.45 * std::sin(kTwoPi * 3.0 * x / n + 1.3);
    const float white = rng.Next();
    raw[i] = static_cast<float>(gust) * (1.0f * low.Process(white, f1, 0.09f) + 0.45f * high.Process(white, f2, 0.07f));
  }
  std::vector<float> out(raw.begin(), raw.begin() + static_cast<std::ptrdiff_t>(length));
  for (std::size_t i = 0; i < over; ++i) {
    const double a = 0.5 * 3.14159265358979 * static_cast<double>(i) / static_cast<double>(over);
    out[i] = raw[i] * static_cast<float>(std::sin(a)) + raw[length + i] * static_cast<float>(std::cos(a));
  }
  return out;
}

/// The sub bass drone: D with a partner a few cycles a loop away from it, so the two beat once every twenty four
/// seconds, breathing; and the octave above, fainter. Every frequency is a whole number of cycles in the loop, so the
/// loop joins with no jump.
std::vector<float> Drone(std::size_t length, double rate) {
  const double seconds = static_cast<double>(length) / rate;
  const double base = std::round(36.708 * seconds);
  std::vector<float> out(length);
  for (std::size_t i = 0; i < length; ++i) {
    const double x = static_cast<double>(i) / static_cast<double>(length);
    const double swell = 0.72 + 0.28 * std::sin(kTwoPi * 3.0 * x);
    const double v = 0.55 * std::sin(kTwoPi * base * x) + 0.45 * std::sin(kTwoPi * (base + 3.0) * x) +
                     0.16 * std::sin(kTwoPi * 2.0 * (base + 1.0) * x + 1.0);
    out[i] = static_cast<float>(swell * v);
  }
  return out;
}

/// A long, dark reverb: eight comb filters with a low pass in each loop, so the highs die first and the tail grows
/// dull, feeding two all pass filters that smear it into a cloud. The input is run through a stretch of silence longer
/// than the tail and the result folded back onto the start, which is exactly a reverb that runs round the loop.
std::vector<float> Reverb(const std::vector<float>& dry, double rate) {
  struct Comb {
    std::vector<float> buffer;
    std::size_t position = 0;
    float feedback = 0.0f;
    float damp = 0.0f;
    float low = 0.0f;
    float Process(float x) {
      const float y = buffer[position];
      low = y * (1.0f - damp) + low * damp;
      buffer[position] = x + feedback * low;
      position = (position + 1) % buffer.size();
      return y;
    }
  };
  struct AllPass {
    std::vector<float> buffer;
    std::size_t position = 0;
    float Process(float x) {
      const float d = buffer[position];
      const float y = -0.5f * x + d;
      buffer[position] = x + 0.5f * d;
      position = (position + 1) % buffer.size();
      return y;
    }
  };

  const double reverb_time = 7.0;
  std::array<Comb, 8> combs;
  const std::array<double, 8> delays = {0.0297, 0.0371, 0.0411, 0.0437, 0.0503, 0.0571, 0.0617, 0.0683};
  for (std::size_t k = 0; k < combs.size(); ++k) {
    combs[k].buffer.assign(static_cast<std::size_t>(delays[k] * rate), 0.0f);
    combs[k].feedback = static_cast<float>(std::pow(10.0, -3.0 * delays[k] / reverb_time));
    combs[k].damp = 0.42f;
  }
  std::array<AllPass, 2> all;
  all[0].buffer.assign(static_cast<std::size_t>(0.0050 * rate), 0.0f);
  all[1].buffer.assign(static_cast<std::size_t>(0.0017 * rate), 0.0f);

  const std::size_t length = dry.size();
  const std::size_t tail = static_cast<std::size_t>(9.0 * rate);
  const std::size_t pre = static_cast<std::size_t>(0.045 * rate);
  std::vector<float> out(length, 0.0f);
  for (std::size_t i = 0; i < length + tail; ++i) {
    const float x = i >= pre && i - pre < length ? dry[i - pre] : 0.0f;
    float sum = 0.0f;
    for (Comb& c : combs) {
      sum += c.Process(x);
    }
    float y = sum * 0.11f;
    y = all[0].Process(y);
    y = all[1].Process(y);
    out[i % length] += y;
  }
  return out;
}

struct Chord {
  std::array<int, 5> notes;  // the pad and the harp, low to high
  int root;                  // the bass
};

// Twelve chords, two bars each. Nothing resolves: each one is a colour, not a step to the next
constexpr Chord kDm9{{50, 57, 60, 64, 65}, 38};
constexpr Chord kBbLydian{{53, 57, 58, 62, 64}, 34};   // B flat major seventh with the sharp eleventh in it
constexpr Chord kGm9{{50, 53, 58, 60, 62}, 31};
constexpr Chord kAsusFlat9{{52, 57, 58, 62, 64}, 33};  // the B flat grinds against the A
constexpr Chord kEbMaj7{{51, 55, 58, 62, 67}, 39};     // the flat two of the key: the strangest chord of the piece
constexpr Chord kBbOverD{{50, 58, 62, 65, 69}, 38};
constexpr Chord kDmOpen{{50, 57, 62, 64, 69}, 38};

constexpr std::array<Chord, 12> kChords = {{
    kDm9, kBbLydian, kGm9, kAsusFlat9,   // the opening: bare
    kDm9, kEbMaj7, kBbLydian, kAsusFlat9,  // the middle: fuller, and stranger
    kGm9, kBbOverD, kEbMaj7, kDmOpen,    // the last: it thins to an open fifth
}};

/// A note of the ghost's tune: its length in beats and its pitch
struct Note {
  double beats;
  int midi;
};

/// A phrase of it, starting at a bar
struct Phrase {
  int bar;
  std::array<Note, 4> notes;  // up to four, the rest of no length
};

constexpr std::array<Phrase, 10> kPhrases = {{
    {2, {{{4.0, 69}, {1.0, 70}, {1.0, 69}, {2.0, 67}}}},
    {6, {{{3.0, 76}, {1.0, 74}, {4.0, 70}, {0.0, 0}}}},
    {8, {{{2.0, 74}, {2.0, 77}, {4.0, 76}, {0.0, 0}}}},
    {10, {{{3.0, 79}, {1.0, 77}, {4.0, 74}, {0.0, 0}}}},
    {12, {{{2.0, 76}, {2.0, 77}, {4.0, 79}, {0.0, 0}}}},
    {14, {{{2.0, 82}, {2.0, 81}, {4.0, 76}, {0.0, 0}}}},
    {16, {{{4.0, 74}, {2.0, 72}, {2.0, 70}, {0.0, 0}}}},
    {18, {{{3.0, 77}, {1.0, 74}, {4.0, 69}, {0.0, 0}}}},
    {20, {{{2.0, 67}, {2.0, 70}, {4.0, 74}, {0.0, 0}}}},
    {22, {{{8.0, 69}, {0.0, 0}, {0.0, 0}, {0.0, 0}}}},
}};

}  // namespace

std::vector<std::int16_t> RenderStationTheme(int sample_rate) {
  const double rate = std::max(8000, sample_rate);
  const double beat = 60.0 / StationTheme::kBeatsPerMinute;
  const std::size_t length =
      static_cast<std::size_t>(std::llround(StationTheme::kBars * StationTheme::kBeatsPerBar * beat * rate));
  const std::size_t tail = static_cast<std::size_t>(12.0 * rate);
  const auto at = [&](int bar, double beats) {
    return static_cast<std::size_t>(std::llround((bar * StationTheme::kBeatsPerBar + beats) * beat * rate));
  };
  const double bar_seconds = StationTheme::kBeatsPerBar * beat;

  Track pads(length, tail);
  Track harp(length, tail);
  Track lead(length, tail);
  Track bells(length, tail);
  Track pulse(length, tail);
  Noise rng;
  const Wavetable hollow{1.0, 0.0, 0.34, 0.0, 0.14, 0.0, 0.06};  // the odd harmonics of a soft, hollow wave
  const Wavetable sub{1.0, 0.28, 0.06};

  // The pads and the bass pad: each chord is held for its two bars and a breath more, so that it is still fading when
  // the next one is swelling in
  for (std::size_t c = 0; c < kChords.size(); ++c) {
    const Chord& chord = kChords[c];
    const int bar = static_cast<int>(c) * 2;
    const double seconds = 2.0 * bar_seconds + 1.6;
    const double section_level = c < 4 ? 0.8 : (c < 8 ? 1.0 : 0.9);
    for (std::size_t k = 0; k < 4; ++k) {
      PadNote(pads, hollow, at(bar, 0.0), chord.notes[k], seconds, 0.07 * section_level, rate, 1.7 * static_cast<double>(c) + static_cast<double>(k));
    }
    PadNote(pads, sub, at(bar, 0.0), chord.root, seconds, 0.2 * section_level, rate, 0.4 * static_cast<double>(c));
  }

  // The harp: three notes a bar from the chord, not on the beat, from the fifth bar to the twenty-first
  for (int bar = 4; bar < 22; ++bar) {
    const Chord& chord = kChords[static_cast<std::size_t>(bar / 2)];
    const double where[3] = {0.0, 1.5, 2.75};
    const int shapes[3] = {(bar * 2) % 5, (bar * 2 + 3) % 5, (bar * 3 + 1) % 5};
    for (int k = 0; k < 3; ++k) {
      if (bar >= 16 && k == 1) {
        continue;  // thinning
      }
      const int octave = (bar + k) % 3 == 0 ? 24 : 12;
      const double jitter = 0.03 * static_cast<double>(rng.Next());
      Pluck(harp, at(bar, std::max(0.0, where[k] + jitter)), Hz(chord.notes[static_cast<std::size_t>(shapes[k])] + octave),
            bar < 8 ? 0.16 : 0.22, 2.8, rate, rng);
    }
  }

  // The ghost
  for (const Phrase& phrase : kPhrases) {
    double beats = 0.0;
    double previous = -1.0;
    for (const Note& note : phrase.notes) {
      if (note.beats <= 0.0) {
        continue;
      }
      const double from = previous > 0.0 ? previous : note.midi - 2.5;
      Ghost(lead, at(phrase.bar, beats), from, note.midi, note.beats * beat * 0.97, 0.15, rate, rng);
      beats += note.beats;
      previous = note.midi;
    }
  }

  // The bells: out of the dark, one in some bars, more as it goes on, from the notes of D minor with a flat two thrown
  // in now and then; a few swell into the start of a bar backwards
  static constexpr std::array<int, 9> pool = {74, 77, 81, 84, 86, 89, 93, 75, 80};
  for (int bar = 0; bar < StationTheme::kBars; ++bar) {
    const double chance = bar < 8 ? 0.4 : (bar < 16 ? 0.6 : 0.75);
    if (rng.Unit() > chance) {
      continue;
    }
    const double where = 0.5 + 3.2 * rng.Unit();
    const std::size_t pick = static_cast<std::size_t>(rng.Unit() * (bar < 8 ? 6.99 : 8.99));
    Bell(bells, at(bar, where), pool[pick], bar < 8 ? 0.07 : 0.09, 2.4, rate);
  }
  for (const int bar : {7, 15, 21}) {
    ReverseBell(bells, at(bar + 1, 0.0), bar == 15 ? 75 : 86, 0.1, 2.2, rate);
  }

  // The sonar: a ping that goes out and comes back, three times in the piece
  for (const int bar : {4, 12, 20}) {
    Ping(bells, at(bar, 0.0), 88, 0.07, rate);
    Ping(bells, at(bar, 1.5), 88, 0.04, rate);
    Ping(bells, at(bar, 3.0), 88, 0.02, rate);
  }

  // The heartbeat, from the ninth bar, which comes up and goes away again
  for (int bar = 8; bar < 20; ++bar) {
    const double up = bar < 12 ? (bar - 7) / 5.0 : (bar > 16 ? (20 - bar) / 4.0 : 1.0);
    Thump(pulse, at(bar, 0.0), 0.18 * up, rate);
    Thump(pulse, at(bar, 0.42), 0.11 * up, rate);
    Thump(pulse, at(bar, 2.0), 0.18 * up, rate);
    Thump(pulse, at(bar, 2.42), 0.11 * up, rate);
  }

  const std::vector<float> pads_out = pads.Folded();
  const std::vector<float> harp_out = harp.Folded();
  const std::vector<float> lead_out = lead.Folded();
  const std::vector<float> bells_out = bells.Folded();
  const std::vector<float> pulse_out = pulse.Folded();
  const std::vector<float> wind_out = Wind(length, rate, rng);
  const std::vector<float> drone_out = Drone(length, rate);

  // What goes into the reverb is most of the pads, nearly all of the harp, the ghost and the bells, and a little of the
  // wind and the heartbeat. The drone stays dry, as it is below what a room would colour.
  std::vector<float> send(length);
  for (std::size_t i = 0; i < length; ++i) {
    send[i] = 0.55f * pads_out[i] + 0.85f * harp_out[i] + 1.0f * lead_out[i] + 1.0f * bells_out[i] + 0.3f * wind_out[i] +
              0.2f * pulse_out[i];
  }
  const std::vector<float> wet = Reverb(send, rate);

  std::vector<float> mix(length);
  float peak = 1e-9f;
  for (std::size_t i = 0; i < length; ++i) {
    const float dry = 0.14f * drone_out[i] + 0.5f * pads_out[i] + 0.5f * harp_out[i] + 0.4f * lead_out[i] +
                      0.4f * bells_out[i] + 0.035f * wind_out[i] + 0.8f * pulse_out[i];
    mix[i] = dry + 1.5f * wet[i];
    peak = std::max(peak, std::fabs(mix[i]));
  }
  const float scale = 0.82f / peak;
  std::vector<std::int16_t> out(length);
  for (std::size_t i = 0; i < length; ++i) {
    out[i] = static_cast<std::int16_t>(std::lround(std::tanh(mix[i] * scale * 1.1f) * 30000.0f));
  }
  return out;
}

}  // namespace open_elite
