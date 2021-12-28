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
* VulkanMemoryAllocator-Hpp , the usual VMA library but with Vulkan-Hpp binding.
* GLM , math library for OpenGL and Vulkan.
* configuru , config file library.
* tinygltf , glTF model import.
* eig3 , eigenvector library for PCA for OBBs.

# Installation (Compiling)

  First of all, you need a Vulkan-ready graphics driver, a modern C++ compiler, CMake, Git, Vulkan SDK installed and environment variables `VULKAN_SDK_PATH`/`VULKAN_SDK` pointing to the installation path of the SDK.

#### Using latest git Vulkan header files. For now I personally overwrite Vulkan SDK's include/vulkan files with the latest git ones. The next Vulkan SDK release should do the trick.
  
  On Linux glfw need X11 development packages installed.
  * On Ubuntu you need:
`xorg-dev`
  * On Fedora :
`libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel`

  Having environment set, download the repo and submodules by running the following command.
```
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/thesmallcreeper/inMyRoom_vulkan.git
```
 ## Windows using Visual Studio 2022

 #### Not maintained! Using CLion on Linux thesedays... So some files might be missing from VS project file.  
 
 * Run `buildForVS2022.bat` script. This will generate project files for glfw using CMake.
 * Launch solution by opening `/inMyRoom_vulkan/inMyRoom_vulkan.sln` and compile. 
 * In order to launch a scene/game which has a `gameDLL` you should go to the game's folder (over at `/inMyRoom_vulkan/testGames/` folder), open the `game_dll` solution and compile.
 * Make sure scene's/game's `gameConfig.cfg`'s variable `gameDLL/path` is pointing to the .dll just created (if one was needed).
 * Make sure `/inMyRoom_vulkan/config.cfg`'s variable `game/path` is pointing to the game's `gameConfig.cfg` you want to launch.
 * Launch `inMyRoom_vulkan.exe` with the `/inMyRoom_vulkan` as working folder.

 ## Linux/Windows using CMake
 
 * CMake build inside `/inMyRoom_vulkan` folder.
 * In order to launch a scene/game which has a `gameDLL` you should go to the game's folder (over at `/inMyRoom_vulkan/testGames/` folder), open the `game_dll` and CMake build it.
 * Make sure scene's/game's `gameConfig.cfg`'s variable `gameDLL/path` is pointing to the .dll or .so just created (if one was needed).
 * Make sure `/inMyRoom_vulkan/config.cfg`'s variable `game/path` is pointing to the game's `gameConfig.cfg` you want to launch.
 * Launch `inMyRoom_vulkan(.exe)` with the `/inMyRoom_vulkan` as working folder.

 # Notes / Known Issues

 * If the game use a `gameDLL` both engine and scene should have been compiled with the same compiler and options (Release .exe -> Release .dll , Debug .exe -> Debug .dll).
 * Default resolution is 1600x900. In case your screen is smaller over at `/inMyRoom_vulkan/config.cfg` you can change window resolution (`graphicsSettings/xRes-yRes`)
 * Default scene/game is SnakeGame over at `/inMyRoom_vulkan/testGames/SnakeGame`. Inside this folder `gameConfig.cfg` exist. Make sure to compile the .dll and have `gameDLL/path` config's option point at the correct path.
 
