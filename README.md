[![Tests](https://github.com/p-sawicki/tin/actions/workflows/tests.yml/badge.svg)](https://github.com/p-sawicki/tin/actions/workflows/tests.yml)

# Configuration

## Download
To download the repository with all submodules use:

```
git clone https://github.com/p-sawicki/tin.git --recursive
```

## Requirements
```
sudo apt-get update
sudo apt-get install libssl-dev cmake
pip3 install argparse ipcqueue matplotlib
```

## Build configuration
```
cd tin
cmake -B build
```

## Build
```
cmake --build build -j10
```

## Run
```
mkdir logs
build/main_app/run_scenarios --help
```