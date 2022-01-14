# Data Augmentation

Used for adding indicators to our raw course data.

## Setup

```bash
git clone https://github.com/Research-Project-Crypto/data-augmentation.git --recursive
```

## Build Env

```bash
premake5 gmake2 # linux

# building
make
# release build
make config=release
```

### Install ta-lib from your package manager

```bash
yay -S ta-lib

paru -S ta-lib

```

## Executing the program

```bash
bin/Release/AugmentationCPP data/input/ data/output/
```