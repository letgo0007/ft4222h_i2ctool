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
            printf("\033[31mERROR: "#func" Return=[%d]! <%s:%d>\033[0m\n", ret, __FILE__, __LINE__);\
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
            printf("\033[31mERROR: "#func" Return=[%d]! <%s:%d>\033[0m\n", ret, __FILE__, __LINE__);\
            goto exit;\
        }\
    } while (0)

static FT_STATUS gFtStatus = 0;
static FT_HANDLE gFtHandle = NULL;

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
            printf("%10s\t%4d\t%10x\t%10s\t%s\n", "NO-I2C", ftDevInfo[i].Type, ftDevInfo[i].LocId, ftDevInfo[i].SerialNumber,
                    ftDevInfo[i].Description);
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

    exit: return 0;
}

/* Command Syntax:
 * i2c write 		[slv_addr] [data0] 	[data1] ... [dataN]
 * i2c read  		[slv_addr] [read_length]
 * i2c reg_write 	[slv_addr] [reg_addr] [reg_size]	 [data0] [data1] ... [data N]
 * i2c reg_read		[slv_addr] [reg_addr] [reg_size]	 [read_length]
 * i2c get_ack		[slv_addr]
 * i2c scan
 */
int i2cm_processCommand(int argc, char *args[])
{

    const char *s = NULL;
    int8_t *command = args[0];
    uint8_t slv_addr = 0;
    uint8_t data[256] =
    { 0 };
    uint32_t reg_addr = 0;
    uint8_t reg_size = 0;
    int i;
    int count;
    uint8_t status;

    printf("Command: [%d] %s %s %s %s %s\n", argc, args[0], args[1], args[2], args[3], args[4]);
    if (strcmp(command, "write") == 0)
    {
        slv_addr = strtol(args[1], (char **) &s, 0);

        for (i = 0; i < argc - 2; i++)
        {
            data[i] = strtol(args[i + 2], (char **) &s, 0);
        }

        printf("slv_addr[%d] %d %d %d %d\n", slv_addr, data[0], data[1], data[2], data[3]);
        CHECK_FUNC_EXIT(gFtStatus, FT4222_I2CMaster_ReadEx(gFtHandle, slv_addr, START_AND_STOP, data, argc - 2, &count));
        CHECK_FUNC_EXIT(gFtStatus, FT4222_I2CMaster_GetStatus(gFtHandle, &status));
    }
    exit: return 0;
}

int i2cm_processOp(st_I2cOps op)
{
    uint8_t status;
    uint16_t transfered_count;

    //Init I2C if not initialized.
    if (gFtHandle == NULL)
    {
        i2cm_init();
    }

    //Do operation
    switch (op.Operation)
    {
    case WRITE:
    {
        CHECK_FUNC_EXIT(gFtStatus,
                FT4222_I2CMaster_WriteEx(gFtHandle, op.SlaveAdd, START_AND_STOP, op.WritePtr, op.WriteLen, &transfered_count));
        CHECK_FUNC_EXIT(gFtStatus, FT4222_I2CMaster_GetStatus(gFtHandle, &status));
        break;
    }
    case READ:
    {
        break;
    }
    case REG_WRITE:
    {
        break;
    }
    case REG_READ:
    {
        break;
    }
    default:
    {
        printf("Not supported operation");
        return 0;
    }
    }

    exit: return gFtStatus;
}
