# Raylib Binding for y-- (Ymm)

Raylib 2D/3D hardware-accelerated graphics and game programming bindings for the y-- programming language.

## Installation

This library is an official external module for the **y--** (**Ymm**) programming language. You can install it effortlessly via **`yenv`**:

```bash
yenv install raylib
```

This will automatically clone the repository, build the shared library (`liby-raylib.so` / `y-raylib.dll`), and register it in `~/.yenv/lib/`.

## Usage

```ymm
import Raylib
from Raylib import *

define function main(): {
    InitWindow(800, 600, "Raylib Window")
    SetTargetFPS(60)

    while !WindowShouldClose(): {
        BeginDrawing()
        ClearBackground(Raylib.RAYWHITE)
        DrawText("Hello from y-- Raylib!", 190, 200, 20, Raylib.LIGHTGRAY)
        EndDrawing()
    }

    CloseWindow()
}
```

## Documentation

Full documentation, including manual pages and reference texts, are available in the [`docs/`](docs/) directory:
- `raylib.man`: UNIX troff man page
- `raylib.txt`: Plaintext documentation reference

You can view the documentation using:
```bash
man -l docs/raylib.man
```

## License

This project is licensed under the same license as the **y--** programming language. See the [LICENSE](LICENSE) file for details.
