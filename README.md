# X4Launcher

Modern Minecraft launcher written in C++20 with Qt6, focused on performance, portability and full control over the launch process.

---

## Overview

X4Launcher is a custom Minecraft launcher that implements the full launch pipeline:

* Microsoft OAuth authentication (Xbox Live + XSTS + Minecraft services)
* Instance-based game management
* Java detection and provisioning
* Version parsing and launch argument resolution
* Asset and library caching
* Modrinth integration for mod browsing and downloads

The project is designed to be lightweight, modular and cross-platform (Linux / Windows).

---

## Features

### Authentication

* Microsoft OAuth flow (PKCE)
* Xbox Live + XSTS integration
* Secure token storage (platform-specific)
* Multi-account support

### Instance System

* Create, edit and manage multiple instances
* Per-instance configuration
* Mod loader support (extensible)
* Instance importing

### Launch Engine

* Full version manifest parsing
* Argument resolution system
* Library and asset handling
* Custom launch configuration

### Java Management

* Automatic Java detection
* Java version handling
* Built-in provisioning system (download/install Java if missing)

### Modrinth Integration

* Browse mods directly from launcher
* Fetch project/version metadata
* Download and integrate mods into instances

### Network & Downloads

* Custom HTTP client
* Download manager with task system
* Parallel downloads support

### Caching System

* Asset cache
* Library cache
* Central cache manager

### UI (Qt6)

* Instance manager UI
* Console output viewer
* Settings panel
* Authentication dialogs
* Mod browser interface

---

## Project Structure

```
src/
 ├── core/        # Logging, paths, settings, global events
 ├── auth/        # Microsoft OAuth, Xbox Live, token handling
 ├── launch/      # Version parsing, argument resolution, launcher engine
 ├── instance/    # Instance system and configuration
 ├── java/        # Java detection and provisioning
 ├── network/     # HTTP client and download manager
 ├── cache/       # Asset and library caching
 ├── modrinth/    # Modrinth API integration
 └── ui/          # Qt6 interface
```

---

## Build

### Requirements

* C++20 compatible compiler
* Qt6
* CMake

### Build steps

```bash
git clone https://github.com/X4yi/X4Launcher.git
cd X4Launcher
mkdir build && cd build
cmake ..
cmake --build .
```

---

## How It Works (Simplified)

1. User authenticates via Microsoft OAuth
2. Tokens are exchanged (Xbox Live → XSTS → Minecraft)
3. Instance is selected
4. Version manifest is parsed
5. Required assets/libraries are downloaded or loaded from cache
6. Java is resolved (or installed if missing)
7. Launch arguments are generated
8. Game is executed

---

## Design Goals

* Low memory usage
* High control over launch process
* Minimal external dependencies
* Cross-platform compatibility
* Modular architecture

---

## Current State

The project already includes:

* Complete auth pipeline
* Functional launch engine
* UI for instance and settings management
* Modrinth integration

Some parts may still be evolving or incomplete.

---

## Future Improvements

* Better mod loader integration (Forge/Fabric/Quilt)
* Improved error handling and diagnostics
* Performance optimizations in download/cache layers
* UI/UX refinements

---

## License

[GNU GENERAL PUBLIC LICENSE]

---

## Notes

This project is not affiliated with Mojang or Microsoft.
Minecraft is a trademark of Mojang AB.
