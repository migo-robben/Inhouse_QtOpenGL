## QtOpenGL

# Build

```bash
cd Inhouse_QtOpenGL
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=[path]\Qt\5.12.6\msvc2017_64 ..
cmake --build . --config Release
```

```bash
-DCMAKE_PREFIX_PATH=G:\Developer\Qt\5.12.6\msvc2017_64\lib\cmake;C:\DEV_PROJECT\dev-repository\vcpkg\installed\x64-windows\share\spdlog;C:\DEV_PROJECT\dev-repository\vcpkg\installed\x64-windows\share\fmt
-DSTB_ROOT=C:\DEV_PROJECT\dev-repository\stb
-DCMAKE_TOOLCHAIN_FILE_=C:\DEV_PROJECT\dev-repository\vcpkg\scripts\buildsystems\vcpkg.cmake
-DEIGEN_ROOT=C:\DEV_PROJECT\dev-repository\vcpkg\packages\eigen3_x64-windows
-G "Visual Studio 16 2019"
-DUSD_HOME=G:/DEV_PROJECT/dev-library/usd-dev-win64
-DASSIMP_ROOT=C:\DEV_PROJECT\dev-repository\assimp\bin
-DCMAKE_BUILD_TYPE=Debug
```

# Run

## Visual Studio

<div align=center>
<img src="https://github.com/winsingaaron/Inhouse_QtOpenGL/blob/master/resource/images/set_up_in_vs.jpg?raw=true" width="600">
</div>

## Command line
```bash
set QT_PLUGIN_PATH=[path]\Qt\5.12.6\msvc2017_64\plugins
```

## Clion

<div align=center>
<img src="https://github.com/winsingaaron/Inhouse_QtOpenGL/blob/master/resource/images/run_clion.jpg?raw=true" width="600">
</div>
