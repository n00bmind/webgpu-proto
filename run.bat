pushd %~dp0

set RUST_BACKTRACE=full
build\Debug\App.exe

popd
