## QtOpenGL

# Build

```bash
cd Inhouse_QtOpenGL
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=[path]\Qt\5.12.6\msvc2017_64 ..
cmake --build . --config Release
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