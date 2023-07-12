rm -f ./compile_commands.json
cmake -B BUILD/DEBUG/HOSTCC -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ln -s BUILD/DEBUG/HOSTCC/compile_commands.json compile_commands.json
