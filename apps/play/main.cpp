// open_elite: the game in a window. It reads the keyboard and an Xbox style controller, and shows the title screen, the
// station screens and the flight view that GameUi draws.
//
//   open_elite [--seed N] [--size WxH] [--window WxH] [--quality 1-4] [--save FILE] [--fullscreen] [--invert-pitch]
//              [--autostart] [--frames N --screenshot FILE] [--no-sound] [--volume 0-100]
//
// Keys:  flight    arrows pitch and roll (Q E also roll), A D yaw, W S throttle, X full throttle, Z throttle off,
//                  hold Shift for fine control, Space lasers, T lock missile, M fire missile, Tab and Shift+Tab
//                  next and previous target, C ECM, B energy bomb, H hyperspace, K docking computer,
//                  Y flight assist, N classic or Newtonian flight, 1 2 3 4 views, G galactic chart, P or Esc
//                  pause, F1 the list of keys, hold Backspace to eject in the escape pod
//        station   Tab next screen, arrows and Enter to choose and buy, Backspace sell, L launch, S save, Esc menu
//        F11 or Alt+Enter full screen on and off, F2 sound on and off

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "open_elite/input.h"
#include "open_elite/png.h"
#include "open_elite/sound.h"
#include "open_elite/ui.h"

using namespace open_elite;

namespace {

bool ParseSize(const char* text, int* w, int* h) {
  int a = 0;
  int b = 0;
  if (std::sscanf(text, "%dx%d", &a, &b) != 2 || a < 160 || b < 120 || a > 4096 || b > 4096) {
    return false;
  }
  *w = a;
  *h = b;
  return true;
}

std::string DefaultSavePath() {
  const char* home = std::getenv("HOME");
  return std::string(home != nullptr ? home : ".") + "/.open_elite_save.json";
}

/// A controller through SDL2's game controller interface
class Controller {
 public:
  ~Controller() { Close(); }

  void OpenFirst() {
    if (pad_ != nullptr) {
      return;
    }
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
      if (SDL_IsGameController(i) != 0) {
        pad_ = SDL_GameControllerOpen(i);
        if (pad_ != nullptr) {
          id_ = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad_));
          const bool paddles = SDL_GameControllerHasButton(pad_, SDL_CONTROLLER_BUTTON_PADDLE1) == SDL_TRUE;
          const SDL_GameControllerType type = SDL_GameControllerGetType(pad_);
          kind_ = paddles ? GamepadKind::kXboxElite
                          : (type == SDL_CONTROLLER_TYPE_XBOX360 ? GamepadKind::kXbox360 : GamepadKind::kGeneric);
          std::printf("controller: %s\n", SDL_GameControllerName(pad_));
          return;
        }
      }
    }
  }

  void Close() {
    if (pad_ != nullptr) {
      SDL_GameControllerClose(pad_);
      pad_ = nullptr;
    }
  }

  void Handle(const SDL_Event& e) {
    if (e.type == SDL_CONTROLLERDEVICEADDED) {
      OpenFirst();
    } else if (e.type == SDL_CONTROLLERDEVICEREMOVED && pad_ != nullptr && e.cdevice.which == id_) {
      Close();
    }
  }

  bool connected() const { return pad_ != nullptr; }
  GamepadKind kind() const { return kind_; }

  GamepadState Read() const {
    GamepadState s;
    if (pad_ == nullptr) {
      return s;
    }
    const auto axis = [&](SDL_GameControllerAxis a) {
      return static_cast<float>(SDL_GameControllerGetAxis(pad_, a)) / 32767.0f;
    };
    s.left_x = axis(SDL_CONTROLLER_AXIS_LEFTX);
    s.left_y = -axis(SDL_CONTROLLER_AXIS_LEFTY);
    s.right_x = axis(SDL_CONTROLLER_AXIS_RIGHTX);
    s.right_y = -axis(SDL_CONTROLLER_AXIS_RIGHTY);
    s.left_trigger = std::max(0.0f, axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    s.right_trigger = std::max(0.0f, axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    const struct {
      SDL_GameControllerButton from;
      GamepadButton to;
    } map[] = {
        {SDL_CONTROLLER_BUTTON_A, GamepadButton::kSouth},
        {SDL_CONTROLLER_BUTTON_B, GamepadButton::kEast},
        {SDL_CONTROLLER_BUTTON_X, GamepadButton::kWest},
        {SDL_CONTROLLER_BUTTON_Y, GamepadButton::kNorth},
        {SDL_CONTROLLER_BUTTON_BACK, GamepadButton::kBack},
        {SDL_CONTROLLER_BUTTON_START, GamepadButton::kStart},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, GamepadButton::kLeftStick},
        {SDL_CONTROLLER_BUTTON_RIGHTSTICK, GamepadButton::kRightStick},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, GamepadButton::kLeftShoulder},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, GamepadButton::kRightShoulder},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, GamepadButton::kDpadUp},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, GamepadButton::kDpadDown},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, GamepadButton::kDpadLeft},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, GamepadButton::kDpadRight},
        {SDL_CONTROLLER_BUTTON_PADDLE1, GamepadButton::kPaddleRightUpper},
        {SDL_CONTROLLER_BUTTON_PADDLE2, GamepadButton::kPaddleLeftUpper},
        {SDL_CONTROLLER_BUTTON_PADDLE3, GamepadButton::kPaddleRightLower},
        {SDL_CONTROLLER_BUTTON_PADDLE4, GamepadButton::kPaddleLeftLower},
    };
    for (const auto& m : map) {
      if (SDL_GameControllerGetButton(pad_, m.from) != 0) {
        s.Set(m.to);
      }
    }
    return s;
  }

  void Rumble(float strength) {
    if (pad_ != nullptr && strength > 0.0f) {
      const Uint16 v = static_cast<Uint16>(std::min(1.0f, strength) * 65535.0f);
      SDL_GameControllerRumble(pad_, v, v / 2, 120);
    }
  }

 private:
  SDL_GameController* pad_ = nullptr;
  SDL_JoystickID id_ = 0;
  GamepadKind kind_ = GamepadKind::kGeneric;
};

