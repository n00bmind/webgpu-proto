pushd %~dp0

set backend=WGPU
if "%1" == "dawn" set backend=DAWN

cmake . -B build -DWEBGPU_BACKEND=%backend%
cmake --build build

popd
