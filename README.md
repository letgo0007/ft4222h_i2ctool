# fti2c
A Mac OSX tool to drive FT4222H USB to I2C bridge.

## Install driver

Use the install script below to install FT4222H driver. Note that this command need to run with sudo.
```
sudo ./install4222.sh
```
After install, please check  `libft4222.dylib` in `/usr/local/lib` and `libft4222.h ftd2xx.h WinTypes.h` in `/usr/local/include`.

## Check some dependency lib
The tool needs some dependency development lib, use Homebrew is the easist way to install.
```
brew install libusb
brew install boost
```

## Compile
```
make all

```

## Usage

The basic syntax of the command is
```
/*       [arg0]      [arg1]      [arg2]      [arg3]      [arg4]
* fti2c  [write]     [slv_addr]  [data0]                             ... [dataN]
* fti2c  [read]      [slv_addr]  [read_len]
* fti2c  [reg_write] [slv_addr]  [reg_addr]  [reg_size]  [data0]     ... [dataN]
* fti2c  [reg_read]  [slv_addr]  [reg_addr]  [reg_size]  [read_len]
* fti2c  [poll_ack]   [slv_addr]
* fti2c  [scan]
*/
```