/// The sound effects through SDL2 audio: the synthesizer's effects are made once, and the audio thread mixes whatever
/// is playing whenever the device wants more. The game works with no audio device; the sound is just left out.
class Audio {
 public:
  Audio() = default;
  ~Audio() { Close(); }
  Audio(const Audio&) = delete;
  Audio& operator=(const Audio&) = delete;

  bool Open(float volume) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
      std::fprintf(stderr, "no sound: %s\n", SDL_GetError());
      return false;
    }
    mixer_ = std::make_unique<SoundMixer>(kRate, 1);
    mixer_->set_master_volume(volume);
    mixer_->PrepareMusic();  // a moment's work now, so there is no pause when the ship first docks
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = kRate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = &Audio::Callback;
    want.userdata = this;
    SDL_AudioSpec have;
    device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (device_ == 0) {
      std::fprintf(stderr, "no sound: %s\n", SDL_GetError());
      mixer_.reset();
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
      return false;
    }
    SDL_PauseAudioDevice(device_, 0);
    return true;
  }

  void Close() {
    if (device_ != 0) {
      SDL_CloseAudioDevice(device_);
      device_ = 0;
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
  }

  bool open() const { return device_ != 0; }
  bool muted() const { return muted_; }
  void ToggleMute() { muted_ = !muted_; }

  void Play(const SoundCue& cue) {
    if (device_ == 0 || muted_) {
      return;
    }
    SDL_LockAudioDevice(device_);
    mixer_->Play(cue);
    SDL_UnlockAudioDevice(device_);
  }

  /// The engine drone's level, 0 to 1, or below 0 for none
  void SetEngine(float level) {
    if (device_ == 0) {
      return;
    }
    SDL_LockAudioDevice(device_);
    mixer_->SetEngine(muted_ ? -1.0f : level);
    SDL_UnlockAudioDevice(device_);
  }

  /// The station music's level, 0 to 1, or below 0 for none
  void SetMusic(float level) {
    if (device_ == 0) {
      return;
    }
    SDL_LockAudioDevice(device_);
    mixer_->SetMusic(muted_ ? -1.0f : level);
    SDL_UnlockAudioDevice(device_);
  }

 private:
  static constexpr int kRate = 44100;

  static void Callback(void* user, Uint8* stream, int length) {
    Audio* self = static_cast<Audio*>(user);
    self->mixer_->Mix(reinterpret_cast<std::int16_t*>(stream), static_cast<std::size_t>(length) / sizeof(std::int16_t));
  }

  std::unique_ptr<SoundMixer> mixer_;
  SDL_AudioDeviceID device_ = 0;
  bool muted_ = false;
};

