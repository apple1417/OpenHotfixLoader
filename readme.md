# OpenHotfixLoader
[![Support Discord](https://img.shields.io/static/v1?label=&message=Support%20Discord&logo=discord&color=424)](https://discord.gg/bXeqV8Ef9R)

OpenHotfixLoader is a simple, bloat-free, hotfix injector for BL3 and Wonderlands modding.

Traditional injectors have worked through creating a proxy, meaning they redirect all your internet
traffic through themselve. OpenHotfixLoader is the first proxyless injector, so there's no risk of
it accidentally leaking your data, or impacting other internet usage.

# Installation
Alternatively, see [this video guide](https://example.com)

1. Open up your game's local files.
2. Navigate to `<game>\OakGame\Binaries\Win64`.
3. Download the [latest plugin loader release](https://github.com/FromDarkHell/BL3DX11Injection/releases/).
   Make sure to download `D3D11.zip`, *not* either of the source code links.
4. Extract the plugin loader zip into this folder, so that you have a file `Win64\d3d11.dll` and a
   folder `Win64\Plugins`.
5. Download the [latest OpenHotfixLoader release](https://github.com/apple1417/OpenHotfixLoader/releases).
   Make sure to grab `OpenHotfixLoader.zip`, *not* either of the source code links.
6. Open up the `Plugins` folder you just extracted, and extract the OpenHotfixLoader zip into, so
   that you have a file `Plugins\OpenHotfixLoader.dll` and a folder `Plugins\ohl-mods`.

At this point, OpenHotfixLoader should be fully installed, and you should see a new news message on
the main menu. If you're still having trouble, try install the latest
[Microsoft Visual C Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Installing Hotfix Mods
To install a hotfix mod, simply download it, and add it to the `ohl-mods` folder you just extracted.
If you edit mods while the game is open, you need to quit to title screen, then load back onto the
main menu in order to reload them.

Mods are loaded in alphabetical order, so if one mod needs to be loaded before another, simply
rename it.


# Notes for Modders
If you ever need to debug the exact hotfixes being applied, launch the game with the
`--dump-hotfixes` command line argument. This will create a `hotfixes.dump` in win64 every time
they're loaded.

# Developing
To get started developing:

1. Clone the repo (including submodules).
   ```
   git clone --recursive https://github.com/bl-sdk/PythonSDK.git
   ```

2. Choose a preset, and run CMake. Most IDEs will have some form of CMake intergration, or you can
   run the commands manually.
   ```
   cmake . --preset msvc-debug
   ```

3. (OPTIONAL) Copy `postbuild.template`, and edit it to copy files to your game install directories.
   Re-run CMake after doing this, existance is only checked during configuration.

4. (OPTIONAL) If you're debugging a game on Steam, add a `steam_appid.txt` in the same folder as the
   executable, containing the game's Steam App Id.

   Normally, games compiled with Steamworks will call
   [`SteamAPI_RestartAppIfNecessary`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_RestartAppIfNecessary),
   which will drop your debugger session when launching the exe directly - adding this file prevents
   that. Not only does this let you debug from entry, it also unlocks some really useful debugger
   features which you can't access from just an attach (i.e. Visual Studio's Edit and Continue).
