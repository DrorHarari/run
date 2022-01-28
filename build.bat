@echo off
pushd %~dp0
if not exist build mkdir build
cd build
echo RUN - Configuring CMake
cmake ..
echo RUN - Building
cmake --build . --config MinSizeRel
copy MinSizeRel\run.exe ..
popd
