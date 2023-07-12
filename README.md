# Multisource PMT Player


## Install libraries
```sh
sudo apt install libcurl4-openssl-dev
sudo apt install libjson-c-dev
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
./BUILD/RELEASE/HOSTCC/PMTPlayer

# in separate terminal...

# udp multicas

echo '{"cmd":"play","url":"udp://239.0.0.11:1234"}' >> ./pmt_player_ctl

# http unicast

echo '{"cmd":"play","url":"http://93.189.151.113:8754/udp/234.166.172.91:5555"}' >> ./pmt_player_ctl

```
