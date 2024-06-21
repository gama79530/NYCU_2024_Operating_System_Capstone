import os
import sys
import serial
import re
import select
import termios
import time


kernel_path = './Lab2/kernel/kernel8.img'
port = '/dev/ttyUSB0'
baudrate = 115200
communicator = serial.Serial(port=None, baudrate=baudrate)
buffer = ''

def configure_shell():
    global kernel_path
    global port
    global communicator
    global buffer

    print('This is a remote shell. Please finish configuration before starting.')
    arg = input(f'The path of kernel image (default - "{kernel_path}"): ')
    kernel_path = arg if len(arg) else kernel_path
    # assert os.path.exists(kernel_path), 'kernel image does not exist.'

    arg = input(f'Port of usb device (default - {port}): ')
    port = arg if len(arg) else port
    communicator.port = port
    print('')


def receive() -> list[str] | None:
    if not communicator.in_waiting:
        return None
    
    responses_bytes = b''
    while communicator.in_waiting:
        responses_bytes += communicator.read(communicator.in_waiting)
        time.sleep(5e-2)

    responses = responses_bytes.decode().split(sep='\n')

    return responses


def shell():
    global communicator

    try:
        communicator.open()
        while True:
            responses = receive()
            if responses:
                for response in responses:
                    response = response.rstrip('\r')
                    if response == '# ':
                        termios.tcflush(sys.stdin, termios.TCIFLUSH)
                        print(f'\r{response}', end='')
                    elif re.match(r'^\$[^$]+\$', response):
                        execute(response[1:-1])
                    elif response != request.rstrip('\n'):
                        print(response)
            else:
                rlist, _, _ = select.select([sys.stdin], [], [], 5e-2)
                if rlist:
                    request = f'{input()}\n'
                    if 'exit' in request:
                        print('\rbye')
                        sys.exit()

                    communicator.write(request.encode())
    except serial.SerialException as e:
        print(e)
    except KeyboardInterrupt as e:
        print('\nBye!')
    finally:
        if communicator.is_open:
            communicator.close() 

def execute(command):
    print(f'\rexecute interactive command "{command}" ...')

    if command == 'upload_kernel':
        upload_kernel()


def upload_kernel():
    global communicator

    kernel_size = os.stat(kernel_path).st_size
    print(f'\rsending kernel size {kernel_size} ...')
    communicator.write(f'{kernel_size}\n'.encode())
    response = communicator.read_until()
    assert '$start_upload\n' == response, 'upload_kernel fail - wrong response 1'
    print(f'\rsending ...')
    with open(kernel_path, 'rb') as f:
        communicator.write(f.read())
    
    response = communicator.read_until()
    assert '$done\n' == response, 'upload_kernel fail - wrong response 2'
    print(f'\rdone')
    

if __name__ == '__main__':
    assert os.name == 'posix', 'Only support Unix-like operating system.'
    configure_shell()
    shell()

    