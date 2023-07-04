# CURL MpegTs PMT dumper


## Install curl
```sh
sudo apt install libcurl4-openssl-dev
```

## Configure
```sh
sh scripts/cmake/gen_hostcc_release.sh
```

## cppcheck
```sh
cppcheck --enable=all --project=compile_commands.json
```
## Build
```sh
cmake --build ./BUILD/RELEASE/HOSTCC/
```

## Usage
```sh
./BUILD/RELEASE/HOSTCC/MpegTsHttp
```
