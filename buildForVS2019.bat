rem A small script that build (or rebuild) Anvil, Compressonator, MatheGeoLib project/solution for VS2017

cd Anvil
if exist "build" rmdir /s/q build
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cd ..
cd ..

cd Compressonator
cd Compressonator
if exist "build" rmdir /s/q build
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ../Make
cd ..
cd ..
cd ..

pause
