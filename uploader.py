import os
import argparse
import serial
import time
import re
import select
import sys
import termios

__args = None
__communicator = None


def configure_shell() -> None:
    global __args
    global __communicator

    parser = argparse.ArgumentParser()
    parser.add_argument('kernel_path', help='The path of the kernel image.', type=str)
    parser.add_argument('port', help='The port of usb device.', type=str)
    parser.add_argument('--baudrate', help='Baud rate such as 9600 or 115200 etc.', default=115200, type=int)
    parser.add_argument('--timeout', help='Set a read timeout value in seconds.', default=5, type=int)
    parser.add_argument('--automatic', help='Automatic start booting process.', action='store_false')
    __args = parser.parse_args()

    assert os.path.exists(__args.kernel_path), 'The kernel image does not exist.'
    __communicator = serial.Serial(port=__args.port, baudrate=__args.baudrate, timeout=__args.timeout)


def shell() -> None:
    global __args
    global __communicator
    request = None
    step = 0

    try:
        while True:
            responses = receive()
            if responses:
                for response in responses:
                    response = response.strip('\r')
                    # format "$xxx$" is a command for uploader
                    if re.match(r'^\$[^$]+\$', response):
                        exec(response)
                    # Respberry Pi waiting for next shell command
                    elif response == '$ ':
                        termios.tcflush(sys.stdin, termios.TCIFLUSH)
                        print(f'\r{response}', end='')
                    # response is not the request command
                    elif response and response != request[:-1]:
                        print(response)
            else:
                if __args.automatic:
                    request = 'automatic\n'
                    __communicator.write(request.encode())
                    time.sleep(5e-2)
                    __args.automatic = False
                else:
                    rlist, _, _ = select.select([sys.stdin], [], [], 5e-2)
                    if rlist:
                        request = f'{input()}\n'
                        if 'exit\n' == request:
                            print('Uploader is terminated.')
                            sys.exit()

                        __communicator.write(request.encode())
    except KeyboardInterrupt:
        print('\rUploader is terminated.')
    finally:
        if __communicator is not None:
            __communicator.close()
            __communicator = None


def exec(command: str) -> None:
    if command == '$upload_kernel$':
        upload_kernel()
    else:
        print('Receive an unsupported command.')


def upload_kernel() -> None:
    global __args
    global __communicator

    # protocol 3: uploader receives "$upload_kernel$\n" and replies with a "4-byte data" that indicates the size of the kernel image in little-endian format.
    kernel_size = os.stat(__args.kernel_path).st_size
    print(f'kernel size: {kernel_size} bytes ...')
    __communicator.write(kernel_size.to_bytes(4, 'little'))

    # protocol 5: uploader receives "start_upload$" and uploads the kernel image.
    response = __communicator.read_until(expected=b'$').decode(encoding='ascii')
    assert response == 'start_upload$', 'upload_kernel fail: protocol 5'
    print(f'sending kernel image ...')
    with open(__args.kernel_path, 'rb') as f:
        __communicator.write(f.read())

    # protocol 7: uploader receives "done$" and ends the process.
    response = __communicator.read_until(expected=b'$').decode(encoding='ascii')
    assert response == 'done$', 'upload_kernel fail: protocol 7'


def receive() -> list[str] | None:
    global __communicator
    
    if not __communicator.in_waiting:
        return None
    
    responses_bytes = b''
    while __communicator.in_waiting:
        responses_bytes += __communicator.read(__communicator.in_waiting)
        time.sleep(5e-2)

    responses = responses_bytes.decode().split(sep='\n')
    return responses


if __name__ == '__main__':
    assert os.name == 'posix', 'Only support Unix-like operating system.'
    configure_shell()
    shell()

