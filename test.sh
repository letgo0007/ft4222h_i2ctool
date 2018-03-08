echo "Test fti2c"
./fti2c write 0xA0 1 2 3 4 5
./fti2c read 0xA0 5
./fti2c reg_write 0xA0 0x12345678 4 1 2 3 4 5
./fti2c reg_read 0xA0, 0x12345678 4 5
