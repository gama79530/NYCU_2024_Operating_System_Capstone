import argparse
import os
import serial
import time

kernel_path = './Lab1/cpp/bin/kernel8.img'
port = '/dev/ttyUSB0'
# port = '/dev/pts/5'
baudrate = 115200
communicator = None
serial_timeout = 5  # seconds
exit_msg = 'Kernel uploader is terminated.'
start = False

def exit_handler(exit_code=0):
    global communicator
    if communicator:
        communicator.close()
    print(exit_msg)
    exit(exit_code)

if __name__ == '__main__':
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Kernel Uploader Configuration')
    parser.add_argument('--kernel', type=str, default=kernel_path, help='Path to the kernel image')
    parser.add_argument('--port', type=str, default=port, help='Serial port for communication')
    args = parser.parse_args()

    kernel_path = args.kernel
    port = args.port

    print("This program acts as an uploader for the kernel loader.")
    print('Please review and complete the configuration settings before starting the upload.')

    while not start:
        print()
        print('Current configuration:')
        print(f'1) Kernel path: {kernel_path}')
        print(f'2) Serial port: {port}')
        print("Enter 'y' to start the upload, type the number of the setting to change, or 'exit' to quit:")

        cmd = input().strip().lower()
    
        if cmd == 'y':
            start = True
            # Validate the kernel path and port
            if not os.path.isfile(kernel_path):
                print(f'Error: The kernel image "{kernel_path}" does not exist.')
                start = False
            if not os.path.exists(port):
                print(f'Error: The serial port "{port}" does not exist.')
                start = False
            print()
        elif cmd == '1':
            path = input('Enter the path to the kernel image: ').strip()
            if os.path.isfile(path):
                kernel_path = path
            else:
                print(f'Error: The kernel image "{path}" does not exist.')
        elif cmd == '2':
            path = input('Enter the serial port (e.g., /dev/ttyUSB0): ').strip()
            if os.path.exists(path):
                port = path
            else:
                print(f'Error: The serial port "{path}" does not exist.')
        elif cmd == 'exit':
            exit_handler()
        else:
            print(f'Invalid command "{cmd}". Please try again.')

    communicator = serial.Serial(port=port, baudrate=baudrate, timeout=serial_timeout)
    print("Starting the upload process...")

    # Transmit "#upload_kernel$\\n" command to initiate upload.
    communicator.write(b'#upload_kernel$\n')
    
    # Receive "$download_kernel#\\n" to acknowledge request.
    response = communicator.readline().decode().strip() # ignore command echo
    response = communicator.readline().decode().strip()
    print(f'Received acknowledge from kernel loader: {repr(response)}')
    if response != '$download_kernel#':
        print(f'Error: Unexpected acknowledge from kernel loader: {repr(response)}')
        exit_handler(1)
    
    # # Send 4-byte little-endian integer representing kernel image size.
    kernel_size = os.path.getsize(kernel_path)
    print(f'Uploading kernel of size {kernel_size} bytes...')
    communicator.write(kernel_size.to_bytes(4, byteorder='little'))

    # Receive with "$start_upload#\\n" to signal start of data transfer.
    response = communicator.readline().decode().strip()
    print(f'Received signal from kernel loader: {repr(response)}')
    if response != '$start_upload#':
        print(f'Error: Unexpected signal from kernel loader: {repr(response)}')
        exit_handler(1)

    # Transmit kernel image data.
    print('Transferring kernel image data...')
    with open(kernel_path, 'rb') as f:
        communicator.write(f.read())

    # Return "$done#\\n" indicating transfer completion.
    response = communicator.readline().decode().strip()
    print(f'Received message from kernel loader: {repr(response)}')
    if response != '$done#':
        print(f'Error: Unexpected message from kernel loader: {repr(response)}')
        exit_handler(1)

    print('Kernel upload completed successfully.')
    exit_handler(0)
