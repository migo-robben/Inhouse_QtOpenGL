1. 使用 qmake -project 生成.pro 文件
2. 开启 emsdk 的环境，包括mingw
3. qmake 当前工程，生成makefile
4. mingw-make 生成最后的wasm文件