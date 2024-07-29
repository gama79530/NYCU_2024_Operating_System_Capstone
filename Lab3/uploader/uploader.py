import os
import serial
import time
import re
import select
import sys
import termios

__kernel_path = './Lab2/kernel/kernel8.img'
__port = '/dev/ttyUSB0'
__baudrate = 115200
__communicator = None
__timeout = 5


def configure_shell() -> None:
    global __kernel_path
    global __port

    print('This is an uploader shell. Please complete the configuration before starting.')
    arg = input(f'The path of the kernel image (default: "{__kernel_path}"): ')
    __kernel_path = arg if len(arg) else __kernel_path
    assert os.path.exists(__kernel_path), 'The kernel image does not exist.'

    arg = input(f'The port of usb device (default: "{__port}"): ')
    __port = arg if len(arg) else __port

    print('')


def shell() -> None:
    global __port
    global __baudrate
    global __communicator
    global __timeout
    request = None

    try:
        __communicator = serial.Serial(port=__port, baudrate=__baudrate, timeout=__timeout)
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
                rlist, _, _ = select.select([sys.stdin], [], [], 5e-2)
                if rlist:
                    request = f'{input()}\n'
                    if 'exit\n' == request:
                        print('Uploader is terminated.')
                        sys.exit()

                    __communicator.write(request.encode())

    except serial.SerialException as e:
        print(e)
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
    global __kernel_path
    global __communicator

    # protocol 3: uploader receives "$upload_kernel$\n" and replies with a "4-byte data" that indicates the size of the kernel image in little-endian format.
    kernel_size = os.stat(__kernel_path).st_size
    print(f'kernel size: {kernel_size} bytes ...')
    __communicator.write(kernel_size.to_bytes(4, 'little'))

    # protocol 5: uploader receives "start_upload$" and uploads the kernel image.
    response = __communicator.read_until(expected=b'$').decode(encoding='ascii')
    assert response == 'start_upload$', 'upload_kernel fail: protocol 5'
    print(f'sending kernel image ...')
    with open(__kernel_path, 'rb') as f:
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