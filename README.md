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
 * Launch solution by opening `/inMyRoom_vulkan/inMyRoom_vulkan/inMyRoom_vulkan.sln` and compile.
 * Note: To run application successfully `/inMyRoom_vulkan/inMyRoom_vulkan/` should be your working folder.
 
 (Well... you can go with CMake way, but due to my lack of CMake knowledge VS solution is better ;) )
 
 ## Linux using CMake with Clang or GCC 7.0
 
 * For window creation and I/O handling XCB is used. So for Ubuntu environment you need these packages:
 ```
 $ apt-get install libxcb-xkb-dev
 $ apt-get install libxcb-keysyms1-dev
 ```
 and for Arch Linux this one:
 ```
 $ pacman -S xcb-util
 ```
 * Also you need Clang/Clang++ or GCC/G++ 7 installed (GCC/G++ 8 cannot compile Anvil because it is more restict with using memset/memcpy).
 * Open terminal with `/inMyRoom_vulkan/inMyRoom_vulkan/` as your working folder and pick up your favorite compiler.
 
 If you want to use Clang/Clang++ type:
 ```
 $ export CC=/usr/bin/clang
 $ export CXX=/usr/bin/clang++
 ```
 If you want to use GCC/G++ 7 instead type:
 ```
 $ export CC=/usr/bin/gcc-7
 $ export CXX=/usr/bin/g++-7
 ```
 * Create makefile by typing:
 ```
 $ cmake -G 'Unix Makefiles'
 ```
 * Compile by typing
 ```
 $ make
 ```
 * Launch application by typing
 ```
 $ ./inMyRoom_vulkan
 ```
 
  ## Using CLion (Linux/Windows)
 
 * If you have environment setup accordingly, launch CLion, load `/inMyRoom_vulkan/inMyRoom_vulkan/CMakeLists.txt` and compile.
 
 gg ez :D (Actually.. I hope so :P )
 
