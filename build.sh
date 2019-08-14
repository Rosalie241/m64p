#!/usr/bin/env bash

UNAME=$(uname -s)
if [[ $UNAME == *"MINGW"* ]]; then
  suffix=".dll"
  if [[ $UNAME == *"MINGW64"* ]]; then
    mingw_prefix="mingw64"
  else
    mingw_prefix="mingw32"
  fi
else
  suffix=".so"
fi


install_dir="$PWD/mupen64plus"
mkdir -p "$install_dir"
base_dir="$PWD"

cd "$base_dir"/mupen64plus-core/projects/unix
make -j$(nproc) all
cp -P "$base_dir"/mupen64plus-core/projects/unix/*$suffix* "$install_dir"
cp "$base_dir"/mupen64plus-core/data/* $install_dir

cd "$base_dir"/mupen64plus-rsp-hle/projects/unix
make -j$(nproc) all
cp "$base_dir"/mupen64plus-rsp-hle/projects/unix/*$suffix "$install_dir"

cd "$base_dir"/mupen64plus-input-sdl/projects/unix
make -j$(nproc) all
cp "$base_dir"/mupen64plus-input-sdl/projects/unix/*$suffix "$install_dir"
cp "$base_dir"/mupen64plus-input-sdl/data/* $install_dir

cd "$base_dir"/mupen64plus-audio-sdl2/projects/unix
make -j$(nproc) all
cp "$base_dir"/mupen64plus-audio-sdl2/projects/unix/*$suffix "$install_dir"

mkdir -p "$base_dir"/mupen64plus-gui/build
cd "$base_dir"/mupen64plus-gui/build

if [[ $UNAME == *"MINGW"* ]]; then
  qmake ../mupen64plus-gui.pro
  make -j$(nproc) release
  cp "$base_dir"/mupen64plus-gui/build/release/mupen64plus-gui.exe "$install_dir"
else
  qmake ../mupen64plus-gui.pro
  make -j$(nproc)
  cp "$base_dir"/mupen64plus-gui/build/mupen64plus-gui "$install_dir"
fi

cd $base_dir/GLideN64/src
./getRevision.sh

mkdir -p "$base_dir"/GLideN64/src/GLideNUI/build
cd "$base_dir"/GLideN64/src/GLideNUI/build
if [[ $UNAME == *"MINGW"* ]]; then
  qmake ../GLideNUI.pro
  make -j$(nproc) release
else
  qmake ../GLideNUI.pro
  make -j$(nproc)
fi

cd "$base_dir"/GLideN64/projects/cmake
sed -i 's/GLideNUI\/build\/debug\/libGLideNUI.a/GLideNUI\/build\/release\/libGLideNUI.a/g' ../../src/CMakeLists.txt
if [[ $UNAME == *"MINGW"* ]]; then
  # Fix compiling error
  sed -i "s|C:\/building\/msys32|C:/msys64|g" /$mingw_prefix/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake
  sed -i 's/check_ipo_supported(RESULT result)//g' ../../src/CMakeLists.txt
  cmake -G "MSYS Makefiles" -DVEC4_OPT=On -DCRC_OPT=On -DMUPENPLUSAPI=On ../../src/
else
  rm -rf ../../src/GLideNHQ/inc
  cmake -DUSE_SYSTEM_LIBS=On -DVEC4_OPT=On -DCRC_OPT=On -DMUPENPLUSAPI=On ../../src/
fi

make -j4

if [[ $UNAME == *"MINGW"* ]]; then
  cp mupen64plus-video-GLideN64$suffix "$install_dir"
else
  cp plugin/Release/mupen64plus-video-GLideN64$suffix "$install_dir"
fi
cp "$base_dir"/GLideN64/ini/GLideN64.custom.ini "$install_dir"

cd "$base_dir"

strip "$install_dir"/*$suffix

if [[ $UNAME == *"MINGW"* ]]; then
  if [[ $UNAME == *"MINGW64"* ]]; then
    my_os=win64
  else
    my_os=win32
  fi
  
  bash ../bundle_dlls.sh "$install_dir/" "$install_dir/mupen64plus-gui.exe" "/$mingw_prefix/bin/"
  bash ../bundle_dlls.sh "$install_dir/" "$install_dir/mupen64plus.dll" "/$mingw_prefix/bin"
  windeployqt.exe "$install_dir/"
  
  cp $base_dir/7za.exe "$install_dir"
else
  if [[ $HOST_CPU == "i686" ]]; then
    my_os=linux32
  else
    my_os=linux64
  fi
fi

rm -rf "$base_dir"/*.zip

zip -r mupen64plus-GLideN64-$my_os.zip mupen64plus
