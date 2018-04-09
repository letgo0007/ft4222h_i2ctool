echo "I2C command auto test script"
CMD=./fti2c
CHANNEL=0
ADDR=0x50
REG=0x00

echo "===Scan I2C bus==="
$CMD -s $CHANNEL

echo "===Write 1 byte to EERPROM==="
$CMD -v $CHANNEL &ADDR &REG 0x12

echo "===Read 1 byte from EERPROM==="
$CMD -d $CHANNEL &ADDR &REG 1
