/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    user_diskio.c
  * @brief   FatFs USER disk I/O driver for SD card over SPI1
  ******************************************************************************
  */
/* USER CODE END Header */

#include <string.h>

#include "ff_gen_drv.h"
#include "sd_spi.h"

/* Private variables ---------------------------------------------------------*/

static volatile DSTATUS Stat = STA_NOINIT;


/* Private function prototypes -----------------------------------------------*/

DSTATUS USER_initialize(BYTE pdrv);
DSTATUS USER_status(BYTE pdrv);

DRESULT USER_read(
    BYTE pdrv,
    BYTE *buff,
    DWORD sector,
    UINT count
);

#if _USE_WRITE == 1
DRESULT USER_write(
    BYTE pdrv,
    const BYTE *buff,
    DWORD sector,
    UINT count
);
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff
);
#endif


/* Disk driver ---------------------------------------------------------------*/

Diskio_drvTypeDef USER_Driver =
{
    USER_initialize,
    USER_status,
    USER_read,

#if _USE_WRITE
    USER_write,
#endif

#if _USE_IOCTL
    USER_ioctl,
#endif
};


/* ============================================================
 * INITIALIZE DISK
 * ============================================================ */

DSTATUS USER_initialize(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }

    /*
     * Initialize SD card through our SPI1 driver.
     */

    if (SD_Init() == 0)
    {
        Stat = 0;
    }
    else
    {
        Stat = STA_NOINIT;
    }

    return Stat;
}


/* ============================================================
 * GET DISK STATUS
 * ============================================================ */

DSTATUS USER_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }

    return Stat;
}


/* ============================================================
 * READ SECTOR(S)
 * ============================================================ */

DRESULT USER_read(
    BYTE pdrv,
    BYTE *buff,
    DWORD sector,
    UINT count
)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }

    if (buff == NULL)
    {
        return RES_PARERR;
    }

    /*
     * Read each requested 512-byte sector.
     */

    for (UINT i = 0; i < count; i++)
    {
        if (SD_ReadBlock(
                sector + i,
                &buff[i * 512]
            ) != 0)
        {
            return RES_ERROR;
        }
    }

    return RES_OK;
}


/* ============================================================
 * WRITE SECTOR(S)
 * ============================================================ */

#if _USE_WRITE == 1

DRESULT USER_write(
    BYTE pdrv,
    const BYTE *buff,
    DWORD sector,
    UINT count
)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }

    if (buff == NULL)
    {
        return RES_PARERR;
    }

    /*
     * Write each requested 512-byte sector.
     */

    for (UINT i = 0; i < count; i++)
    {
        if (SD_WriteBlock(
                sector + i,
                &buff[i * 512]
            ) != 0)
        {
            return RES_ERROR;
        }
    }

    return RES_OK;
}

#endif


/* ============================================================
 * IOCTL
 * ============================================================ */

#if _USE_IOCTL == 1

DRESULT USER_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff
)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }

    /*
     * These values are required by FatFs.
     *
     * Our current SD driver does not yet implement
     * card-specific geometry/status commands.
     *
     * Return basic 512-byte sector information.
     */

    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_SIZE:
            if (buff != NULL)
            {
                *(WORD *)buff = 512;
                return RES_OK;
            }
            break;

        case GET_BLOCK_SIZE:
            if (buff != NULL)
            {
                *(DWORD *)buff = 1;
                return RES_OK;
            }
            break;

        case GET_SECTOR_COUNT:
            if (buff != NULL)
            {
                /*
                 * Temporary value.
                 *
                 * This will be replaced with the actual
                 * SD card capacity information when needed.
                 */
                *(DWORD *)buff = 124702720;
                return RES_OK;
            }
            break;

        default:
            break;
    }

    return RES_ERROR;
}

#endif
