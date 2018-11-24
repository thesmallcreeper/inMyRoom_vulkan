rem A small script that build (or rebuild) AMD Anvil project/solution and OpenGLMathimatics(glm) files using cmake

cd Anvil-master
if exist "build" rmdir /s/q build
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..

cd ..
cd ..

cd glm-master
if exist "build" rmdir /s/q build
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
pause