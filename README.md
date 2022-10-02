# OpenHotfixLoader
[![Support Discord](https://img.shields.io/static/v1?label=&message=Support%20Discord&logo=discord&color=424)](https://discord.gg/bXeqV8Ef9R)

OpenHotfixLoader is a simple, bloat-free, hotfix injector for BL3 and Wonderlands modding.

Traditional injectors have worked through creating a proxy, meaning they redirect all your internet
traffic through themselve. OpenHotfixLoader is the first proxyless injector, so there's no risk of
it accidentally leaking your data, or impacting other internet usage.

# Installation
Alternatively, see [this video guide](https://youtu.be/gHX3dtZIojY)

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
If you want to automatically download the latest version, simply create a URL shortcut and put it in
this folder - just drag the download link out of your browser into Windows Explorer.

If you edit mods while the game is open, you need to quit to title screen, then load back onto the
main menu in order to reload them.

Mods are loaded in numeric-alphabetical order - `A.txt` loads before `B.txt`, and `9.txt` before
`10.txt`. This should be the same order which Windows Explorer displays when you sort by name. If
one mod needs to be loaded before another, simply rename it so sorts later.

# Notes for Modders
## Commands
OpenHotfixLoader supports a number of commands. All commands strip leading whitespace, and are
detected case insensitively (though this doesn't mean case doesn't matter when they're intepreted).

### Hotfixes
OpenHotfixLoader follows the standard hotfix format.

```
SparkPatchEntry,(1,1,0,),/Game/PlayerCharacters/_Shared/_Design/Sliding/ControlledMove_Global_Sliding.Default__ControlledMove_Global_Sliding_C,Duration.BaseValueConstant,0,,5000
```

Any line starting with `Spark` is considered to be a hotfix. Everything before the first comma is
taken as the hotfix key, and will have an index automatically appended. Everything after the first
comma is taken as the value.

Type 11 hotfixes are automatically detected, moved to the front of the hotfix list, and have their
delay hotfixes auto generated.

### Inject News Item
You can inject custom news items using `InjectNewsItem` commands.

```
InjectNewsItem,Header,https://url.to/image.png,https://url.to/article,News body, not visible in BL3
```

These commands consist of five comma seperated fields: the command, the header, the image url, the
article url, and the body. You only have to specify the fields you're using, no need for trailing
commas if you only have a header. If you want to include a comma in the header or urls, use csv
escaping - quote it, and use double quotes to insert a literal quote. The body text is always taken
literally (if it exists), no need to escape it.

```
InjectNewsItem,"Header, which contains a comma and a pair of ""quotes""",https://url.to/image.png
```

### Exec
You can execute another mod file using the `exec` command. This acts as if all the contents of that
file were inserted directly into yours at the command's location. Note that a file can only be
included once, whether that's from an `exec` command, or simply from being in `ohl-mods`, it will
just get loaded the first time it's referenced.

This command mirrors the format of the actual UE console command - there must be a whitespace
seperator, and you must quote paths including whitespace.

```
exec my_mod.bl3hotfix
exec "D:\My Mods\testing_mod.txt"
```

Paths are taken relative to the `ohl-mods` folder (unless they're absolute to begin with).

Note that, as a security precaution, exec commands *cannot* be run from files downloaded from a url
(see below). They are simply ignored.

### URL
You can download and execute a mod from a url using the `URL=` command. This has all the same
semantics as executing a local file, as discussed above.

```
URL=https://url.to/mod.file
```
All content after the `=` is considered part of the url. This syntax is chosen to support Windows'
URL shortcut files.

## Misc Notes
If you ever need to debug the exact hotfixes being applied, launch the game with the
`--dump-hotfixes` command line argument. This will create a `hotfixes.dump` in win64 every time
they're loaded.

If you launch the game with the `--ohl-debug` command line argument, OpenHotfixLoader will print
some more detailed logs messages.

While not strictly part of OpenHotfixLoader, launching with the `--debug` command line argument will
cause pluginloader to generate an external console window. OpenHotfixLoader's log messages will also
appear here.

Mod files are expected to be utf8 encoded.

# Developing
To get started developing:

1. Clone the repo (including submodules).
   ```
   git clone --recursive https://github.com/apple1417/OpenHotfixLoader/
   ```

2. Choose a preset, and run CMake. Most IDEs will have some form of CMake intergration, or you can
   run the commands manually.
   ```
   cmake --preset msvc-debug .
   cmake --build out/build/msvc-debug
   ```

   Cross compilation on Linux is supported through the `mingw-debug` and `mingw-release` presets.

3. (OPTIONAL) Copy `postbuild.template`, and edit it to copy files to your game install directories.
   Re-run CMake after doing this, existence is only checked during configuration.

4. (OPTIONAL) Copy `user-includes.cmake.template`, and edit it to customize the CMake includes.
   One notable use of this is to make sure libcurl gets properly built with zlib (though the code
   will work without).

   As before, re-run CMake after doing this, as existence is only checked during configuration.

5. (OPTIONAL) If you're debugging a game on Steam, add a `steam_appid.txt` in the same folder as the
   executable, containing the game's Steam App Id.

   Normally, games compiled with Steamworks will call
   [`SteamAPI_RestartAppIfNecessary`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_RestartAppIfNecessary),
   which will drop your debugger session when launching the exe directly - adding this file prevents
   that. Not only does this let you debug from entry, it also unlocks some really useful debugger
   features which you can't access from just an attach (i.e. Visual Studio's Edit and Continue).
