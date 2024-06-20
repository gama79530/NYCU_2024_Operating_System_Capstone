import os
import sys
import serial
import re
import select
import termios


kernel_path = './Lab2/kernel/kernel8.img'
port = 'ttyUSB0'
baudrate = 115200
communicator = serial.Serial(baudrate=baudrate)
line_width = 128

def configure_shell():
    global kernel_path
    global port

    print('This is a remote shell. Please finish configuration before starting.')
    arg = input(f'The path of kernel image (default - "{kernel_path}"): ')
    kernel_path = arg if len(arg) else kernel_path
    assert os.path.exists(kernel_path), 'kernel image does not exist.'

    arg = input(f'Port of usb device (default - {port}): ')
    port = arg if len(arg) else port
    communicator.port = port
    

def shell():
    try:
        communicator.open()
        while True:
            if communicator.in_waiting:
                termios.tcflush(sys.stdin, termios.TCIFLUSH)
                response = communicator.read_until().decode(encoding='ascii')
                if re.match(r'^\$[^$]+\$\n'):
                    command = response[1: -2]
                    execute(command)
                else:
                    print(f'\r{response}'.ljust(line_width))
                    
            else:
                rlist, _, _ = select.select([sys.stdin], [], [], 1e-2)
                if rlist:
                    request = input()
                    if 'exit' in request:
                        print('\rbye'.ljust(line_width))
                        sys.exit()

                    communicator.write(request.encode(encoding='ascii'))

    except serial.SerialException as e:
        print(e)
    except KeyboardInterrupt as e:
        print('\nBye!')
    finally:
        communicator.close()
    

def execute(command):
    print(f'\rexecute interactive command "{command}" ...'.ljust(line_width))

    if command == 'upload_kernel':
        upload_kernel()


def upload_kernel():
    kernel_size = os.stat(kernel_path).st_size
    print(f'\rsending kernel size {kernel_size} ...'.ljust(line_width))
    communicator.write(f'{kernel_size}\n'.encode(encoding='ascii'))
    response = communicator.read_until()
    assert '$start_upload\n' == response, 'upload_kernel fail - wrong response 1'
    print(f'\rsending ...'.ljust(line_width))
    with open(kernel_path, 'rb') as f:
        communicator.write(f.read())
    
    response = communicator.read_until()
    assert '$done\n' == response, 'upload_kernel fail - wrong response 2'
    print(f'\rdone'.ljust(line_width))
    

if __name__ == '__main__':
    assert os.name == 'posix', 'Only support Unix-like operating system.'
    configure_shell()
    communicator = serial.Serial(port=port, baudrate=baudrate)
    shell()

    