/*
 *i2c
 *This command will read or write data to a particular I2C device, or sweep an I2C bus for devices.
 *Parameters:
 *--read|-r [Bus] [Addr] [Length] Read raw data
 *      Bus - Bus to perform the read on
 *      Addr - I2C Addr to read from (in hex)
 *      Length - Number of bytes to read
 *--devread|-d [Bus] [Addr] [Reg] [Len]   Read register data
 *      Bus - Bus to perform the read on
 *      Addr - I2C Addr to read from (in hex)
 *      Reg - Device register to start reading from
 *      Len - Number of bytes to read
 *--write|-w [Bus] [Addr] [Data]  Write register data
 *      Bus - Bus to perform the write on
 *      Addr - I2C Addr to write to (in hex)
 v      Data - String of bytes to write out
 *--devwrite|-v [Bus] [Addr] [Reg] [Data] Write register data
 *      Bus - Bus to perform the write on
 *      Addr - I2C Addr to write to (in hex)
 *      Reg - Device register to start writing to
 *      Data - String of bytes to write out
 *--maskwrite|-m [Bus] [Addr] [Reg] [Mask] [Data] Write register data
 *      Bus - Bus to perform the write on
 *      Addr - I2C Addr to write to (in hex)
 *      Reg - Device register to start writing to
 *      Mask - Mask to apply to Data
 *      Data - String of bytes to write out
 *--sweep|-s [Bus]    Sweep I2C bus for devices
 *      Bus - Bus to sweep
 *--addrsize|-z [Size]
 *      (optional)  Register address size in bytes. If not specified, it defaults to 1.
 *      Size - Accept values: 1 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ftd2xx.h>
#include <libft4222.h>

#include "cli.h"

// FT_STATUS message
static const char *FT_RET_MSG[] =
{ "FT_OK", "FT_INVALID_HANDLE", "FT_DEVICE_NOT_FOUND", "FT_DEVICE_NOT_OPENED", "FT_IO_ERROR",
        "FT_INSUFFICIENT_RESOURCES", "FT_INVALID_PARAMETER", "FT_INVALID_BAUD_RATE", "FT_DEVICE_NOT_OPENED_FOR_ERASE",
        "FT_DEVICE_NOT_OPENED_FOR_WRITE", "FT_FAILED_TO_WRITE_DEVICE", "FT_EEPROM_READ_FAILED",
        "FT_EEPROM_WRITE_FAILED", "FT_EEPROM_ERASE_FAILED", "FT_EEPROM_NOT_PRESENT", "FT_EEPROM_NOT_PROGRAMMED",
        "FT_INVALID_ARGS", "FT_NOT_SUPPORTED", "FT_OTHER_ERROR", "FT_DEVICE_LIST_NOT_READY", };

// FT_STATUS extending message, for FT4222H only, starting from 1000
static const char *FT_RET_MSG_EXTEND[] =
{ "FT4222_DEVICE_NOT_SUPPORTED", "FT4222_CLK_NOT_SUPPORTED", "FT4222_VENDER_CMD_NOT_SUPPORTED",
        "FT4222_IS_NOT_SPI_MODE", "FT4222_IS_NOT_I2C_MODE", "FT4222_IS_NOT_SPI_SINGLE_MODE",
        "FT4222_IS_NOT_SPI_MULTI_MODE", "FT4222_WRONG_I2C_ADDR", "FT4222_INVAILD_FUNCTION", "FT4222_INVALID_POINTER",
        "FT4222_EXCEEDED_MAX_TRANSFER_SIZE", "FT4222_FAILED_TO_READ_DEVICE", "FT4222_I2C_NOT_SUPPORTED_IN_THIS_MODE",
        "FT4222_GPIO_NOT_SUPPORTED_IN_THIS_MODE", "FT4222_GPIO_EXCEEDED_MAX_PORTNUM", "FT4222_GPIO_WRITE_NOT_SUPPORTED",
        "FT4222_GPIO_PULLUP_INVALID_IN_INPUTMODE", "FT4222_GPIO_PULLDOWN_INVALID_IN_INPUTMODE",
        "FT4222_GPIO_OPENDRAIN_INVALID_IN_OUTPUTMODE", "FT4222_INTERRUPT_NOT_SUPPORTED",
        "FT4222_GPIO_INPUT_NOT_SUPPORTED", "FT4222_EVENT_NOT_SUPPORTED", "FT4222_FUN_NOT_SUPPORT" };

//General Print
#define CLI_PRINT(msg, args...)  \
    do {\
        printf(msg, ##args);\
    } while (0)

//Warning info
#define CLI_WARNING(msg, args...)  \
    do {\
        printf("\e[33m"msg"\e[0m", ##args);\
    } while (0)

//Error Message output, with RED color.
#define CLI_ERROR(msg, args...)  \
    do {\
        printf("\e[31m"msg"\e[0m", ##args);\
    } while (0)

//Check null pointer and return failure with a simple error message.
#define CHECK_NULL_PTR(ptr) \
    do {\
        if (ptr == NULL) \
        {\
            CLI_ERROR("ERROR: NULL pointer="#ptr"<%s:%d>\n", __FILE__, __LINE__);\
            return CLI_FAILURE;\
        }\
    }while(0)

//Check function return = CLI_SUCCESS, otherwise jump to exit label with a error message.
#define CHECK_FUNC_RET(status, func) \
    do {\
        int ret = func;\
        if (status != ret)\
        {\
            if(ret >= 1000)\
                CLI_ERROR("ERROR: Return=[%d] %s <%s:%d>\n", ret, FT_RET_MSG_EXTEND[ret-1000], __FILE__, __LINE__);\
            else\
                CLI_ERROR("ERROR: Return=[%d] %s <%s:%d>\n", ret, FT_RET_MSG[ret], __FILE__, __LINE__);\
            return ret;\
        }\
    } while (0)

//Static buffers
static uint8 gbuf_value[256] =
{ 0 };
static uint16 gbuf_count = 0;

//Print uint8 data array
void print_u8(int c, uint8 *d)
{
    for (int i = 0; i < c; i++)
    {
        CLI_PRINT("0x%02X\t", d[i]);
    }
    CLI_PRINT("\n");
}

//Print args
int print_args(int argc, char **args)
{
    if ((argc == 0) || (args == 0))
    {
        return -1;
    }

    //Example of a call back to handle un-used args.
    //Pass un-used args back;
    int i;
    printf("Un-used argc = [%d]\nUn-used args = ", argc);

    for (i = 0; i < argc; i++)
    {
        printf("%s ", args[i]);
    }
    printf("\n");
    return 0;
}

void print_devinfo(FT_DEVICE_LIST_INFO_NODE *devInfo)
{
    int i = 0;

    for (i = 0; devInfo[i].ID != 0; i++)
    {
        CLI_PRINT("Dev Index [%d]:\n", i);
        CLI_PRINT("  Flags\t\t=0x%x\n", devInfo[i].Flags);
        CLI_PRINT("  Type\t\t=0x%x\n", devInfo[i].Type);
        CLI_PRINT("  ID\t\t=0x%x\n", devInfo[i].ID);
        CLI_PRINT("  LocId\t\t=0x%x\n", devInfo[i].LocId);
        CLI_PRINT("  SerialNumber\t=%s\n", devInfo[i].SerialNumber);
        CLI_PRINT("  Description\t=%s\n", devInfo[i].Description);
        CLI_PRINT("  ftHandle\t=0x%X\n", (int ) devInfo[i].ftHandle);
    }
}

//Convert string to uint8
int str_to_u8(int argc, char *argv[])
{
    int i = 0;
    gbuf_count = 0;

    for (i = 0; i < argc; i++)
    {
        if (argv[i] == NULL)
        {
            return 0;
        }
        char *tail = NULL;

        gbuf_value[gbuf_count] = strtol(argv[i], &tail, 0);

        if (tail[0] != 0)
        {
            CLI_WARNING("[Warning]Ignore un-recognized string of [%s]\n", argv[i]);
        }
        else
        {
            gbuf_count++;
        }
    }

    return i;
}

FT_STATUS FT_getVersion(FT_HANDLE ftHandle)
{
    FT4222_Version ft4222Version;
    FT4222_GetVersion(ftHandle, &ft4222Version);
    printf("Chip version: %08X, LibFT4222 version: %08X\n", (unsigned int) ft4222Version.chipVersion,
            (unsigned int) ft4222Version.dllVersion);
    return FT_OK;
}

uint8 FT_checkI2cAddr(FT_HANDLE ftHandle, uint8 slvadd)
{
    uint8 ReadPtr[1] =
    { 0 };
    uint16 TransferSize = 0;
    uint8 i2cstatus = 0;

    CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_ReadEx(ftHandle, slvadd, START_AND_STOP, ReadPtr, 1, &TransferSize));
    CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_GetStatus(ftHandle, &i2cstatus));
    CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_Reset(ftHandle));

    if (I2CM_ADDRESS_NACK(i2cstatus))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

uint8 FT_checkI2cBus(FT_HANDLE ftHandle)
{
    uint8 i2cstatus = 0;
    uint32 timeout = 0;

    CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_GetStatus(ftHandle, &i2cstatus));

    //Wait bus busy flag.
    while (I2CM_BUS_BUSY(i2cstatus) || I2CM_CONTROLLER_BUSY(i2cstatus))
    {
        timeout++;
        usleep(1000);
        if (timeout > 1000)
        {
            CLI_ERROR("I2C BUS Timeout: I2CM_BUS_BUSY, Error Code = [0x%X]\n", i2cstatus);
            break;
        }
        CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_GetStatus(ftHandle, &i2cstatus));
    }

    // The normal condition should be bus free
    if (i2cstatus == 0x20)
    {
        return FT_OK;
    }

    // Print Error Message
    if (i2cstatus & 0x02)
    {
        CLI_ERROR("I2C BUS ERROR: ");
        if (I2CM_DATA_NACK(i2cstatus))
        {
            CLI_ERROR("[I2CM_DATA_NACK] ");
        }
        if (I2CM_ADDRESS_NACK(i2cstatus))
        {
            CLI_ERROR("[I2CM_ADDRESS_NACK] ");
        }
        if (I2CM_ARB_LOST(i2cstatus))
        {
            CLI_ERROR("[I2CM_ARB_LOST] ");
        }
        CLI_ERROR("\n");
    }

    FT4222_I2CMaster_Reset(ftHandle);

    return FT_OTHER_ERROR;
}

int FT_listI2cBus(FT_DEVICE_LIST_INFO_NODE *I2cDevInfo)
{
    FT_STATUS ftStatus;
    FT_DEVICE_LIST_INFO_NODE *devInfo;
    DWORD numDevs = 0;
    DWORD numI2cDevs = 0;

    // Create the device information list
    ftStatus = FT_CreateDeviceInfoList(&numDevs);

    if (numDevs > 0)
    {
        // allocate storage for list based on numDevs
        devInfo = (FT_DEVICE_LIST_INFO_NODE*) malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevs);
        // get the device information list
        ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
        if (ftStatus == FT_OK)
        {
            for (int i = 0; i < numDevs; i++)
            {
                if (strcmp(devInfo[i].Description, "FT4222 A") == 0)
                {
                    //Copy device info
                    memcpy(&I2cDevInfo[numI2cDevs], &devInfo[i], sizeof(FT_DEVICE_LIST_INFO_NODE));
                    numI2cDevs++;
                }

            }
        }
    }

    return numI2cDevs;
}

FT_STATUS FT_openI2cBus(int devicenumber, FT_HANDLE *pHandle, uint32 kbps)
{
    FT_DEVICE_LIST_INFO_NODE devInfo[16];
    DWORD numI2cDevs = 0;
    DWORD locid = 0;

    numI2cDevs = FT_listI2cBus(devInfo);
    if (numI2cDevs == 0)
    {
        CLI_ERROR("ERROR: No FT4222 I2C found! exit(-1)\n");
        exit(-1);
    }

    if (devicenumber < numI2cDevs)
    {
        locid = devInfo[devicenumber].LocId;
    }
    else
    {
        locid = devInfo[0].LocId;
        CLI_WARNING("WARNING: Can't find I2C bus [%d], use I2C bus [0] instead, location ID = [0x%X].\n", devicenumber,
                locid);
    }

    CHECK_FUNC_RET(FT_OK, FT_OpenEx((void *)locid, FT_OPEN_BY_LOCATION, pHandle));
    //CHECK_FUNC_RET(FT_OK, FT_OpenEx("FT4222 A", FT_OPEN_BY_DESCRIPTION, pHandle));
    CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_Init(*pHandle, kbps));
    return FT_OK;
}

int command_i2c(int argc, char *argv[])
{
    /********************************************************
     * Parse Augments
     ********************************************************/
    // Build required parameter structure
    struct param_i2c
    {
        int ch_read;
        int ch_write;
        int ch_devread;
        int ch_devwrite;
        int ch_maskwrite;
        int ch_sweep;
        _Bool i2c_list;
        int reg_length;
        int i2c_kbps;
    } param_i2c;

    // Set default value
    param_i2c.ch_read = -1;
    param_i2c.ch_write = -1;
    param_i2c.ch_devread = -1;
    param_i2c.ch_devwrite = -1;
    param_i2c.ch_maskwrite = -1;
    param_i2c.ch_sweep = -1;
    param_i2c.reg_length = 1;
    param_i2c.i2c_kbps = 100;

    //Build option structure.
    stCliOption option_i2c[] =
    {
    { OPT_COMMENT, 0, NULL, "Basic Parameters", NULL },
    { OPT_INT, 'r', "read", "[Bus] [Addr] [Length] Read raw data", (void*) &param_i2c.ch_read },
    { OPT_INT, 'd', "devread", "[Bus] [Addr] [Reg] [Len] Read register data", (void*) &param_i2c.ch_devread },
    { OPT_INT, 'w', "write", "[Bus] [Addr] [Data] Write raw data", (void*) &param_i2c.ch_write },
    { OPT_INT, 'v', "devwrite", "[Bus] [Addr] [Reg] [Data] Write register data", (void*) &param_i2c.ch_devwrite },
    { OPT_INT, 'm', "maskwrite", "[Bus] [Addr] [Reg] [Mask] [Data]Write register data with mask",
            (void*) &param_i2c.ch_maskwrite },
    { OPT_INT, 's', "sweep", "[Bus] Sweep I2C bus for devices", (void*) &param_i2c.ch_sweep },
    { OPT_BOOL, 'l', "list", "List I2c bus available", (void*) &param_i2c.i2c_list },
    { OPT_COMMENT, 0, NULL, "Optional Parameters", NULL },
    { OPT_INT, 'f', "freq", "[Freq] Set I2c frequency in kHz. Default is 100.", (void*) &param_i2c.i2c_kbps },
    { OPT_INT, 'z', "addrsize", "[Size] Register address size in bytes. Default is 1.", (void*) &param_i2c.reg_length },
    { OPT_HELP, 'h', "help", "Show help hints", NULL },
    { OPT_END, 0, NULL, NULL, NULL, str_to_u8 } };

    //Run Arguments parse using option_i2c
    CLI_parseArgs(argc, argv, option_i2c);

    /********************************************************
     * I2C operation
     ********************************************************/
    FT_HANDLE ftHandle = 0;
    uint8 Addr = 0;
    uint8 Mask = 0;
    uint16 Length = 0;
    uint16 TransferSize = 0;
    uint8 *WritePtr = NULL;
    uint8 *ReadPtr = NULL;
    uint8 *RegPtr = NULL;

    //--read|-r [Bus] [Addr] [Length] Read raw data
    if (param_i2c.ch_read >= 0)
    {
        //Check minimum args count
        if (gbuf_count < 2)
        {
            CLI_ERROR("ERROR:Not enough parameters. Try [--help].\n");
            return FT_INVALID_PARAMETER;
        }

        //Handle command syntax
        Addr = gbuf_value[0];
        Length = gbuf_value[1];
        ReadPtr = &gbuf_value[2];

        //Initial I2C port
        CHECK_FUNC_RET(FT_OK, FT_openI2cBus(param_i2c.ch_read, &ftHandle, param_i2c.i2c_kbps));

        //I2c read operation
        CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_ReadEx(ftHandle, Addr, START_AND_STOP, ReadPtr, Length, &TransferSize));
        CHECK_FUNC_RET(FT_OK, FT_checkI2cBus(ftHandle));

        //Print read result
        CLI_PRINT("I2C READ, count=[%d]\n", TransferSize);
        print_u8(TransferSize, ReadPtr);
    }

    //--devread|-d [Bus] [Addr] [Reg] [Length]   Read register data
    if (param_i2c.ch_devread >= 0)
    {
        //Check minimum args count
        if (gbuf_count < 3)
        {
            CLI_ERROR("ERROR:Not enough parameters, Try [--help].\n");
            return FT_INVALID_PARAMETER;
        }

        //1. Handle command syntax
        Addr = gbuf_value[0];
        RegPtr = &gbuf_value[1];
        if (param_i2c.reg_length == 1)
        {
            Length = gbuf_value[2];
            ReadPtr = &gbuf_value[3];
        }
        else if (param_i2c.reg_length == 2)
        {
            Length = gbuf_value[3];
            ReadPtr = &gbuf_value[4];
        }
        else
        {
            CLI_ERROR("ERROR:Invalid register size, must be 1 or 2.");
            return FT_INVALID_PARAMETER;
        }

        //2. Initial I2C port
        CHECK_FUNC_RET(FT_OK, FT_openI2cBus(param_i2c.ch_devread, &ftHandle, param_i2c.i2c_kbps));

        //3. I2c write/read operation
        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_WriteEx(ftHandle, Addr, START, RegPtr, param_i2c.reg_length, &TransferSize));
        CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_ReadEx(ftHandle, Addr, START_AND_STOP, ReadPtr, Length, &TransferSize));
        CHECK_FUNC_RET(FT_OK, FT_checkI2cBus(ftHandle));
        //4. Print read result
        if (param_i2c.reg_length == 1)
        {
            CLI_PRINT("I2C REG_READ, REG=[0x%02X], count=[%d]\n", RegPtr[0], TransferSize);
        }
        else if (param_i2c.reg_length == 2)
        {
            CLI_PRINT("I2C REG_READ, REG=[0x%02X%02X], count=[%d]\n", RegPtr[0], RegPtr[1], TransferSize);
        }
        print_u8(TransferSize, ReadPtr);

    }

    //--write|-w [Bus] [Addr] [Data]  Write register data
    if (param_i2c.ch_write >= 0)
    {
        //Check minimum args count
        if (gbuf_count < 2)
        {
            CLI_ERROR("ERROR:Not enough parameters, Try [--help].\n");
            return FT_INVALID_PARAMETER;
        }

        //1. Handle command syntax
        Addr = gbuf_value[0];
        WritePtr = &gbuf_value[1];
        Length = gbuf_count - 1;

        //2. Initial I2C port
        CHECK_FUNC_RET(FT_OK, FT_openI2cBus(param_i2c.ch_write, &ftHandle, param_i2c.i2c_kbps));

        //3. I2C write operation
        CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_Write(ftHandle, Addr, WritePtr, Length, &TransferSize));
        CHECK_FUNC_RET(FT_OK, FT_checkI2cBus(ftHandle));
        //CHECK_FUNC_RET(FT_OK, FT4222_I2CMaster_WriteEx(ftHandle, Addr, START_AND_STOP, WritePtr, Length, &TransferSize));

        //4. Print read result
        CLI_PRINT("I2C WRITE, count=[%d]\n", TransferSize);
        print_u8(TransferSize, WritePtr);
    }

    //--devwrite|-v [Bus] [Addr] [Reg] [Data] Write register data
    if (param_i2c.ch_devwrite >= 0)
    {
        //Check minimum args count
        if (gbuf_count < 3)
        {
            CLI_ERROR("ERROR:Not enough parameters, Try [--help].\n");
            return FT_INVALID_PARAMETER;
        }

        //1. Handle command syntax
        Addr = gbuf_value[0];
        RegPtr = &gbuf_value[1];
        if (param_i2c.reg_length == 1)
        {
            Length = gbuf_count - 2;
            WritePtr = &gbuf_value[2];
        }
        else if (param_i2c.reg_length == 2)
        {
            Length = gbuf_count - 3;
            WritePtr = &gbuf_value[3];
        }
        else
        {
            CLI_ERROR("ERROR:invalid register size, must be 1 or 2.");
            return -1;
        }

        //2. Initial I2C port
        CHECK_FUNC_RET(FT_OK, FT_openI2cBus(param_i2c.ch_devwrite, &ftHandle, param_i2c.i2c_kbps));

        //3. I2c write operation
        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_WriteEx(ftHandle, Addr, START, RegPtr, param_i2c.reg_length, &TransferSize));
        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_WriteEx(ftHandle, Addr, Repeated_START | STOP, WritePtr, Length, &TransferSize));
        CHECK_FUNC_RET(FT_OK, FT_checkI2cBus(ftHandle));

        //4. Print read result
        if (param_i2c.reg_length == 1)
        {
            CLI_PRINT("I2C REG_WRITE, REG=[0x%02X], count=[%d]\n", RegPtr[0], TransferSize);
        }
        else if (param_i2c.reg_length == 2)
        {
            CLI_PRINT("I2c EG_WRITE, REG=[0x%02X%02X], count=[%d]\n", RegPtr[0], RegPtr[1], TransferSize);
        }
        print_u8(TransferSize, WritePtr);
    }

    //--maskwrite|-m [Bus] [Addr] [Reg] [Mask] [Data] Write register data
    if (param_i2c.ch_maskwrite >= 0)
    {
        //Check minimum args count
        if (gbuf_count < 4)
        {
            CLI_ERROR("ERROR:Not enough parameters, Try [--help].\n");
            return FT_INVALID_PARAMETER;
        }

        //1. Handle command syntax
        Addr = gbuf_value[0];
        RegPtr = &gbuf_value[1];

        if (param_i2c.reg_length == 1)
        {
            Mask = gbuf_value[2];
            Length = gbuf_count - 2;
            WritePtr = &gbuf_value[3];
        }
        else if (param_i2c.reg_length == 2)
        {
            Mask = gbuf_value[3];
            Length = gbuf_count - 3;
            WritePtr = &gbuf_value[4];
        }
        else
        {
            CLI_ERROR("ERROR:invalid register size, must be 1 or 2.");
            return -1;
        }

        ReadPtr = (uint8*) malloc(Length);

        //2. Initial I2C port
        CHECK_FUNC_RET(FT_OK, FT_openI2cBus(param_i2c.ch_maskwrite, &ftHandle, param_i2c.i2c_kbps));

        //3. I2C read and mask write
        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_WriteEx(ftHandle, Addr, START, RegPtr, param_i2c.reg_length, &TransferSize));
        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_ReadEx(ftHandle, Addr, Repeated_START | STOP, ReadPtr, Length, &TransferSize));

        for (int i = 0; i < Length; i++)
        {
            WritePtr[i] = ReadPtr[i] & Mask;
        }

        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_WriteEx(ftHandle, Addr, START, RegPtr, param_i2c.reg_length, &TransferSize));
        CHECK_FUNC_RET(FT_OK,
                FT4222_I2CMaster_WriteEx(ftHandle, Addr, START_AND_STOP, WritePtr, Length, &TransferSize));

        //4. Print result
        CLI_PRINT("I2c MASK_WRITE, REG=[0x%02X], count=[%d]\n", RegPtr[0], TransferSize);
        print_u8(TransferSize, WritePtr);
    }

    //--sweep|-s [Bus]    Sweep I2C bus for devices
    if (param_i2c.ch_sweep >= 0)
    {
        uint8 i = 0;
        uint8 count = 0;
        uint8 result = -1;

        CLI_PRINT("I2C slave sweep on bus [%d]\n", param_i2c.ch_sweep);

        //2. Initial I2C port
        CHECK_FUNC_RET(FT_OK, FT_openI2cBus(param_i2c.ch_sweep, &ftHandle, param_i2c.i2c_kbps));

        for (i = 0; i < 0x80; i++)
        {
            result = FT_checkI2cAddr(ftHandle, i);
            if (result == FT_OK)
            {
                CLI_PRINT("I2C slave detected: 0x%02X\n", i);
                count++;
            }
        }
    }

    if (param_i2c.i2c_list)
    {
        FT_DEVICE_LIST_INFO_NODE devInfo[16];
        DWORD numI2cDevs = 0;

        numI2cDevs = FT_listI2cBus(devInfo);

        CLI_PRINT("I2C Master Bus Count = [%d]\n-----------------\n", numI2cDevs);
        print_devinfo(devInfo);
    }

    //Finish all operation, close device.
    if (ftHandle)
    {
        FT4222_UnInitialize(ftHandle);
        FT_Close(ftHandle);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    return command_i2c(--argc, ++argv);
}
