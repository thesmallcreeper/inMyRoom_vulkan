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
  
  Having environment, download all the repo and submodules by running the following command.
  ```
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/thesmallcreeper/inMyRoom_vulkan.git
  ```
 ## Windows using Visual Studio 2019
 
 * Run `buildForVS2019.bat` script. This will generate project files for Anvil and Compressonator using CMake.
 * Launch solution by opening `/inMyRoom_vulkan/inMyRoom_vulkan/inMyRoom_vulkan.sln` and compile. 
 * In order to launch scenes with `gameDLL` you should go to the scene's folder, open the `game_dll` solution and compile.
 * Launch `inMyRoom_vulkan.exe`.
 * Note: To run application successfully `/inMyRoom_vulkan/inMyRoom_vulkan/` should be your working folder.
 * Note: If you use `gameDLL` both engine and scene should have been compiled with the same compiler and options (Release .exe -> Release .dll , Debug .exe -> Debug .dll)
 
 #### This commit has only Windows support. Linux support to come back
 
 gg ez :D (Actually.. I hope so :P )
 
  #### Note: When application loads a scene for first time it is gonna take some time, because mipmaps should be created, compressed and saved/cached for later reuse/relaunch.
