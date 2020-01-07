# inMyRoom_vulkan
This is an indie Vulkan graphics engine. Supports glTF 2.0 scenes and has both Windows and Linux support (no x86 support, only x64).
Still in early stage, hope you will find something intresting on it.

#### This commit has only Windows support. Linux support to come back

Feel free to share you opinion on anything, ask, file bugs or suggest enhancement using "Issues" :)

### Deps - Submodules
* Anvil , Vulkan library of AMD that has been modified on the user I/O part of it.
* Compressonator , image compression library.
* GLM , math library for OpenGL but can be used with a little bit hacking for Vulkan. Also 50MB .pdf removed
* configuru , config file library. Got sligthly modified but changes got pulled by the original repo ;)
* tinygltf , glTF model import.
* DiTO , aglorithm for OBB.

# Installation (Compiling)

  First of all, you need a Vulkan-ready graphics driver, a modern C++ compiler, CMake, Git, Vulkan SDK installed (1.1.114.0 preferred) and environment variable `VULKAN_SDK_PATH` (Windows),  `VULKAN_SDK` (Linux) pointing to the installation path of the SDK.
   #### Note: Vulkan SDKs greater than 1.1.114.0 fail to compile
  
  Having environment, download all the repo and submodules by running the following command.
  ```
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/thesmallcreeper/inMyRoom_vulkan.git
  ```
 ## Windows using Visual Studio 2019
 
 * Run `buildForVS2019.bat` script. This will generate project files for Anvil and Compressonator using CMake.
 * Launch solution by opening `inMyRoom_vulkan/inMyRoom_vulkan.sln` and compile. 
 * In order to launch scenes/game with `gameDLL` you should go to the game's (over at `/inMyRoom_vulkan/testGames/` folder), open the `game_dll` solution and compile.
 * Launch `inMyRoom_vulkan.exe`.
 * Note: To run application successfully `/inMyRoom_vulkan/` should be your working folder.
 * Note: If you use `gameDLL` both engine and scene should have been compiled with the same compiler and options (Release .exe -> Release .dll , Debug .exe -> Debug .dll)
 
 #### This commit has only Windows support. Linux support to come back
 
 # Notes / Known Issues
 
 * Default resolution is 1600x900. In case your screen is smaller over at `/inMyRoom_vulkan/config.cfg` you can change window resolution (`graphicsSettings/xRes-yRes`)
 * Mipmapping is enabled by default. This means that on the first use of a model mipmaps will be created for it. Mipmapping generator use formats that are not supported from all GPUs. If application crashes then you may need to disable mipmaps over at `/inMyRoom_vulkan/config.cfg` (`graphicsSettings/useMipmaps` true->false ).
 * Default scene/game is SnakeGame over at `/inMyRoom_vulkan/testGames/SnakeGame`. Inside this folder `gameConfig.cfg` exist. Make sure to compile the .dll and have `gameDLL/path_win` config's option point at the correct path.
 
