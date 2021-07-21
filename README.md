# inMyRoom_vulkan
This is an indie Vulkan graphics engine. Supports glTF 2.0 scenes and has both Windows and Linux support (tested only on x64).
Still in early stage, hope you will find something interesting on it.

Named after the mixtape of Greek hiphop artist 013, "In My Room". Project started during some strange times..

![Screenshot_0](https://i.imgur.com/rCjelC7.png)

### Noticeable Features
* 100% homemade collision detection.
* 100% homemade ECS.
* Coded with care and love. Debugged with salt and tears.

### Deps - Submodules
* Anvil , Vulkan library of AMD that has been modified on the user I/O part of it.
* Compressonator , image compression library.
* GLM , math library for OpenGL but can be used with a little hacking for Vulkan.
* configuru , config file library. Got slightly modified but changes got pulled by the original repo ;)
* tinygltf , glTF model import.
* DiTO , algorithm for OBB.

# Installation (Compiling)

  First of all, you need a Vulkan-ready graphics driver, a modern C++ compiler, CMake, Git, Vulkan SDK installed and environment variables `VULKAN_SDK_PATH`/`VULKAN_SDK` pointing to the installation path of the SDK.
  
  On Linux you will also need the X server dev packages.
  * On Ubuntu:
`libxcb-xkb-dev libxcb-keysyms1-dev`
* On Arch Linux :
`xcb-util`


  Having environment set, download the repo and submodules by running the following command.
  ```
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/thesmallcreeper/inMyRoom_vulkan.git
  ```
 ## Windows using Visual Studio 2019

 #### Not maintained! Using CLion thesedays...
 
 * Run `buildForVS2019.bat` script. This will generate project files for Anvil and Compressonator using CMake.
 * Launch solution by opening `inMyRoom_vulkan/inMyRoom_vulkan.sln` and compile. 
 * In order to launch a scene/game which has a `gameDLL` you should go to the game's folder (over at `/inMyRoom_vulkan/testGames/` folder), open the `game_dll` solution and compile.
 * Make sure scene's/game's `gameConfig.cfg`'s variable `gameDLL/path` is pointing to the .dll just created (if one was needed).
 * Make sure `/inMyRoom_vulkan/config.cfg`'s variable `game/path` is pointing to the game's `gameConfig.cfg` you want to launch.
 * Launch `inMyRoom_vulkan.exe`.

 ## Linux/Windows using CMake
 
 * CMake build inside `inMyRoom_vulkan` folder.
 * In order to launch a scene/game which has a `gameDLL` you should go to the game's folder (over at `/inMyRoom_vulkan/testGames/` folder), open the `game_dll` and CMake build it.
 * Make sure scene's/game's `gameConfig.cfg`'s variable `gameDLL/path` is pointing to the .dll or .so just created (if one was needed).
 * Make sure `/inMyRoom_vulkan/config.cfg`'s variable `game/path` is pointing to the game's `gameConfig.cfg` you want to launch.
 * Launch `inMyRoom_vulkan(.exe)`.
 # Notes / Known Issues

 * To run application successfully `/inMyRoom_vulkan/` should be your working folder.
 * If the game use a `gameDLL` both engine and scene should have been compiled with the same compiler and options (Release .exe -> Release .dll , Debug .exe -> Debug .dll)
 * Default resolution is 1600x900. In case your screen is smaller over at `/inMyRoom_vulkan/config.cfg` you can change window resolution (`graphicsSettings/xRes-yRes`)
 * Mipmapping is enabled by default. This means that on the first use of a model mipmaps will be created for it. Mipmapping generator use formats that are not supported from all GPUs. If application crashes then you may need to disable mipmaps over at `/inMyRoom_vulkan/config.cfg` (`graphicsSettings/useMipmaps` true->false ).
 * Default scene/game is SnakeGame over at `/inMyRoom_vulkan/testGames/SnakeGame`. Inside this folder `gameConfig.cfg` exist. Make sure to compile the .dll and have `gameDLL/path_win` config's option point at the correct path.
 
