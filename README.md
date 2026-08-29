# LunaPassport Patcher

Win32 GUI utility for Windows XP. Produces a single static `.exe` that configures a LunaPassport lab machine.

See [README.ru.md](README.ru.md) for full Russian documentation.

## Build (Windows)

Requirements: CMake + MinGW-w64 **32-bit** (`g++` in PATH).

```bat
build.bat
```

Output: `build\lunapassport-patcher.exe`

## Usage

1. Run as Administrator on Windows XP.
2. Enter server IP and base domain (e.g. `pidoras.top`).
3. Click **Применить** (Apply).
4. Restart IE6 and open the LunaPassport landing page.
