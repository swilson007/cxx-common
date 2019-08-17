thisdir=$PWD

mkdir -p build/macos && cd build/macos
conan install ../.. --build=missing

cd $thisdir
mkdir -p build/macosd && cd build/macosd
conan install ../..
