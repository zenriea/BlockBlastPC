# Block Blast PC

A modern Block Blast puzzle game built for PC with C++ and Raylib.

![Block Blast](icon.png)

## Features

- 🎮 Classic block puzzle gameplay
- 🎨 Modern neon visual style with particle effects
- 🎵 Background music and sound effects
- ⚙️ Settings menu (music/sfx volume, particles, screen shake)
- 💾 High score and settings persistence
- 🖼️ Custom window with draggable title bar

## How to Play

1. Drag blocks from the bottom of the screen to the grid
2. Fill complete rows or columns to clear them and earn points
3. Chain multiple line clears for combo bonuses
4. Game ends when no more blocks can be placed

## Building

### Requirements
- MSYS2 with MinGW-w64 GCC
- Raylib 5.5 (Windows MinGW version)

### Compile
```bash
g++ -o BlockBlast.exe main.cpp icon.o -I"raylib-5.5_win64_mingw-w64/include" -L"raylib-5.5_win64_mingw-w64/lib" -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -static -O2
```

### Adding Icon
```bash
windres -O coff icon.rc -o icon.o
```

## Sound Files

Place the following files in the `sounds/` folder:
- `music.mp3` - Background music
- `place.ogg` - Block placement sound
- `clear.ogg` - Line clear sound
- `gameover.ogg` - Game over sound

## Controls

- **Left Mouse Button** - Drag and drop blocks
- **Title Bar** - Drag to move window
- **X Button** - Close game
- **- Button** - Minimize window

## License

This project is open source.
