#ifndef CORIOLIS_MUSIC_H_
#define CORIOLIS_MUSIC_H_

#include <cstdint>
#include <vector>

namespace coriolis {

/// The music of the station: an original instrumental score, written for this game, dark and slow and half empty, like
/// the corridors of a station at the end of a long shift. A sub bass drone breathes under everything. Slow pads of
/// detuned voices swell and fade through chords that never quite settle (a minor ninth, a major seventh with its sharp
/// eleventh, a suspended chord with a flat ninth in it that grinds against the note above), and a ghostly voice like a
/// theremin slides between long notes. A harp is plucked now and then, glass bells ring out of the dark and swell into
/// notes backwards, a sonar ping goes out and comes back as echoes, wind moves through the corridors, and in the middle a
/// slow heartbeat comes up from below. All of it goes through one long, dark reverb, and the loop is made so that the
/// tail of the reverb runs round the end into the beginning. Nothing in it is taken from any existing piece of music.
///
/// It is 24 bars in 4/4 at 80 beats a minute (72 seconds) in D minor: three eight bar sections, a bare opening, a middle
/// that fills with harp and pulse, and a last section that thins out again to the drone, so that it can be looped for as
/// long as the pilot stays in the station.
struct StationTheme {
  static constexpr double kBeatsPerMinute = 80.0;
  static constexpr int kBars = 24;
  static constexpr int kBeatsPerBar = 4;
  /// The length of the loop in seconds
  static constexpr double Seconds() { return kBars * kBeatsPerBar * 60.0 / kBeatsPerMinute; }
};

/// The loop as 16 bit mono samples, made by additive voices, plucked string, FM bell and filtered noise synthesis and a
/// reverb; the same every time. It takes a moment (a second or two) to make.
std::vector<std::int16_t> RenderStationTheme(int sample_rate = 44100);

}  // namespace coriolis

#endif  // CORIOLIS_MUSIC_H_
