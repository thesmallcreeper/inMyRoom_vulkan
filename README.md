# inMyRoom_vulkan
This is an indie Vulkan graphics engine. Supports glTF 2.0 scenes and has both Windows and Linux support (no x86 support, only x64).
Still in early stage, hope you will find something intresting on it.

Feel free to share you opinion on anything, ask, file bugs or suggest enhancement using "Issues" :)

### Deps - Submodules
* Anvil , Vulkan library of AMD that has been modified on user I/O part of it.
* GLM , math library for OpenGL but can be used with a little bit hacking for Vulkan. Also 50MB .pdf removed
* configuru , config file library. Got sligthly modified but changes got pulled by the original repo ;)
* tinygltf , glTF model import.
* MathGeoLib , math library for culling in the future. Not yet used.

# Installation (Compiling)

  First of all, you need a Vulkan-ready graphics driver, a modern C++ compiler, CMake, Vulkan SDK installed and environment variable `VULKAN_SDK_PATH` (Windows),  `VULKAN_SDK` (Linux) pointing to the installation path of SDK.
  
  Also download all the repo and submodules by running the following command.
  ```
git clone --recurse-submodules --shallow-submodules https://github.com/thesmallcreeper/inMyRoom_vulkan.git
  ```
 ## Windows using Visual Studio 2017
 
 * Run `buildForVS2017.bat` script. This will generate project files for Anvil and MathGeoLib using CMake.
 * Launch solution by openning `inMyRoom_vulkan/inMyRoom_vulkan.sln` and compile.
 
 (Well... you can go with CMake way, but due to my lack of CMake knowledge VS solution is better ;) )
 
 ## Windows using CLion with Microsoft Compiler
 
 * Launch CLion, load `inMyRoom_vulkan/CMakeLists.txt` and compile.
 
 ## Linux using CLion with Clang or GCC 7.0
 
 * For window creation and I/O handling XCB is used. Soo... you need these two packages (Ubuntu enviroment)
 ```
 libxcb-xkb-dev
 libxcb-keysyms1-dev
 ```
 * Also you need Clang or GCC 7.0+ setup as your CLion compiler (GCC 8.0+ cannot compile Anvil because it is more restict with using memset/memcpy).
 * Launch CLion ,load `inMyRoom_vulkan/CMakeLists.txt` and compile.
 
 gg ez :D (Actually.. I hope so :P )
 
