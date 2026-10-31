#ifndef CORIOLIS_UI_H_
#define CORIOLIS_UI_H_

#include <cstdint>
#include <string>

#include "coriolis/cockpit.h"
#include "coriolis/game.h"
#include "coriolis/model.h"
#include "coriolis/sound.h"
#include "coriolis/types.h"

namespace coriolis {

/// A press of a menu key. The window front end turns keyboard keys and controller buttons into these, and flight
/// controls into a `FlightInput`.
enum class UiKey {
  kUp,
  kDown,
  kLeft,
  kRight,
  /// Choose, buy one, resume
  kAccept,
  /// Sell one, clear the destination
  kAlternate,
  /// Leave the station
  kLaunch,
  kNextTab,
  kPrevTab,
  /// Save the game (in the station)
  kSave,
  /// Escape: pause in flight, back to the title from the station
  kBack,
  /// Switch between classic and Newtonian flight
  kFlightMode,
  /// Show or hide the list of flight keys (the game waits while it is shown)
  kHelp,
};

/// Which screen is showing
enum class Screen { kTitle, kStatus, kMarket, kEquipment, kShipyard, kMissions, kChart, kSystem, kFlight, kDead };

/// The whole game as a player sees it, without a window: the title screen, the screens of the station (status, market,
/// equipment, galactic chart, data on a system), the flight view, the pause menu and the end screen. It owns the game
/// and draws into an image; a front end only has to pass keys and flight controls in and show the image. Because it has
/// no window, it can be tested and played from a script.
class GameUi {
 public:
  /// `save_path` is where the commander is saved and loaded; empty for no saving
  GameUi(std::string save_path, std::uint64_t seed);

  /// A menu key went down
  void Key(UiKey key);

  /// Advance by `seconds`. `input` is the flight control state; it is only read while flying.
  void Update(const FlightInput& input, double seconds);

  void Draw(Image& image);

  Screen screen() const { return screen_; }
  bool paused() const { return paused_; }
  bool chart_open() const { return chart_open_; }
  bool help_open() const { return help_open_; }
  /// True once the player has chosen to quit
  bool wants_quit() const { return quit_; }
  Game& game() { return game_; }
  const Game& game() const { return game_; }
  Cockpit& cockpit() { return cockpit_; }
  /// The most recent message
  const std::string& message() const { return message_; }
  /// A controller rumble strength 0 to 1 (hits on the ship), cleared by the call
  float TakeRumble();
  /// The sounds to play since the last call: shots, hits, explosions, the menu clicks and so on, with how loud each
  /// should be (far off things are quiet). The front end plays them, for example through a `SoundMixer`.
  std::vector<SoundCue> TakeSounds();
  /// How loud the engine drone is to be, 0 to 1, or below 0 when there is none (it is heard while flying)
  float EngineLevel() const;
  /// How loud the station music is to be, 0 to 1, or below 0 when there is none: it plays in the station, on the trading,
  /// equipment, chart and status screens, and fades out when the ship launches. The front end hands it to
  /// `SoundMixer::SetMusic`.
  float MusicLevel() const { return MusicLevel(MusicTrack::kStation); }
  /// The same for each piece of music: the station theme in the station, the title theme on the title screen, the fight
  /// theme in flight while enemies are near (and for a few seconds after), and the jump theme in hyperspace. The front
  /// end hands each to `SoundMixer::SetMusic`, so one fades out as the next comes in.
  float MusicLevel(MusicTrack track) const;

 private:
  void StartNewGame();
  void Continue();
  void Resume();
  void Collect();
  void ShowDockedScreen(Screen screen);
  void MoveDestination(int dx, int dy);
  void Say(const std::string& text);
  void Cue(SoundEffect effect, float gain = 1.0f);
  void CueKey(UiKey key);
  void CueEvents(const std::vector<Event>& events);
  void CueMessages(const std::vector<std::string>& messages);
  void CueState(double seconds);
  bool InCombat() const;
  bool SaveNow();
  int TitleItemCount() const;
  std::string TitleItem(int index) const;
  void ChooseTitleItem(int index);
  void DrawTitle(Image& image);
  void DrawDocked(Image& image);
  void DrawFlight(Image& image);
  void DrawHelp(Image& image);
  void DrawDead(Image& image);
  int DeadItemCount() const;
  std::string DeadItem(int index) const;
  void ChooseDeadItem(int index);
  void DrawBar(Image& image, const std::string& text);

  std::string save_path_;
  Game game_;
  Cockpit cockpit_;
  Model title_model_;
  /// The ship the player died in, for the end screen
  Model wreck_;
  Screen screen_ = Screen::kTitle;
  bool in_progress_ = false;
  bool has_save_ = false;
  bool paused_ = false;
  bool chart_open_ = false;
  bool help_open_ = false;
  bool quit_ = false;
  int title_row_ = 0;
  int pause_row_ = 0;
  int market_row_ = 0;
  int equipment_row_ = 0;
  int shipyard_row_ = 0;
  int dead_row_ = 0;
  /// Seconds since the player died
  double dead_age_ = 0.0;
  std::string message_;
  double message_age_ = 100.0;
  double time_ = 0.0;
  std::uint32_t frame_ = 0;
  float rumble_ = 0.0f;
  std::vector<SoundCue> sounds_;
  bool was_jumping_ = false;
  /// Seconds left of the fight music: it holds for a while after the last enemy has gone
  double fight_hold_ = 0.0;
  // What the sound cues last saw, to cue only changes
  EntityId last_lock_ = kNoEntity;
  EntityId last_target_ = kNoEntity;
  bool last_autopilot_ = false;
  double last_shield_ = 0.0;
  int last_legal_ = 0;
  double alarm_timer_ = 0.0;
};

}  // namespace coriolis

#endif  // CORIOLIS_UI_H_
