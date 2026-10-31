#ifndef OPEN_ELITE_INPUT_H_
#define OPEN_ELITE_INPUT_H_

#include <bitset>
#include <cstddef>
#include <utility>
#include <vector>

namespace open_elite {

/// The buttons of an Xbox style controller. Named by position as SDL does, with the Xbox label in brackets.
/// kLeftTrigger and kRightTrigger are triggers read as buttons: they are never set in GamepadState::buttons, the
/// InputMapper works them out from the analog trigger values.
enum class GamepadButton {
  kSouth,          // A
  kEast,           // B
  kWest,           // X
  kNorth,          // Y
  kBack,           // View
  kStart,          // Menu
  kLeftStick,      // L3
  kRightStick,     // R3
  kLeftShoulder,   // LB
  kRightShoulder,  // RB
  kDpadUp,
  kDpadDown,
  kDpadLeft,
  kDpadRight,
  // The four paddles on the back of the Xbox Elite controllers. The Xbox 360 controller has none.
  kPaddleRightUpper,
  kPaddleRightLower,
  kPaddleLeftUpper,
  kPaddleLeftLower,
  kLeftTrigger,   // LT
  kRightTrigger,  // RT
  kCount
};

constexpr std::size_t kGamepadButtonCount = static_cast<std::size_t>(GamepadButton::kCount);

/// Which controller is plugged in. The Xbox 360 pad has looser sticks than the Elite pads, so it gets a bigger dead
/// zone.
enum class GamepadKind { kGeneric, kXbox360, kXboxElite };

/// One reading of a controller, independent of the library that read it. Sticks run -1 to 1 with up and right
/// positive. Triggers run 0 (released) to 1 (fully pressed).
struct GamepadState {
  float left_x = 0.0f;
  float left_y = 0.0f;
  float right_x = 0.0f;
  float right_y = 0.0f;
  float left_trigger = 0.0f;
  float right_trigger = 0.0f;
  std::bitset<kGamepadButtonCount> buttons;

  bool Down(GamepadButton button) const { return buttons.test(static_cast<std::size_t>(button)); }
  void Set(GamepadButton button, bool down = true) { buttons.set(static_cast<std::size_t>(button), down); }
};

/// Things the pilot can do with a button
enum class Action {
  kFireLasers,
  kFireMissile,
  kTargetMissile,
  kNextTarget,
  kEcm,
  kEnergyBomb,
  kEscapePod,
  kHyperspace,
  kDockingComputer,
  kToggleFlightAssist,
  kViewFront,
  kViewRear,
  kViewLeft,
  kViewRight,
  kGalacticChart,
  kPause,
  kPreviousTarget,
  /// Throttle straight to full, or straight to zero
  kThrottleFull,
  kThrottleZero,
  kCount
};

constexpr std::size_t kActionCount = static_cast<std::size_t>(Action::kCount);

const char* ActionName(Action action);

/// Makes a button do an action. With hold_seconds above 0 the button has to be held that long first, which guards
/// actions that must not happen by accident, like ejecting.
struct ButtonBinding {
  GamepadButton button;
  Action action;
  float hold_seconds = 0.0f;
};

struct InputConfig {
  /// Sticks inside this radius read as centred
  float stick_dead_zone = 0.15f;
  /// Sticks past this radius read as fully deflected
  float stick_outer_zone = 0.97f;
  /// Stick response is deflection raised to this power: 1 is linear, more gives finer control near the centre
  float stick_response = 1.5f;
  /// Pull back to climb. Set to swap to push forward to climb.
  bool invert_pitch = false;
  /// A trigger counts as pressed above the first value and stays so until it falls below the second
  float trigger_press = 0.5f;
  float trigger_release = 0.35f;
  std::vector<ButtonBinding> bindings;
};

/// The default layout for a controller:
///   left stick   pitch and roll          right stick  yaw (left/right) and throttle (up/down)
///   RT lasers, LT missile, RB target missile, LB ECM, L3 next target, R3 eject (hold)
///   A flight assist, B docking computer, X energy bomb, Y hyperspace
///   d-pad front, rear, left and right views, View chart, Menu pause
///   Xbox Elite paddles, so the thumbs can stay on the sticks in a fight: right upper flight assist, right lower
///   next target, left upper energy bomb, left lower docking computer
InputConfig DefaultInputConfig(GamepadKind kind);

/// What the pilot is asking for this frame
struct FlightInput {
  /// -1 to 1. Positive pitch is nose up, positive roll is right wing down, positive yaw is nose right.
  float pitch = 0.0f;
  float roll = 0.0f;
  float yaw = 0.0f;
  /// -1 to 1, positive for more speed
  float throttle = 0.0f;
  /// Actions whose buttons are down (and held long enough), that went down this frame, and that came up
  std::bitset<kActionCount> held;
  std::bitset<kActionCount> pressed;
  std::bitset<kActionCount> released;

  bool Held(Action a) const { return held.test(static_cast<std::size_t>(a)); }
  bool Pressed(Action a) const { return pressed.test(static_cast<std::size_t>(a)); }
  bool Released(Action a) const { return released.test(static_cast<std::size_t>(a)); }
};

/// A stick through the dead zone and response curve. Takes and gives a position in the unit circle.
void ShapeStick(const InputConfig& config, float x, float y, float* out_x, float* out_y);

/// Turns controller readings into flight input, frame by frame. It remembers what was down last frame, so one
/// mapper goes with one controller.
class InputMapper {
 public:
  explicit InputMapper(InputConfig config) : config_(std::move(config)) { Reset(); }

  const InputConfig& config() const { return config_; }
  void SetConfig(InputConfig config) {
    config_ = std::move(config);
    Reset();
  }

  /// Forget the button history, as when the controller is unplugged
  void Reset();

  /// `seconds` is the time since the last call, used for buttons that have to be held
  FlightInput Update(const GamepadState& pad, float seconds);

 private:
  bool ButtonDown(const GamepadState& pad, GamepadButton button) const;

  InputConfig config_;
  std::vector<float> held_for_;
  std::bitset<kActionCount> previous_;
  bool left_trigger_down_ = false;
  bool right_trigger_down_ = false;
};

}  // namespace open_elite

#endif  // OPEN_ELITE_INPUT_H_
