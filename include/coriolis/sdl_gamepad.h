#ifndef CORIOLIS_SDL_GAMEPAD_H_
#define CORIOLIS_SDL_GAMEPAD_H_

#include <string>

#include "coriolis/input.h"

struct SDL_Gamepad;
union SDL_Event;

namespace coriolis {

/// Reads the first connected controller through SDL3: Xbox 360, Xbox One, Xbox Elite and Elite Series 2 pads, wired or
/// wireless, as SDL supports them. Hot plugging works: feed it the SDL events and it follows pads coming and going.
/// This is the only code that knows about SDL; everything else works on GamepadState.
class SdlGamepad {
 public:
  SdlGamepad() = default;
  ~SdlGamepad();
  SdlGamepad(const SdlGamepad&) = delete;
  SdlGamepad& operator=(const SdlGamepad&) = delete;

  /// Start the SDL gamepad subsystem and open a controller if one is already plugged in. A controller can be
  /// absent: that is not an error.
  bool Init(std::string* error);

  /// Pass every SDL event through, so controllers can be plugged in and removed while the game runs
  void HandleEvent(const SDL_Event& event);

  bool connected() const { return pad_ != nullptr; }
  GamepadKind kind() const { return kind_; }
  /// The name the controller reports, empty if there is none
  std::string name() const;

  /// The current state. Returns false, leaving *state centred, when no controller is connected.
  bool Read(GamepadState* state) const;

  /// Shake the controller. Both strengths are 0 to 1: low is the heavy motor and high the light one. Does nothing if
  /// the controller cannot rumble.
  void Rumble(float low, float high, int milliseconds);

 private:
  void OpenFirst();
  void Close();

  SDL_Gamepad* pad_ = nullptr;
  unsigned id_ = 0;
  GamepadKind kind_ = GamepadKind::kGeneric;
  bool started_ = false;
};

}  // namespace coriolis

#endif  // CORIOLIS_SDL_GAMEPAD_H_
