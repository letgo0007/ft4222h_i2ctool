# fti2c
A Mac OSX tool to drive FT4222H USB to I2C bridge.

## Install driver
Use the install script below to install FT4222H driver. Note that this command need to run with sudo.
```
cd ./install
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
make all | debug | clean

```

## Usage
The basic syntax of the command is as below, handled by [https://github.com/letgo0007/cli](https://github.com/letgo0007/cli)
```
Basic Parameters:
    -r   --read      :[Bus] [Addr] [Length] Read raw data
    -d   --devread   :[Bus] [Addr] [Reg] [Len] Read register data
    -w   --write     :[Bus] [Addr] [Data] Write raw data
    -v   --devwrite  :[Bus] [Addr] [Reg] [Data] Write register data
    -m   --maskwrite :[Bus] [Addr] [Reg] [Mask] [Data]Write register data with mask
    -s   --sweep     :[Bus] Sweep I2C bus for devices
    -l   --list      :List I2c bus available
Optional Parameters:
    -f   --freq      :[Freq] Set I2c frequency in kHz. Default is 100.
    -z   --addrsize  :[Size] Register address size in bytes. Default is 1.
    -h   --help      :Show help hints
```

## Examples
```shell
## List available FT4222H I2C bus.
./fti2c -l
I2C Master Bus Count = [1]
-----------------
Dev Index [0]:
  Flags         =0x2
  Type          =0xa
  ID            =0x403601c
  LocId         =0x14191
  SerialNumber  =
  Description   =FT4222 A
  ftHandle      =0x0
```
```shell
## Scan I2C device on I2C bus 0.
./fti2c -s 0
I2C slave sweep on bus [0]
I2C slave detected: 0x55
```
```shell
## Write/Read an EEPROM address @ 0x50
./fti2c -v 0 0x50 0x01 0x12
I2C REG_WRITE, REG=[0x01], count=[1]
0x12
./fti2c -d 0 0x50 0x01 1
I2C REG_READ, REG=[0x01], count=[1]
0x12
```
