# ft4222h_i2ctool
A I2C host tool of FT4222H USB to I2C/SPI/GPIO bridge on Mac OS.

# Install driver
1. Make sure `libft4222.dylib` is in `/usr/local/lib` 
2. Make sure `libft4222.h``ftd2xx.h``WinTypes.h` in `/usr/local/include`.
3. Or there's a simple script to do this.
```
sudo ./install4222.sh
```
Check some dependency lib install on 
```
brew install libusb
brew install boost
```

# Make & Usage
Enter the working directory and run `make all` to compile.
