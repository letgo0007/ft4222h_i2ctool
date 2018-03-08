#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "ftd2xx.h"
#include "libft4222.h"
#include "i2cm.h"

//Call a function and check if return value == 0.
//  1. Give a simple error message function/file/line.
//  2. return with error.
#define CHECK_FUNC_RET(ret, func) \
    do {\
        ret = func;\
        if (0 != ret)\
        {\
            printf("\033[31mERROR: Return=[%d] "#func"<%s:%d>\033[0m\n", ret, __FILE__, __LINE__);\
            return ret;\
        }\
    } while (0)

//Call a function and check if return value == 0.
//  1. Give a simple error message function/file/line.
//  2. goto exit label. The function must have a exit lable.
#define CHECK_FUNC_EXIT(ret, func) \
    do {\
        ret = func;\
        if (0 != ret)\
        {\
            printf("\033[31mERROR: Return=[%d] "#func"<%s:%d>\033[0m\n", ret, __FILE__, __LINE__);\
            goto exit;\
        }\
    } while (0)

static FT_STATUS gFtStatus = 0;
static FT_HANDLE gFtHandle = NULL;

int i2cm_write(st_I2cOps *op);
int i2cm_read(st_I2cOps *op);
int i2cm_reg_write(st_I2cOps *op);
int i2cm_reg_read(st_I2cOps *op);
int i2cm_pollack(st_I2cOps *op);
int i2cm_scan(st_I2cOps *op);

//@brief Scan connected FT4222H deveice, return number of device & the USB location.
int I2cMaster_detectI2cInterface(int *numOfDev, int *locationid)
{
    FT_STATUS ftStatus;
    FT_DEVICE_LIST_INFO_NODE *ftDevInfo = NULL;
    DWORD ftDevAmount = 0;
    int ftI2cIfAmount = 0;
    int retCode = 0;

    //Step1 : Check number of FT4222 connected.
    CHECK_FUNC_EXIT(ftStatus, FT_CreateDeviceInfoList(&ftDevAmount));

    if (ftDevAmount == 0)
    {
        printf("No devices connected.\n");
        retCode = -20;
        //goto exit;
    }

    // Allocate storage for device info according to device amount.
    ftDevInfo = calloc((size_t) ftDevAmount, sizeof(FT_DEVICE_LIST_INFO_NODE));
    if (ftDevInfo == NULL)
    {
        printf("Allocation failure.\n");
        retCode = -30;
        goto exit;
    }

    //Step2: Get FT422H device info.
    CHECK_FUNC_EXIT(ftStatus, FT_GetDeviceInfoList(ftDevInfo, &ftDevAmount));

    //Step3: Check device info list and return those are configured in I2C mode.
    /* FT4222H has 4 mode, seleted by {DCNF1, DCNF0} pin.
     * Mode[0] & Mode[1] are avialable for I2C operation.
     *  Mode[0] : 1x SPI M/S + I2C + GPIO
     *  Mode[1] : 3x SPI M + GPIO
     *  Mode[2] : 4x SPI M
     *  Mode[3] : 1x SPI M/S + I2C
     */
    printf("[%d]FT4222H device is found:\n", ftDevAmount);
    printf("[I2C Index]\t[MODE]\t[LocationId]\t[SerialNumber]\t[Description]\n");
    int i = 0;
    for (i = 0; i < (int) ftDevAmount; i++)
    {
        // Mode0 & Mode3 are avaiable I2C interface.
        if ((ftDevInfo[i].Type == FT_DEVICE_4222H_0) || (ftDevInfo[i].Type == FT_DEVICE_4222H_3))
        {
            printf("%10d\t%4d\t%10x\t%10s\t%s\n", ftI2cIfAmount, ftDevInfo[i].Type, ftDevInfo[i].LocId,
                    ftDevInfo[i].SerialNumber, ftDevInfo[i].Description);
            ftI2cIfAmount++;
        }

        // Mode1 & Mode2 are not I2C mode.
        if (ftDevInfo[i].Type == FT_DEVICE_4222H_1_2)
        {
            printf("%10s\t%4d\t%10x\t%10s\t%s\n", "NO-I2C", ftDevInfo[i].Type, ftDevInfo[i].LocId,
                    ftDevInfo[i].SerialNumber, ftDevInfo[i].Description);
        }
    }
    printf("[%d]I2C interface detected.\n", ftI2cIfAmount);

    exit: free(ftDevInfo);
    //Return I2c If number & LocationId
    *numOfDev = ftI2cIfAmount;
    for (i = 0; i < ftI2cIfAmount; i++)
    {
        locationid[i] = ftDevInfo[i].LocId;
    }
    return retCode;
}

int i2cm_init()
{
    FT_STATUS ftStatus;

    // Open a FT4222 device by DESCRIPTION
    CHECK_FUNC_EXIT(ftStatus, FT_OpenEx("FT4222 A", FT_OPEN_BY_DESCRIPTION, &gFtHandle));
    CHECK_FUNC_EXIT(ftStatus, FT_OpenEx("FT4222 A", FT_OPEN_BY_DESCRIPTION, &gFtHandle));

    exit: return 0;
}

void i2cm_printOp(st_I2cOps *op)
{
    int i;
    static char *optitle[10] =
    {
            "NoOperation","Write","Read","RegWrite","RegRead","PollAck","Scan"
    };
    printf("Operation:\t%s\n", optitle[op->Operation]);
    printf("SlvAddr:\t[0x%X]", op->SlaveAddr);
    if(op->RegAddrLen)
    {
        printf("\nRegLen:\t\t%d\nRegAddr:\t", op->RegAddrLen);
        for (i = 0; i < op->RegAddrLen; i++)
        {
            printf("[0x%X]\t", op->RegAddrBuf[i]);
        }
    }
    if(op->TxLen)
    {
        printf("\nTxLen:\t\t%d\nTxData:\t\t", op->TxLen);
        for (i = 0; i < op->TxLen; i++)
        {
            printf("[0x%X]\t", op->TxBuf[i]);
        }
    }
    if(op->RxLen)
    {
        printf("\nRxLen:\t\t%d\nRxData:\t\t", op->RxLen);
        for (i = 0; i < op->RxLen; i++)
        {
            printf("[0x%X]\t", op->RxBuf[i]);
        }
    }
    printf("\n");
}
/* Parse Command args into the Syntax:
 *      [arg0]      [arg1]      [arg2]      [arg3]      [arg4]
 * i2c  [write]     [slv_addr]  [data0]                             ... [dataN]
 * i2c  [read]      [slv_addr]  [read_len]
 * i2c  [reg_write] [slv_addr]  [reg_addr]  [reg_size]  [data0]     ... [dataN]
 * i2c  [reg_read]  [slv_addr]  [reg_addr]  [reg_size]  [read_len]
 * i2c  [poll_ack]   [slv_addr]
 * i2c  [scan]
 *
 */
int i2cm_ArgsPrase(st_I2cOps *op, int argc, char *args[])
{
    int i = 0;
    char *s = NULL;

    if (strcmp("write", args[0]) == 0)
    {
        op->Operation = WRITE;
        op->SlaveAddr = (uint8_t) strtol(args[1], &s, 0);
        op->TxLen = argc - 2;
        for (i = 0; i < op->TxLen; i++)
        {
            op->TxBuf[i] = (uint8_t) strtol(args[i + 2], &s, 0);
        }
    }
    if (strcmp("read", args[0]) == 0)
    {
        op->Operation = READ;
        op->SlaveAddr = (uint8_t) strtol(args[1], &s, 0);
        op->RxLen = (uint8_t) strtol(args[2], &s, 0);
    }
    if (strcmp("reg_write", args[0]) == 0)
    {
        long regtemp = 0;
        op->Operation = REG_WRITE;
        op->SlaveAddr = (uint8_t) strtol(args[1], &s, 0);
        regtemp = (uint32_t) strtol(args[2], &s, 0);
        op->RegAddrLen = (uint8_t) strtol(args[3], &s, 0);
        for (i = 0; i < op->RegAddrLen; i++)
        {
            op->RegAddrBuf[op->RegAddrLen - i - 1] = (uint8_t) (regtemp >> (8 * i));
        }
        op->TxLen = argc - 4;
        for (i = 0; i < op->TxLen; i++)
        {
            op->TxBuf[i] = (uint8_t) strtol(args[i + 4], &s, 0);
        }
    }
    if (strcmp("reg_read", args[0]) == 0)
    {
        long regtemp = 0;
        op->Operation = REG_READ;
        op->SlaveAddr = (uint8_t) strtol(args[1], &s, 0);
        regtemp = (uint32_t) strtol(args[2], &s, 0);
        op->RegAddrLen = (uint8_t) strtol(args[3], &s, 0);
        for (i = 0; i < op->RegAddrLen; i++)
        {
            op->RegAddrBuf[op->RegAddrLen - i - 1] = (uint8_t) (regtemp >> (8 * i));
        }
        op->RxLen = (uint8_t) strtol(args[4], &s, 0);
    }
    if (strcmp("poll_ack", args[0]) == 0)
    {
        op->Operation = POLLACK;
        op->SlaveAddr = (uint8_t) strtol(args[1], &s, 0);
    }
    if (strcmp("scan", args[0]) == 0)
    {
        op->Operation = SCAN;
    }

    i2cm_printOp(op);
    return 0;
}

int i2cm_write(st_I2cOps *op)
{
    uint16 trans_count;    //Actual transfer count;
    uint8 status;

    CHECK_FUNC_RET(gFtStatus,
            FT4222_I2CMaster_WriteEx(gFtHandle, op->SlaveAddr, START_AND_STOP, op->TxBuf, op->TxLen, &trans_count));
    CHECK_FUNC_RET(gFtStatus, FT4222_I2CMaster_GetStatus(gFtHandle, &status));

    return status;
}
int i2cm_read(st_I2cOps *op)
{
    uint16 trans_count;    //Actual transfer count;
    uint8 status;

    CHECK_FUNC_RET(gFtStatus,
            FT4222_I2CMaster_ReadEx(gFtHandle, op->SlaveAddr, START_AND_STOP, op->RxBuf, op->RxLen, &trans_count));
    CHECK_FUNC_RET(gFtStatus, FT4222_I2CMaster_GetStatus(gFtHandle, &status));

    return status;
}
int i2cm_reg_write(st_I2cOps *op)
{
    uint16 trans_count;    //Actual transfer count;
    uint8 status;

    CHECK_FUNC_RET(gFtStatus,
            FT4222_I2CMaster_WriteEx(gFtHandle, op->SlaveAddr, START, op->RegAddrBuf, op->RegAddrLen, &trans_count));
    CHECK_FUNC_RET(gFtStatus,
            FT4222_I2CMaster_WriteEx(gFtHandle, op->SlaveAddr, START_AND_STOP, op->TxBuf, op->TxLen, &trans_count));
    CHECK_FUNC_RET(gFtStatus, FT4222_I2CMaster_GetStatus(gFtHandle, &status));

    return status;
}
int i2cm_reg_read(st_I2cOps *op)
{
    uint16 trans_count;    //Actual transfer count;
    uint8 status;

    CHECK_FUNC_RET(gFtStatus,
            FT4222_I2CMaster_WriteEx(gFtHandle, op->SlaveAddr, START, op->RegAddrBuf, op->RegAddrLen, &trans_count));
    CHECK_FUNC_RET(gFtStatus,
            FT4222_I2CMaster_ReadEx(gFtHandle, op->SlaveAddr, START_AND_STOP, op->RxBuf, op->RxLen, &trans_count));
    CHECK_FUNC_RET(gFtStatus, FT4222_I2CMaster_GetStatus(gFtHandle, &status));

    return status;
}
int i2cm_pollack(st_I2cOps *op)
{
    return 0;
}
int i2cm_scan(st_I2cOps *op)
{
    return 0;
}

int i2cm_processOp(st_I2cOps *op)
{
    //Init I2C if not initialized.
    if (gFtHandle == NULL)
    {
        i2cm_init();
    }

    //Do operation
    switch (op->Operation)
    {
    case WRITE:
    {
        i2cm_write(op);
        break;
    }
    case READ:
    {
        i2cm_read(op);
        break;
    }
    case REG_WRITE:
    {
        i2cm_reg_write(op);
        break;
    }
    case REG_READ:
    {
        i2cm_reg_read(op);
        break;
    }
    default:
    {
        printf("Not supported operation");
        return 0;
    }
    }

    return gFtStatus;
}
