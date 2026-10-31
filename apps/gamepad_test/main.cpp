// Shows what the game sees from a controller: the controller's name and type, then the flight input and the actions
// as they change. Handy for checking an Xbox 360 or Xbox Elite pad, and its paddles, before flying.

#include <SDL3/SDL.h>

#include <cstdio>
#include <string>

#include "coriolis/input.h"
#include "coriolis/sdl_gamepad.h"

using namespace coriolis;

namespace {

const char* KindName(GamepadKind kind) {
  switch (kind) {
    case GamepadKind::kXbox360:
      return "Xbox 360";
    case GamepadKind::kXboxElite:
      return "Xbox Elite";
    case GamepadKind::kGeneric:
      break;
  }
  return "generic";
}

}  // namespace

int main() {
  SdlGamepad gamepad;
  std::string error;
  if (!gamepad.Init(&error)) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }
  std::printf("Waiting for a controller. Move the sticks and press buttons. Press Ctrl+C to quit.\n");

  InputMapper mapper(DefaultInputConfig(GamepadKind::kGeneric));
  bool was_connected = false;
  int last_pitch = 0;
  int last_roll = 0;
  int last_yaw = 0;
  int last_throttle = 0;

  Uint64 last_ticks = SDL_GetTicks();
  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        return 0;
      }
      gamepad.HandleEvent(event);
    }

    GamepadState state;
    const bool connected = gamepad.Read(&state);
    if (connected != was_connected) {
      was_connected = connected;
      if (connected) {
        mapper.SetConfig(DefaultInputConfig(gamepad.kind()));
        std::printf("Connected: %s (%s)\n", gamepad.name().c_str(), KindName(gamepad.kind()));
        // A short buzz shows the rumble motors work
        gamepad.Rumble(0.4f, 0.4f, 200);
      } else {
        mapper.Reset();
        std::printf("Controller disconnected\n");
      }
    }

    const Uint64 now = SDL_GetTicks();
    const float seconds = static_cast<float>(now - last_ticks) / 1000.0f;
    last_ticks = now;
    const FlightInput input = mapper.Update(state, seconds);

    // Print the axes in hundredths, only when one changes, so the terminal stays readable
    const int pitch = static_cast<int>(input.pitch * 100.0f);
    const int roll = static_cast<int>(input.roll * 100.0f);
    const int yaw = static_cast<int>(input.yaw * 100.0f);
    const int throttle = static_cast<int>(input.throttle * 100.0f);
    if (pitch != last_pitch || roll != last_roll || yaw != last_yaw || throttle != last_throttle) {
      last_pitch = pitch;
      last_roll = roll;
      last_yaw = yaw;
      last_throttle = throttle;
      std::printf("pitch %4d  roll %4d  yaw %4d  throttle %4d\n", pitch, roll, yaw, throttle);
    }
    for (std::size_t i = 0; i < kActionCount; ++i) {
      const Action action = static_cast<Action>(i);
      if (input.Pressed(action)) {
        std::printf("pressed:  %s\n", ActionName(action));
      }
      if (input.Released(action)) {
        std::printf("released: %s\n", ActionName(action));
      }
    }
    std::fflush(stdout);
    SDL_Delay(16);
  }
}
