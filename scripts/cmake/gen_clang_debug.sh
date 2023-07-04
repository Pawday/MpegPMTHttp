rm -f ./compile_commands.json
cmake -B BUILD/DEBUG/CLANG -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER=clang
ln -s BUILD/DEBUG/CLANG/compile_commands.json compile_commands.json