void Set(FlightInput* in, Action a, bool pressed, bool held = false) {
  const std::size_t i = static_cast<std::size_t>(a);
  if (pressed) {
    in->pressed.set(i);
  }
  if (held || pressed) {
    in->held.set(i);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::uint64_t seed = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
  int width = 960;
  int height = 540;
  int window_w = 1280;
  int window_h = 720;
  int quality = 1;
  bool sound = true;
  float volume = 0.7f;
  bool fullscreen = false;
  bool invert = false;
  bool autostart = false;
  int frames = -1;
  std::string screenshot;
  std::string save = DefaultSavePath();
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const bool has_value = i + 1 < argc;
    if (arg == "--seed" && has_value) {
      seed = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--size" && has_value) {
      if (!ParseSize(argv[++i], &width, &height)) {
        std::fprintf(stderr, "bad size, expected WxH from 160x120 to 4096x4096\n");
        return 2;
      }
    } else if (arg == "--window" && has_value) {
      if (!ParseSize(argv[++i], &window_w, &window_h)) {
        std::fprintf(stderr, "bad window size\n");
        return 2;
      }
    } else if (arg == "--quality" && has_value) {
      quality = std::atoi(argv[++i]);
      if (quality < 1 || quality > 4) {
        std::fprintf(stderr, "bad quality, expected 1 to 4\n");
        return 2;
      }
    } else if (arg == "--save" && has_value) {
      save = argv[++i];
    } else if (arg == "--no-sound") {
      sound = false;
    } else if (arg == "--volume" && has_value) {
      volume = std::clamp(static_cast<float>(std::atof(argv[++i])) / 100.0f, 0.0f, 1.0f);
    } else if (arg == "--no-save") {
      save.clear();
    } else if (arg == "--fullscreen") {
      fullscreen = true;
    } else if (arg == "--invert-pitch") {
      invert = true;
    } else if (arg == "--autostart") {
      autostart = true;
    } else if (arg == "--frames" && has_value) {
      frames = std::atoi(argv[++i]);
    } else if (arg == "--screenshot" && has_value) {
      screenshot = argv[++i];
    } else {
      std::fprintf(stderr,
                   "usage: open_elite [--seed N] [--size WxH] [--window WxH] [--quality 1-4] [--save FILE | --no-save] "
                   "[--fullscreen] [--no-sound] [--volume 0-100] "
                   "[--invert-pitch] [--autostart] [--frames N --screenshot FILE]\n");
      return 2;
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
    std::fprintf(stderr, "SDL could not start: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_Window* window =
      SDL_CreateWindow("Open Elite", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h,
                       SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
  SDL_Renderer* renderer = window != nullptr
                               ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
                               : nullptr;
  if (window != nullptr && renderer == nullptr) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (window == nullptr || renderer == nullptr) {
    std::fprintf(stderr, "SDL could not open a window: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  // No logical size: the picture is made to fit the window's shape (see the main loop), so it fills the whole window and
  // full screen with no black bars, whatever the shape of the screen
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
  if (texture == nullptr) {
    std::fprintf(stderr, "SDL could not make a texture: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  Controller controller;
  const auto toggle_fullscreen = [&] {
    const bool is_full = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
    SDL_SetWindowFullscreen(window, is_full ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
  };
  controller.OpenFirst();
  InputMapper mapper(DefaultInputConfig(controller.kind()));
  GamepadKind mapper_kind = controller.kind();

  GameUi ui(save, seed);
  Audio audio;
  if (sound) {
    audio.Open(volume);
  }
  ui.cockpit().set_supersample(quality);
  Image image(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
  if (autostart) {
    ui.Key(UiKey::kAccept);  // the first item is New Game when nothing is saved, and Continue when there is a save
    ui.Key(UiKey::kLaunch);
  }

  FlightInput keys_pressed;
  double eject_hold = 0.0;
  float turn[3] = {0.0f, 0.0f, 0.0f};  // the keyboard's pitch, roll and yaw, eased
  bool running = true;
  int frame_count = 0;
  Uint64 last = SDL_GetPerformanceCounter();
  const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

  while (running && !ui.wants_quit()) {
    SDL_Event e;
    keys_pressed = FlightInput{};
    while (SDL_PollEvent(&e) != 0) {
      controller.Handle(e);
      if (e.type == SDL_QUIT) {
        running = false;
      } else if (e.type == SDL_KEYDOWN) {
        const SDL_Keycode k = e.key.keysym.sym;
        const bool shift = (e.key.keysym.mod & KMOD_SHIFT) != 0;
        const bool repeat = e.key.repeat != 0;
        switch (k) {
          case SDLK_UP:
            ui.Key(UiKey::kUp);
            break;
          case SDLK_DOWN:
            ui.Key(UiKey::kDown);
            break;
          case SDLK_LEFT:
            ui.Key(UiKey::kLeft);
            break;
          case SDLK_RIGHT:
            ui.Key(UiKey::kRight);
            break;
          case SDLK_RETURN:
          case SDLK_KP_ENTER:
            if ((e.key.keysym.mod & KMOD_ALT) != 0) {
              if (!repeat) toggle_fullscreen();
            } else {
              ui.Key(UiKey::kAccept);
            }
            break;
          case SDLK_BACKSPACE:
            ui.Key(UiKey::kAlternate);
            break;
          case SDLK_ESCAPE:
            ui.Key(UiKey::kBack);
            break;
          case SDLK_TAB:
            if (!repeat) {
              ui.Key(shift ? UiKey::kPrevTab : UiKey::kNextTab);
              Set(&keys_pressed, shift ? Action::kPreviousTarget : Action::kNextTarget, true);
            }
            break;
          case SDLK_F1:
            if (!repeat) ui.Key(UiKey::kHelp);
            break;
          case SDLK_l:
            if (!repeat) ui.Key(UiKey::kLaunch);
            break;
          case SDLK_s:
            if (!repeat) ui.Key(UiKey::kSave);
            break;
          case SDLK_n:
            if (!repeat) ui.Key(UiKey::kFlightMode);
            break;
          case SDLK_F2:
            if (!repeat) audio.ToggleMute();
            break;
          case SDLK_F11:
            if (!repeat) toggle_fullscreen();
            break;
          default:
            break;
        }
        if (!repeat) {
          switch (k) {
            case SDLK_x:
              Set(&keys_pressed, Action::kThrottleFull, true);
              break;
            case SDLK_z:
              Set(&keys_pressed, Action::kThrottleZero, true);
              break;
            case SDLK_m:
              Set(&keys_pressed, Action::kFireMissile, true);
              break;
            case SDLK_t:
              Set(&keys_pressed, Action::kTargetMissile, true);
              break;
            case SDLK_c:
              Set(&keys_pressed, Action::kEcm, true);
              break;
            case SDLK_b:
              Set(&keys_pressed, Action::kEnergyBomb, true);
              break;
            case SDLK_h:
              Set(&keys_pressed, Action::kHyperspace, true);
              break;
            case SDLK_k:
              Set(&keys_pressed, Action::kDockingComputer, true);
              break;
            case SDLK_y:
              Set(&keys_pressed, Action::kToggleFlightAssist, true);
              break;
            case SDLK_p:
              Set(&keys_pressed, Action::kPause, true);
              break;
            case SDLK_g:
              Set(&keys_pressed, Action::kGalacticChart, true);
              break;
            case SDLK_1:
              Set(&keys_pressed, Action::kViewFront, true);
              break;
            case SDLK_2:
            case SDLK_r:
              Set(&keys_pressed, Action::kViewRear, true);
              break;
            case SDLK_3:
            case SDLK_COMMA:
              Set(&keys_pressed, Action::kViewLeft, true);
              break;
            case SDLK_4:
            case SDLK_PERIOD:
              Set(&keys_pressed, Action::kViewRight, true);
              break;
            default:
              break;
          }
        }
      } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        // Menus. In flight the controller's own bindings (below) handle pause and the chart.
        const bool flying = ui.screen() == Screen::kFlight && !ui.paused() && !ui.chart_open();
        switch (e.cbutton.button) {
          case SDL_CONTROLLER_BUTTON_DPAD_UP:
            ui.Key(UiKey::kUp);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            ui.Key(UiKey::kDown);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            ui.Key(UiKey::kLeft);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            ui.Key(UiKey::kRight);
            break;
          case SDL_CONTROLLER_BUTTON_A:
            ui.Key(UiKey::kAccept);
            break;
          case SDL_CONTROLLER_BUTTON_X:
            ui.Key(UiKey::kAlternate);
            break;
          case SDL_CONTROLLER_BUTTON_Y:
            ui.Key(UiKey::kLaunch);
            break;
          case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            ui.Key(UiKey::kPrevTab);
            break;
          case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            ui.Key(UiKey::kNextTab);
            break;
          case SDL_CONTROLLER_BUTTON_START:
            if (!flying && ui.screen() != Screen::kFlight) {
              ui.Key(UiKey::kBack);
            }
            break;
          case SDL_CONTROLLER_BUTTON_BACK:
            ui.Key(UiKey::kSave);
            break;
          default:
            break;
        }
      }
    }

    const Uint64 now = SDL_GetPerformanceCounter();
    double dt = frames >= 0 ? 1.0 / 60.0 : std::min(0.1, static_cast<double>(now - last) / frequency);
    last = now;

    // Flight controls: the keyboard and the controller together
    const Uint8* kb = SDL_GetKeyboardState(nullptr);
    FlightInput in;
    const auto key = [&](int a, int b) { return kb[a] != 0 || kb[b] != 0; };
    const auto axis = [&](bool plus, bool minus) { return (plus ? 1.0f : 0.0f) - (minus ? 1.0f : 0.0f); };
    // Keys are all or nothing, so the turn is eased in over a sixth of a second: a quick tap is a small correction.
    // Holding Shift is fine control, for lining up a shot.
    const float fine = key(SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT) ? 0.35f : 1.0f;
    const float wanted[3] = {
        axis(kb[SDL_SCANCODE_DOWN] != 0 || kb[SDL_SCANCODE_KP_2] != 0, kb[SDL_SCANCODE_UP] != 0 || kb[SDL_SCANCODE_KP_8] != 0) * (invert ? -1.0f : 1.0f) * fine,
        axis(key(SDL_SCANCODE_RIGHT, SDL_SCANCODE_E), key(SDL_SCANCODE_LEFT, SDL_SCANCODE_Q)) * fine,
        axis(kb[SDL_SCANCODE_D] != 0, kb[SDL_SCANCODE_A] != 0) * fine,
    };
    for (int i = 0; i < 3; ++i) {
      const bool building = std::fabs(wanted[i]) > std::fabs(turn[i]) && wanted[i] * turn[i] >= 0.0f;
      const float step = static_cast<float>(dt) * (building ? 6.0f : 14.0f);
      turn[i] += std::clamp(wanted[i] - turn[i], -step, step);
    }
    in.pitch = turn[0];
    in.roll = turn[1];
    in.yaw = turn[2];
    in.throttle = axis(kb[SDL_SCANCODE_W] != 0, kb[SDL_SCANCODE_S] != 0);
    Set(&in, Action::kFireLasers, false, kb[SDL_SCANCODE_SPACE] != 0);
    in.pressed |= keys_pressed.pressed;
    in.held |= keys_pressed.held;

    if (controller.connected()) {
      if (controller.kind() != mapper_kind) {
        mapper_kind = controller.kind();
        mapper.SetConfig(DefaultInputConfig(mapper_kind));
      }
      const FlightInput pad = mapper.Update(controller.Read(), static_cast<float>(dt));
      in.pitch = std::clamp(in.pitch + pad.pitch, -1.0f, 1.0f);
      in.roll = std::clamp(in.roll + pad.roll, -1.0f, 1.0f);
      in.yaw = std::clamp(in.yaw + pad.yaw, -1.0f, 1.0f);
      in.throttle = std::clamp(in.throttle + pad.throttle, -1.0f, 1.0f);
      in.held |= pad.held;
      in.pressed |= pad.pressed;
      in.released |= pad.released;
    }

    // Ejecting takes a deliberate hold of Backspace
    if (kb[SDL_SCANCODE_BACKSPACE] != 0 && ui.screen() == Screen::kFlight && !ui.paused() && !ui.chart_open() && !ui.help_open()) {
      const double before = eject_hold;
      eject_hold += dt;
      if (before < 1.0 && eject_hold >= 1.0) {
        Set(&in, Action::kEscapePod, true);
      }
    } else {
      eject_hold = 0.0;
    }

    ui.Update(in, dt);
    controller.Rumble(ui.TakeRumble());
    for (const SoundCue& cue : ui.TakeSounds()) {
      audio.Play(cue);
    }
    audio.SetEngine(ui.EngineLevel());
    audio.SetMusic(ui.MusicLevel());

    // Keep the height, and take the width from the shape of the window, so the picture always fills it. The pixels stay
    // square, and the layout of everything follows the size of the picture.
    int out_w = 0;
    int out_h = 0;
    if (SDL_GetRendererOutputSize(renderer, &out_w, &out_h) == 0 && out_w > 0 && out_h > 0) {
      const int want_w = std::clamp(static_cast<int>(std::lround(static_cast<double>(height) * out_w / out_h)), 160, 4096);
      if (want_w != width) {
        SDL_Texture* next = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, want_w, height);
        if (next != nullptr) {
          SDL_DestroyTexture(texture);
          texture = next;
          width = want_w;
          image = Image(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
        }
      }
    }
    ui.Draw(image);
    SDL_UpdateTexture(texture, nullptr, image.data(), width * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    ++frame_count;
    if (frames >= 0 && frame_count >= frames) {
      running = false;
    }
  }

  if (!screenshot.empty() && !WritePng(image, screenshot)) {
    std::fprintf(stderr, "could not write %s\n", screenshot.c_str());
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
