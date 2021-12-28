rem A small script that build glfw

cd glfw
if exist "build" rmdir /s/q build -D BUILD_SHARED_LIBS=ON
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
cd ..
cd ..

pause
