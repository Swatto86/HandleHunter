# HandleHunter

A lightweight Windows utility for managing file locks on the local machine, built with Pure C and Win32 API.

## Features

- **System Theme Support**: Automatically detects and respects Windows dark/light mode settings
- **Auto-Start**: Automatically displays open files on the local machine when launched
- **Multi-Select**: Select and release multiple file locks at once (Ctrl+Click, Shift+Click)
- **Search/Filter**: Real-time filtering as you type
- **DPI-Aware**: Crisp display on high-resolution monitors (PerMonitorV2)
- **Lightweight**: ~134KB executable, no dependencies required
- **Fast**: Instant launch, no runtime overhead

## Requirements

- Windows 7 or later
- Administrator privileges (required for NetFileEnum/NetFileClose)
- Visual Studio 2022 Build Tools (MSVC compiler) for building

## Building

### Prerequisites

1. Install Visual Studio 2022 Build Tools with C++ support:
   - Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
   - Or use winget: `winget install Microsoft.VisualStudio.2022.BuildTools`
   - Select "Desktop development with C++" workload
2. Ensure `vcvars64.bat` is accessible (build script will locate it automatically)

### Build Steps

1. Open **PowerShell** or **Command Prompt** in the project directory
2. Build: `.\build_msvc.bat`

The executable `HandleHunter.exe` will be created with:
- Application manifest embedded (UAC elevation + DPI awareness)
- Visual styles enabled (modern themed controls)
- Application icon embedded
- Fully CRT-free implementation (except minimal WinMain startup)

### Clean Build

To remove all build artifacts:
```
.\build_msvc.bat clean
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
- Try rebuilding: delete all .obj files and rebuild with `build_msvc.bat`

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
├── lockinfo.c          # Dynamic array implementation (CRT-free)
├── lockinfo.h          # Data structures (FileLockInfo, LockArray, AppState)
├── utilities.c         # CRT-free utility functions (MemCopy, StrSearch, etc.)
├── utilities.h         # CRT-free utility function declarations
├── resource.h          # Control IDs and resource identifiers
├── HandleHunter.rc     # Resource script (embeds manifest & icon)
├── manifest.xml        # UAC elevation and DPI awareness manifest
├── icon.ico            # Application icon
├── build_msvc.bat      # MSVC build script (VS Build Tools required)
└── README.md           # This file
```

## Technical Details

- **Language**: Pure C (C99)
- **Compiler**: Microsoft Visual C++ (MSVC) from VS 2022 Build Tools
- **API**: Win32 API (no frameworks)
- **Libraries**: kernel32.lib, user32.lib, gdi32.lib, advapi32.lib, netapi32.lib, comctl32.lib, dwmapi.lib, uxtheme.lib
- **Unicode**: Full Unicode support (UTF-16)
- **Memory Management**: Windows Heap API (HeapAlloc/HeapFree) - **completely CRT-free**
- **String Operations**: Manual implementations (StrLen, StrSearch, StrToLower, StrCopyN, MemCopy, MemZero)
- **Formatting**: Windows API wsprintfW (user32.lib) instead of stdio.h swprintf
- **Binary Size**: ~137 KB with all features and optimizations
- **DPI Aware**: PerMonitorV2 (Windows 10+) with fallback
- **Visual Styles**: Uses standard Windows controls with system theme detection
- **Modern Features**: Conditional dark mode title bar (based on Windows theme), rounded corners (Windows 11)

## License

This project is provided as-is for educational and practical use.

## Contributing

Feel free to submit issues or pull requests for improvements!
