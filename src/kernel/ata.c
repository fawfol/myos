#include "ata.h"
#include "io.h"
#include "shell.h"

#define ATA_DATA 0x1F0
#define ATA_SECCOUNT0 0x1F2
#define ATA_LBA0 0x1F3
#define ATA_LBA1 0x1F4
#define ATA_LBA2 0x1F5
#define ATA_HDDEVSEL 0x1F6
#define ATA_COMMAND 0x1F7
#define ATA_STATUS 0x1F7

#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30

#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08

void ata_init()
{
}
void ata_read_sector(uint32_t lba, uint8_t* buffer)
{
    uint8_t status;
    int timeout = 1000000;
    while ((inb(ATA_STATUS) & ATA_SR_BSY) && timeout--);

    if (timeout <= 0)
    {
        terminal_print("ATA timeout\n");
        return;
    }

    // select drive
    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));

    // 400ns delay
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);

    //sector count
    outb(ATA_SECCOUNT0, 1);

    // LBA address
    outb(ATA_LBA0, (uint8_t)(lba));
    outb(ATA_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_LBA2, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ);
    //timer reset
    timeout = 1000000;

    // wait for DRQ
    do {
        status = inb(ATA_STATUS);
    } while (!(status & ATA_SR_DRQ) && timeout--);

    if (timeout <= 0)
    {
        terminal_print("ATA DRQ timeout\n");
        return;
    }

    // read 256 words (512 bytes)
    for (int i = 0; i < 256; i++)
    {
        uint16_t data = inw(ATA_DATA);
        buffer[i*2] = data & 0xFF;
        buffer[i*2 + 1] = data >> 8;
    }
}

void ata_write_sector(uint32_t lba, uint8_t* buffer)
{
    uint8_t status;
    int timeout = 1000000;
    while ((inb(ATA_STATUS) & ATA_SR_BSY) && timeout--);

    //select drive and LBA
    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0xF));
    outb(ATA_SECCOUNT0, 1);
    outb(ATA_LBA0, (uint8_t)(lba));
    outb(ATA_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_LBA2, (uint8_t)(lba >> 16));

    // Send Write Command
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    timeout = 1000000;
    do {
        status = inb(ATA_STATUS);
    } while (!(status & ATA_SR_DRQ) && timeout--);

    if (timeout <= 0) {
        terminal_print("ATA Error: Write DRQ timeout\n");
        return;
    }

    //blast the data into the drivess buffer
    for (int i = 0; i < 256; i++)
    {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        outw(ATA_DATA, data);
    }

    //send the Cache Flush command so the drive saves it physically right now
    outb(ATA_COMMAND, 0xE7);
    
    //wait for the flush to finish
    timeout = 1000000;
    while ((inb(ATA_STATUS) & ATA_SR_BSY) && timeout--);
}
