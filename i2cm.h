#ifndef I2CM_H
#define I2CM_H

#include <stdint.h>
#include <ctype.h>
#include "ftd2xx.h"
#include "libft4222.h"

#define I2CM_TXBUF_SIZE     256
#define I2CM_RXBUF_SIZE     256

typedef enum e_I2cOpType
{
    //No Operation
    NOP = 0,
    //Standard operation type.
    WRITE = 1,
    READ = 2,
    REG_WRITE = 3,
    REG_READ = 4,
    SCAN = 5,
    POLLACK = 6,

    //Device specific operation type.
    EEPROM_WRITE = 0x10,
    EEPROM_READ = 0x11,
    EEPROM_ERASE = 0x12,
} e_I2cOpType;

typedef struct st_I2cOps
{
    e_I2cOpType Operation;
    uint16_t Channel;
    uint8_t SlaveAddr;
    uint16_t RegAddrLen;
    uint8_t RegAddrBuf[4];
    uint16_t TxLen;
    uint8_t TxBuf[I2CM_TXBUF_SIZE];
    uint16_t RxLen;
    uint8_t RxBuf[I2CM_RXBUF_SIZE];

} st_I2cOps;

int i2cm_detect(int *numOfDev, int LocId[]);
int I2cMaster_creatI2cHandleByLocId(int LocId);

int i2cm_init(int32 kbps);
int i2cm_initByLocId(int32 kbps, int LocId);
int i2cm_scan(void);
int i2cm_StringToArgs(char *string, char *args[]);
int i2cm_praseArgs(st_I2cOps *op, int argc, char *args[]);
int i2cm_runOp(st_I2cOps *op);
#endif /*I2CM_H*/
