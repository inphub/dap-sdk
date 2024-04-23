#!/bin/bash
# Определяем тип сборкми, - нативная или кроссплатформенная?
BUILD_TYPE_DEFAULT=native
BUILD_TYPE=$BUILD_TYPE_DEFAULT
INSTALL_PREFIX=/opt/dap-sdk
if [ -f ".build_type" ]; then
    BUILD_TYPE=`cat .build_type`
fi

if [ ! -z "$1" ]; then
    if [[ "$1" == "cross" || "$1" == "native" ]]; then
	BUILD_TYPE="$1"
    else
	echo "В качестве аргумента указан неправильный тип сборки \"$1\" , может быть только cross (кроссплатформенная) или native (нативная)"
	exit -1
    fi
fi
echo "Building on $BUILD_TYPE build toolchain"

CMAKE_OPS="-DCMAKE_PREFIX=$INSTALL_PREFIX"

# Выставляем значения параметров для нативной сборки
if [[ "$BUILD_TYPE" == "native" ]]; then
    CMAKE_CMD=cmake
# Выставляем значения параметров для кроссплатформенной сборки
elif [[ "$BUILD_TYPE" == "cross" ]]; then
    CMAKE_CMD=debian_12_cmake.sh
    CMAKE_OPS="$CMAKE_OPS -DCMAKE_TOOLCHAIN_FILE=share/cmake/Toolchain_host_Linux_target_armel.cmake"
# В теории мы сюда никогда не попадём, но мало ли
else
	echo "Неправильный тип сборки \"$BUILD_TYPE\", может быть только cross (кроссплатформенная) или native (нативная)"
	exit -100
fi

# Инициируем отладочную сборку
mkdir -p build.debug
cd build.debug
rm -rf *
$CMAKE_CMD $CMAKE_OPS -DCMAKE_BUILD_TYPE=Debug ../
cd ..

# Инициируем релизную сборку

mkdir -p build.release
cd build.release
rm -rf *
$CMAKE_CMD $CMAKE_OPS ../
cd ..

echo $BUILD_TYPE > .build_type