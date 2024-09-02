# Recaps

## Description

You can find more explanation from [here](https://gama79530.github.io/3_Course/OperatingSystemCapstone/Lab2/index.html).

### bootloader

```bash
# compile
make

# run on qemu isolatedly
make run

# run on qemu, in conjunction with uploader
make pty

# debug
make debug-qemu
make debug-gdb
```

### Kernel

```bash
# compile
make

# run on qemu
make run

# debug
make debug-qemu
make debug-gdb

# execute kernel on rpi3
make run-on-board
```

## kernel uploading protocol

1. `uploader` sends a command `"download\n"` to `loader`.
2. `loader` initiates the download process by sending a request string `"$upload_kernel$\n"` to `uploader`.
3. `uploader` receives `"$upload_kernel$\n"` and replies with a `"4-byte data"` that indicates the size of the kernel image in little-endian format.
4. `loader` receives the kernel size and replies with `"start_upload$"`.
5. `uploader` receives `"start_upload$"` and uploads the kernel image.
6. `loader` receives the kernel image and replies with `"done$"`.
7. `uploader` receives `"done$"` and ends the process.
