#ifndef I2CM_H
#define I2CM_H

#include <stdint.h>
#include <ctype.h>
#include "ftd2xx.h"
#include "libft4222.h"

typedef enum e_I2cOpType
{
    //Standard operation type.
    POLLACK = 0,
    WRITE   = 1,
    READ    = 2,
    REG_WRITE  = 3,
    REG_READ   = 4,
    SCAN    = 5,

    //Device specific operation type.
    EEPROM_WRITE    = 0x10,
    EEPROM_READ     = 0x11,
    EEPROM_ERASE    = 0x12,
}e_I2cOpType;


typedef struct st_I2cOps
{
    e_I2cOpType     Operation;
    uint16_t        Channel;
    uint8_t         SlaveAdd;
    uint8_t         *WritePtr;
    uint16_t        WriteLen;
    uint8_t         *ReadPtr;
    uint16_t        ReadLen;
}st_I2cOps;

int I2cMaster_detectI2cInterface(int *numOfDev, int *LocId);
int I2cMaster_creatI2cHandleByLocId(int LocId );
int I2cMaster_write(st_I2cOps *op);
int I2cMaster_read(st_I2cOps *op);
int I2cMaster_wwrite(st_I2cOps *op);
int I2cMaster_wread(st_I2cOps *op);

int i2cm_init();
int i2cm_processCommand(int argc, char *args[]);

#endif /*I2CM_H*/
