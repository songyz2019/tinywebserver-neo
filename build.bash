export CXX=`which g++`
export CC=`which gcc`
rm -rf build
cmake . -B build
cmake --build build
build/tinywebsrv