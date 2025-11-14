# HandleHunter

A lightweight Windows utility for managing file locks on Windows file servers, built with Pure C and Win32 API.

## Features

- **List Open Files**: Display all open files on a server
- **Search/Filter**: Real-time filtering as you type
- **Release Lock**: Close file handles remotely with one click
- **Lightweight**: ~100KB executable, no dependencies required
- **Fast Startup**: Instant launch, no runtime overhead

## Requirements

- Windows 7 or later (Windows XP/Vista may work)
- MinGW-w64 (GCC compiler) for building
- Administrator rights (required for NetFileEnum/NetFileClose)

## Building

### Prerequisites

1. Install MinGW-w64 from [mingw-w64.org](https://www.mingw-w64.org/) or using winget:
   ```
   winget install mingw-w64
   ```
2. Add MinGW-w64 bin directory to your PATH (e.g., `C:\mingw64\bin`)
3. Verify installation by running:
   ```
   test_gcc.bat
   ```

### Build Steps

1. Open **Command Prompt** (cmd.exe) **NOT PowerShell** in the project directory
2. Run the build script:
   ```
   build.bat
   ```
3. The executable `HandleHunter.exe` will be created in the same directory

**Important:** Use Command Prompt (cmd.exe), not PowerShell, as batch files work best in cmd.

### Clean Build

To remove all build artifacts:
```
build.bat clean
```

### Manual Build

If you prefer to build manually:
```bash
gcc -c main.c -o main.o -O2 -DUNICODE -D_UNICODE
gcc -c gui.c -o gui.o -O2 -DUNICODE -D_UNICODE
gcc -c netapi.c -o netapi.o -O2 -DUNICODE -D_UNICODE
gcc -c lockinfo.c -o lockinfo.o -O2 -DUNICODE -D_UNICODE
gcc main.o gui.o netapi.o lockinfo.o -o HandleHunter.exe -mwindows -lnetapi32 -lcomctl32 -O2 -s
```

### Embedding Manifest (Optional)

To enable UAC elevation prompt, embed the manifest:
```bash
mt.exe -manifest manifest.xml -outputresource:HandleHunter.exe;1
```

Note: `mt.exe` comes with Windows SDK or Visual Studio.

## Usage

1. **Run as Administrator**: Right-click `HandleHunter.exe` → "Run as administrator"
2. **Enter Server Name**: Type the server name (e.g., `FILESERVER01` or `\\FILESERVER01`)
3. **Connect**: Click "Connect" button or press Enter
4. **Search**: Use the search box to filter results by filename, path, or username
5. **Release Lock**: Select a file and click "Release Selected Lock" or double-click

## Keyboard Shortcuts

- **F5**: Refresh file list
- **Double-click**: Release selected lock (with confirmation)
- **Enter** (in server box): Connect to server

## Troubleshooting

### Access Denied Error
- Ensure you're running as Administrator
- Verify you have admin rights on the target server
- Check Windows Firewall settings (port 445 for SMB)

### Server Not Found
- Verify server name is correct
- Try using IP address instead of hostname
- Check network connectivity (ping the server)
- Ensure SMB/CIFS is enabled on the server

### UAC Prompt Not Appearing
- Manifest may not be embedded - use `mt.exe` to embed it
- Or compile with resource file that includes manifest

## Project Structure

```
HandleHunter/
├── main.c              # Entry point (WinMain) and event handlers
├── gui.c               # GUI creation and window management
├── gui.h               # GUI function declarations
├── netapi.c            # Windows NetAPI wrapper functions
├── netapi.h            # NetAPI function declarations
├── lockinfo.c          # Dynamic array implementation
├── lockinfo.h          # Data structures (FileLockInfo, LockArray, AppState)
├── resource.h          # Control IDs and resource identifiers
├── manifest.xml         # UAC elevation and DPI awareness manifest
├── build.bat           # Build script
└── README.md           # This file
```

## Technical Details

- **Language**: Pure C (C99)
- **API**: Win32 API (no frameworks)
- **Libraries**: netapi32.lib, comctl32.lib
- **Unicode**: Full Unicode support (UTF-16)
- **Memory Management**: Manual malloc/free (no garbage collection)

## License

This project is provided as-is for educational and practical use.

## Contributing

Feel free to submit issues or pull requests for improvements!

