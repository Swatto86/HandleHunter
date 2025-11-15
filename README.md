# HandleHunter

A lightweight Windows utility for managing file locks on the local machine, built with Pure C and Win32 API.

## Features

- **System Theme Support**: Automatically detects and respects Windows dark/light mode settings
- **Auto-Start**: Automatically displays open files on the local machine when launched
- **Multi-Select**: Select and release multiple file locks at once (Ctrl+Click, Shift+Click)
- **Search/Filter**: Real-time filtering as you type
- **DPI-Aware**: Crisp display on high-resolution monitors (PerMonitorV2)
- **Lightweight**: ~79KB executable, no dependencies required
- **Fast**: Instant launch, no runtime overhead

## Requirements

- Windows 7 or later
- Administrator privileges (required for NetFileEnum/NetFileClose)
- MinGW-w64 (GCC compiler) for building

## Building

### Prerequisites

1. Install MinGW-w64 from [mingw-w64.org](https://www.mingw-w64.org/) or using winget:
   ```
   winget install mingw-w64
   ```
2. Add MinGW-w64 bin directory to your PATH (e.g., `C:\mingw64\bin`)
3. Verify installation: `gcc --version`

### Build Steps

1. Open **Command Prompt** (cmd.exe) in the project directory
2. Build: `build.bat`

The executable `HandleHunter.exe` will be created with:
- Application manifest embedded (UAC elevation + DPI awareness)
- Visual styles enabled (modern themed controls)
- Application icon embedded

### Clean Build

To remove all build artifacts:
```
build.bat clean
```

## Usage

1. **Run as Administrator**: Right-click `HandleHunter.exe` → "Run as administrator"
2. The application automatically displays all open files on the local machine
3. **Search**: Use the search box to filter results by filename, path, or username
4. **Refresh**: Click "Refresh (F5)" or press F5 to update the list
5. **Multi-Select**: Use Ctrl+Click to select multiple files, Shift+Click for ranges
6. **Release Locks**: Select one or more files and click "Release Selected Lock(s)", press Del, or double-click
7. **Navigate**: Use Tab key to move between controls (search box, buttons, list)

## Keyboard Shortcuts

- **F5**: Refresh file list
- **Del**: Release selected lock(s) (with confirmation)
- **Ctrl+Click**: Select multiple individual files
- **Shift+Click**: Select range of files
- **Ctrl+A**: Select all files
- **Tab**: Navigate between controls
- **Double-click**: Release selected lock(s) (with confirmation)

## Troubleshooting

### Access Denied Error
- Ensure you're running as Administrator
- Check Windows Firewall settings

### Side-by-Side Configuration Error
- This should not occur with properly embedded manifest
- Try rebuilding: `build.bat clean` then `build.bat`

## Project Structure

```
HandleHunter/
├── main.c              # Application entry point (WinMain)
├── gui.c               # GUI creation and window management
├── gui.h               # GUI function declarations
├── modern_ui.c         # Windows theme detection and styling
├── modern_ui.h         # Theme detection function declarations
├── netapi.c            # Windows NetAPI wrapper functions
├── netapi.h            # NetAPI function declarations
├── lockinfo.c          # Dynamic array implementation
├── lockinfo.h          # Data structures (FileLockInfo, LockArray, AppState)
├── resource.h          # Control IDs and resource identifiers
├── HandleHunter.rc     # Resource script (embeds manifest & icon)
├── manifest.xml        # UAC elevation and DPI awareness manifest
├── icon.ico            # Application icon
├── build.bat           # Build script
└── README.md           # This file
```

## Technical Details

- **Language**: Pure C (C99)
- **API**: Win32 API (no frameworks)
- **Libraries**: netapi32.lib, comctl32.lib, dwmapi.lib, uxtheme.lib
- **Unicode**: Full Unicode support (UTF-16)
- **Memory Management**: Manual malloc/free (no garbage collection)
- **DPI Aware**: PerMonitorV2 (Windows 10+) with fallback
- **Visual Styles**: Uses standard Windows controls with system theme detection
- **Modern Features**: Conditional dark mode title bar (based on Windows theme), rounded corners (Windows 11)

## License

This project is provided as-is for educational and practical use.

## Contributing

Feel free to submit issues or pull requests for improvements!
