# KCSH
Reinventing the penguin-colored wheel: **k**eli5's **c**++ **sh**ell for linux

## Building
### Debug builds
Makefile includes `debug` (builds all debug targets) as well as `(arch)-debug` (for supported architectures.) Just use `make debug` or `make x86_64-debug`/your preferred architecture to build -g3 executables, useful for GDB and etc.
### Release builds
Makefile also includes `release` (builds all release targets.) Release is default, so just `make (arch)` for supported architectures will build a fully optimized release executable.

## Contributing
Feature requests, issues, and pull requests are always welcome!
