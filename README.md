# kwin-effects-baspark

Blue Archive style click animations.

Animation logic ported from [DoomVoss/BASpark](https://github.com/DoomVoss/BASpark).

## Dependencies

- Qt 6.6+ (Core, Gui, Quick, OpenGL)
- KF6 (Config, GlobalAccel, I18n, CoreAddons)
- KWin (Development headers)
- Extra CMake Modules (ECM)
- C++20 compiler

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Install

Copy the generated library to the KWin effect plugin directory:

```bash
# for Arch-Linux
sudo cp \
    build/bin/kwin/effects/plugins/baspark.so \
    /usr/lib/qt6/plugins/kwin/effects/plugins
```

After copying, goto **System Settings -> Desktop Effects**, enable **BASpark**, and done.
