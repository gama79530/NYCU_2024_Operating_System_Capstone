#!/usr/bin/env python3
import argparse
import os
import struct
import sys
import time

import serial


BOOT_MAGIC: bytes = b"KERN"
DEFAULT_KERNEL: str = "./Lab2/c/bin/kernel8.img"
DEFAULT_PORT: str = "/dev/ttyUSB0"
DEFAULT_BAUD: int = 115200
PROMPT: bytes = b"(bootloader)$ "


def read_until(communicator: serial.Serial, marker: bytes, timeout: float) -> bytes:
    deadline = time.monotonic() + timeout
    data = bytearray()

    while marker not in data:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError(f"timed out waiting for {marker!r}; got {bytes(data)!r}")

        communicator.timeout = remaining
        chunk = communicator.read(1024)
        if chunk:
            data.extend(chunk)

    return bytes(data)


def confirm_configuration(args: argparse.Namespace) -> None:
    while True:
        print()
        print("Current configuration:")
        print(f"1) Kernel path: {args.kernel}")
        print(f"2) Serial port: {args.port}")
        print(f"3) Baud rate: {args.baud}")
        print(f"4) Auto boot: {'no' if args.no_boot else 'yes'}")
        print("Enter 'y' to start, setting number to change, or 'exit' to quit:")

        command = input("> ").strip().lower()
        if command == "y":
            return
        if command == "exit":
            raise SystemExit(0)
        if command == "1":
            args.kernel = input("Kernel path: ").strip()
        elif command == "2":
            args.port = input("Serial port: ").strip()
        elif command == "3":
            args.baud = int(input("Baud rate: ").strip())
        elif command == "4":
            args.no_boot = not args.no_boot
        else:
            print(f"Unknown command: {command}")


def validate_configuration(args: argparse.Namespace) -> None:
    if not os.path.isfile(args.kernel):
        raise FileNotFoundError(f"kernel image does not exist: {args.kernel}")
    if not os.path.exists(args.port):
        raise FileNotFoundError(f"serial port does not exist: {args.port}")


def upload_kernel(args: argparse.Namespace) -> None:
    with open(args.kernel, "rb") as kernel_file:
        kernel = kernel_file.read()

    with serial.Serial(port=args.port, baudrate=args.baud, timeout=args.timeout) as communicator:
        print("Waiting for bootloader prompt...")
        communicator.write(b"\n")
        communicator.flush()
        read_until(communicator, PROMPT, args.timeout)

        print("Entering upload mode...")
        communicator.write(b"upload\n")
        communicator.flush()
        read_until(communicator, b"$ready#\r\n", args.timeout)

        print(f"Uploading {len(kernel)} bytes from {args.kernel}...")
        communicator.write(BOOT_MAGIC)
        communicator.write(struct.pack("<I", len(kernel)))
        communicator.flush()
        read_until(communicator, b"$start#\r\n", args.timeout)
        communicator.write(kernel)
        communicator.flush()
        read_until(communicator, b"$done#\r\n", args.timeout)

        print("Upload completed.")
        if not args.no_boot:
            print("Booting uploaded kernel...")
            communicator.write(b"boot\n")
            communicator.flush()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Upload a kernel image to the UART bootloader.")
    parser.add_argument("--kernel", default=DEFAULT_KERNEL, help="path to kernel image")
    parser.add_argument("--port", default=DEFAULT_PORT, help="serial device path")
    parser.add_argument("--baud", default=DEFAULT_BAUD, type=int, help="serial baud rate")
    parser.add_argument("--timeout", default=10.0, type=float, help="handshake timeout in seconds")
    parser.add_argument("--no-boot", action="store_true", help="upload only; do not send boot command")
    parser.add_argument("-y", "--yes", action="store_true", help="skip interactive confirmation")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not args.yes:
        confirm_configuration(args)

    validate_configuration(args)
    upload_kernel(args)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Interrupted.", file=sys.stderr)
        raise SystemExit(130)
    except Exception as exc:
        print(f"KernelUploader.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
