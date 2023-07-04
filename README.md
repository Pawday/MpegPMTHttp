```sh
sh scripts/cmake/gen_hostcc_release.sh
cmake --build ./BUILD/RELEASE/HOSTCC/
cppcheck --enable=all --project=compile_commands.json
./BUILD/RELEASE/HOSTCC/MpegTsHttp
```
