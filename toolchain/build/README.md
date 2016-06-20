StewieOS - Toolchain Building
=============================

StewieOS comes with a toolchain complete with custom ports of `gcc`, `binutils`, and `newlib`. Most syscalls are implemented, and those that aren't have sane defaults. If you are looking for a guide on building the toolchain, you've come to the right place. Here's the full build script (run from repo root):

```
$ make toolchain
```

Phew, that was rough :laughing:. In reality, this command will take some time, as it builds all of `binutils`, `gcc`, and `newlib` and installs them to `toolchain/root`. Grab a :beer:.
