/**
 * @file bootloader.c
 * @author 0xDACC Team (github.com/0xDACC)
 * @brief  The secure bootloader which meets all the functional and security requirements.
 * 
 * @version 1.2
 * @date 2022-4-6
 * 
 * @copyright Copyright (c) 2022
 */

#include <stdint.h>
#include <stdbool.h>

#include "driverlib/interrupt.h"
#include "driverlib/eeprom.h"
#include "inc/hw_types.h"
#include "inc/hw_eeprom.h"
#include "inc/hw_sysctl.h"

#include "flash.h"
#include "uart.h"

#include "aes.h"


// Flash storage layout

/*
 * Firmware:
 *      Size:    0x0002B400 : 0x0002B404 (4B)
 *      Version: 0x0002B404 : 0x0002B408 (4B)
 *      Msg:     0x0002B408 : 0x0002BC00 (~2KB = 1KB + 1B + pad)
 *      Fw:      0x0002BC00 : 0x0002FC00 (16KB)
 *      FW pass: 0x0002FC00 : 0x0002FC0F (16B)
 * Configuration:
 *      Size:    0x0002FC0F : 0x00030000 (1KB = 4B + pad)
 *      Cfg:     0x00030000 : 0x00040000 (64KB)
 */
#define FIRMWARE_METADATA_PTR      ((uint32_t)(FLASH_START + 0x0002B400))
#define FIRMWARE_SIZE_PTR          ((uint32_t)(FIRMWARE_METADATA_PTR + 0))
#define FIRMWARE_VERSION_PTR       ((uint32_t)(FIRMWARE_METADATA_PTR + 4))
#define FIRMWARE_RELEASE_MSG_PTR   ((uint32_t)(FIRMWARE_METADATA_PTR + 8))
#define FIRMWARE_RELEASE_MSG_PTR2  ((uint32_t)(FIRMWARE_METADATA_PTR + FLASH_PAGE_SIZE))

#define FIRMWARE_STORAGE_PTR       ((uint32_t)(FIRMWARE_METADATA_PTR + (FLASH_PAGE_SIZE*2)))
#define FIRMWARE_BOOT_PTR          ((uint32_t)0x20004000)

#define CONFIGURATION_METADATA_PTR ((uint32_t)(FIRMWARE_STORAGE_PTR + (FLASH_PAGE_SIZE*16)))
// Note: This is 16 bytes offset from the reference design because we need to have the password for the firmware stored where this would be
#define CONFIGURATION_SIZE_PTR     ((uint32_t)(CONFIGURATION_METADATA_PTR + 16))

#define CONFIGURATION_STORAGE_PTR  ((uint32_t)(CONFIGURATION_METADATA_PTR + FLASH_PAGE_SIZE))

// Firmware update constants
#define FRAME_OK 0x00
#define FRAME_BAD 0x01

// EEPROM storage layout
/*
 * AES information:
 *      Key:     0x00000000 : 0x00000010 (16B) 
 *      IV:      0x00000010 : 0x00000020 (16B)
 *      Password 0x00000020 : 0x00000030 (16B)
 */

#define EEPROM_START_PTR        ((uint32_t)0x00000000)
#define KEY_OFFSET_PTR          ((uint32_t)EEPROM_START_PTR + 0)
#define IV_OFFSET_PTR           ((uint32_t)EEPROM_START_PTR + 16)
#define PASSWORD_OFFSET_PTR     ((uint32_t)EEPROM_START_PTR + 32)


// 32 bit arrays for reading from eeprom
uint32_t key32[4];
uint32_t iv32[4];
uint32_t password32[4];

// 8 bit arrays for use within the bootloader
uint8_t key[16];
uint8_t iv[16];
uint8_t password[16];

/**
 * @brief Boot the firmware.
 */
void handle_boot(void)
{
    uint32_t size;
    uint32_t i = 0;
    uint8_t *rel_msg;

    // Acknowledge the host
    uart_writeb(HOST_UART, 'B');    

    // Find the metadata
    size = *((uint32_t *)FIRMWARE_SIZE_PTR);
    
    // If the size is -1 then we know that there has not been a firmware installed, or something was tampered with
    if(size == 0xFFFFFFFF){
        //no firmware installed
        uart_writeb(HOST_UART, FRAME_BAD);
    }

    // move firmware to boot, but dont include the password
    for (i = 0; i < size-16; i++) {
        *((uint8_t *)(FIRMWARE_STORAGE_PTR + i)) = *((uint8_t *)(FIRMWARE_STORAGE_PTR + i));
    }

    // acknowledge host
    uart_writeb(HOST_UART, 'M');

    // Print the release message
    rel_msg = (uint8_t *)FIRMWARE_RELEASE_MSG_PTR;
    while (*rel_msg != 0) {
        uart_writeb(HOST_UART, *rel_msg);
        rel_msg++;
    }
    uart_writeb(HOST_UART, '\0'); // Null terminator...

    // Execute the firmware
    void (*firmware)(void) = (void (*)(void))(FIRMWARE_BOOT_PTR + 1);
    firmware();
}


/**
 * @brief Send the firmware data over the host interface. The host will decrypt it
 */
void handle_readback(void)
{
    uint8_t region;
    uint8_t *address;
    uint32_t size = 0;
    uint8_t pbuff[16];
    
    // Acknowledge the host
    uart_writeb(HOST_UART, 'R');

    // Read the password supplied by the host
    uart_read(HOST_UART, pbuff, 16);

    //Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // Check if the password supplied is the correct one
    for(int i = 0; i < 16; i++){
        if(pbuff[i] != password[i]){
            //incorrect or invalid password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    //Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // Receive region identifier
    region = (uint32_t)uart_readb(HOST_UART);

    // Set base address for readback
    if (region == 'F') {
        // Set the base address for the readback
        address = (uint8_t *)FIRMWARE_STORAGE_PTR;
        // Acknowledge the host
        uart_writeb(HOST_UART, 'F');
    } else if (region == 'C') {
        // Set the base address for the readback
        address = (uint8_t *)CONFIGURATION_STORAGE_PTR;
        // Acknowledge the host
        uart_writeb(HOST_UART, 'C');
    } else {
        uart_writeb(HOST_UART, 'Q');
        return;
    }

    // Receive the size to send back to the host
    size = ((uint32_t)uart_readb(HOST_UART)) << 24;
    size |= ((uint32_t)uart_readb(HOST_UART)) << 16;
    size |= ((uint32_t)uart_readb(HOST_UART)) << 8;
    size |= (uint32_t)uart_readb(HOST_UART);

    // Now we want to protect against readback overreach, meaning that we overreach what we really should be reading back. You asked for Fw, were ONLY going to give you FW
    // Read out the data

    if(region == 'F'){
        if(size <= 16385){
            // its not in danger of any overreach
            uart_write(HOST_UART, (uint8_t *)address, size);
        } else {
            // its in danger of overreach, so we will fill the potential overreach with 0xFF
            uart_write(HOST_UART, (uint8_t *)address, 16385);
            // for the remaining bytes we send 0xFF
            for(int i = 0; i < size - 16385; i++){
                uart_writeb(HOST_UART, 0xFF);
            }
        }
    } else {
        if(size <= 65536){
            // its not in danger of any overreach
            uart_write(HOST_UART, (uint8_t *)address, size);
        } else {
            // its in danger of overreach, so we will fill the potential overreach with 0xFF
            uart_write(HOST_UART, (uint8_t *)address, 65536);
            // for the remaining bytes we send 0xFF
            for(int i = 0; i < size - 65536; i++){
                uart_writeb(HOST_UART, 0xFF);
            }
        }
    }
    
}

/**
 * @brief Read data from a UART interface and program to flash memory.
 * 
 * @param interface is the base address of the UART interface to read from.
 * @param dst is the starting page address to store the data.
 * @param size is the number of bytes to load.
 */
void load_data(uint32_t interface, uint32_t dst, uint32_t size)
{
    int i;
    uint32_t frame_size;
    uint8_t page_buffer[FLASH_PAGE_SIZE];

    while(size > 0) {
        // calculate frame size
        frame_size = size > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : size;
        // read frame into buffer
        uart_read(HOST_UART, page_buffer, frame_size);
        // pad buffer if frame is smaller than the page
        for(i = frame_size; i < FLASH_PAGE_SIZE; i++) {
            page_buffer[i] = 0xFF;
        }
        // clear flash page
        flash_erase_page(dst);
        // write flash page
        flash_write((uint32_t *)page_buffer, dst, FLASH_PAGE_SIZE >> 2);
        // next page and decrease size
        dst += FLASH_PAGE_SIZE;
        size -= frame_size;
        // send frame ok
        uart_writeb(HOST_UART, FRAME_OK);
    }
}

/**
 * @brief Update the firmware.
 */
void handle_update(void)
{
    uint8_t vbuff[32];  // 8 bit array that will hold the version for decryption
    uint8_t pbuff[16];
    uint32_t version = 0;
    uint32_t size = 0;
    uint32_t rel_msg_size = 0;
    uint8_t rel_msg[1025]; // 1024 + terminator

    // Acknowledge the host
    uart_writeb(HOST_UART, 'U');
    // Receive version, store in buffer for decryption
    uart_read(HOST_UART, vbuff, 32);
    // Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // Receive size
    size = ((uint32_t)uart_readb(HOST_UART)) << 24;
    size |= ((uint32_t)uart_readb(HOST_UART)) << 16;
    size |= ((uint32_t)uart_readb(HOST_UART)) << 8;
    size |= (uint32_t)uart_readb(HOST_UART);

    // get the size of the release message
    rel_msg_size = uart_readline(HOST_UART, rel_msg) + 1; // Include terminator

    // Decrypt the version number
    struct AES_ctx version_ctx;
    AES_init_ctx_iv(&version_ctx, key, iv);
    AES_CBC_decrypt_buffer(&version_ctx, vbuff, 32);

    // Check for password
    for(int i = 0; i<16; i++){
       if (password[i] != vbuff[16+i]){
            // Version Number is not signed with the correct password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // If it not an acceptable number then return and quit
    if ((version != 0) && (version < *(uint32_t *)FIRMWARE_VERSION_PTR)) {
        // Version is not acceptable
        uart_writeb(HOST_UART, FRAME_BAD);
        return;
    }

    // since the version can actually take up to 16 bits we need to turn the two 8 bit bytes into one larger byte
    version = vbuff[0] << 8;
    version |= vbuff[1];

    //Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // get first password frame
    uart_read(HOST_UART, pbuff, 16);

    // Decrypt password
    struct AES_ctx firstpass_ctx;
    AES_init_ctx_iv(&firstpass_ctx, key, iv);
    AES_CBC_decrypt_buffer(&firstpass_ctx, pbuff, 16);

    // check password
    for(int i = 0; i<16; i++){
       if (password[i] != pbuff[i]){
            // incorrect password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // acknowledge host
    uart_writeb(HOST_UART, FRAME_OK);

    // send crypto for use by host
    uart_write(HOST_UART, key, 16);
    uart_write(HOST_UART, iv, 16);

    // recieve the decrypted ending password and double check
    uart_read(HOST_UART, pbuff, 16);

    // check password
    for(int i = 0; i<16; i++){
       if (password[i] != pbuff[i]){
           // wrong password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // acknowledge host
    uart_writeb(HOST_UART, FRAME_OK);

    // Clear firmware metadata
    flash_erase_page(FIRMWARE_METADATA_PTR);

    //load firmware
    load_data(HOST_UART, FIRMWARE_STORAGE_PTR, size-32);

    size -= 32;

    // Only save new version if it is not 0
    if(version != 0){
        flash_write_word(version, FIRMWARE_VERSION_PTR);
    }
    
    //write message
    flash_write((uint32_t *)rel_msg, FIRMWARE_RELEASE_MSG_PTR, FLASH_PAGE_SIZE >> 2);

    // Write release message
    uint8_t *rel_msg_read_ptr = rel_msg;
    uint32_t rel_msg_write_ptr = FIRMWARE_RELEASE_MSG_PTR;
    uint32_t rem_bytes = rel_msg_size;

    // If release message goes outside of the first page, write the first full page
    if (rel_msg_size > (FLASH_PAGE_SIZE-8)) {

        // Write first page
        flash_write((uint32_t *)rel_msg, FIRMWARE_RELEASE_MSG_PTR, (FLASH_PAGE_SIZE-8) >> 2); // This is always a multiple of 4

        // Set up second page
        rem_bytes = rel_msg_size - (FLASH_PAGE_SIZE-8);
        rel_msg_read_ptr = rel_msg + (FLASH_PAGE_SIZE-8);
        rel_msg_write_ptr = FIRMWARE_RELEASE_MSG_PTR2;
        flash_erase_page(rel_msg_write_ptr);
    }

    // Program last or only page of release message
    if (rem_bytes % 4 != 0) {
        rem_bytes += 4 - (rem_bytes % 4); // Account for partial word
    }
    flash_write((uint32_t *)rel_msg_read_ptr, rel_msg_write_ptr, rem_bytes >> 2);

    // acknowledge host
    uart_writeb(HOST_UART, FRAME_OK);
}

/**
 * @brief Load configuration data.
 */
void handle_configure(void)
{
    uint32_t size = 0;
    uint8_t pbuff[16];

    // Acknowledge the host
    uart_writeb(HOST_UART, 'C');

    // Receive size
    size = (((uint32_t)uart_readb(HOST_UART)) << 24);
    size |= (((uint32_t)uart_readb(HOST_UART)) << 16);
    size |= (((uint32_t)uart_readb(HOST_UART)) << 8);
    size |= ((uint32_t)uart_readb(HOST_UART));

    // Acknowledge the host
    uart_writeb(HOST_UART, FRAME_OK);

    // Recieve first password to check
    uart_read(HOST_UART, pbuff, 16);

    // Decrypt password
    struct AES_ctx firstpass_ctx;
    AES_init_ctx_iv(&firstpass_ctx, key, iv);
    AES_CBC_decrypt_buffer(&firstpass_ctx, pbuff, 16);

    // check password
    for(int i = 0; i < 16; i++){
       if (password[i] != pbuff[i]){
            // Version Number is not signed with the correct password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // send crypto for host use
    uart_write(HOST_UART, key, 16);
    uart_write(HOST_UART, iv, 16);

    // recieve the decrypted ending password and double check
    uart_read(HOST_UART, pbuff, 16);

    // check password
    for(int i = 0; i < 16; i++){
       if (password[i] != pbuff[i]){
            // Version Number is not signed with the correct password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // acknowledge host
    uart_writeb(HOST_UART, FRAME_OK);

    // send crypto for host use
    uart_write(HOST_UART, key, 16);
    uart_write(HOST_UART, iv, 16);

    // Second password!
    uart_read(HOST_UART, pbuff, 16);

    // check password
    for(int i = 0; i < 16; i++){
       if (password[i] != pbuff[i]){
            // Version Number is not signed with the correct password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // Clear firmware metadata
    flash_erase_page(CONFIGURATION_METADATA_PTR);

    //load firmware
    load_data(HOST_UART, CONFIGURATION_STORAGE_PTR, size-32);

    // and after all that we will know the true size of the config, so we can now write it
    flash_write_word(size, CONFIGURATION_SIZE_PTR);
}

/**
 * @brief Host interface polling loop to receive configure, update, readback,
 * and boot commands.
 * 
 * @return int
 */
int main(void){
    // Setting up the eeprom
    HWREG(SYSCTL_RCGCEEPROM) |= SYSCTL_RCGCEEPROM_R0;
    while (!HWREG(SYSCTL_PREEPROM));
    EEPROMInit();

    // Side note: Would not have figured out that part above without the organizers. Thanks Jake!

    // Reading from eeprom to the 32 bit arrays
    EEPROMRead(key32, (uint32_t)KEY_OFFSET_PTR, 16);
    EEPROMRead(iv32, (uint32_t)IV_OFFSET_PTR, 16);
    EEPROMRead(password32, (uint32_t)PASSWORD_OFFSET_PTR, 16);

    // Convert those 4 bytes of 32 bits into 16 bytes of 8 bits
    // There has to be an easier way to do this LOL
    key[0] = key32[0];
    key[1] = key32[0] >> 8;
    key[2] = key32[0] >> 16;
    key[3] = key32[0] >> 24;

    key[4] = key32[1];
    key[5] = key32[1] >> 8;
    key[6] = key32[1] >> 16;
    key[7] = key32[1] >> 24;

    key[8] = key32[2];
    key[9] = key32[2] >> 8;
    key[10] = key32[2] >> 16;
    key[11] = key32[2] >> 24;

    key[12] = key32[3];
    key[13] = key32[3] >> 8;
    key[14] = key32[3] >> 16;
    key[15] = key32[3] >> 24;

    iv[0] = iv32[0];
    iv[1] = iv32[0] >> 8;
    iv[2] = iv32[0] >> 16;
    iv[3] = iv32[0] >> 24;

    iv[4] = iv32[1];
    iv[5] = iv32[1] >> 8;
    iv[6] = iv32[1] >> 16;
    iv[7] = iv32[1] >> 24;

    iv[8] = iv32[2];
    iv[9] = iv32[2] >> 8;
    iv[10] = iv32[2] >> 16;
    iv[11] = iv32[2] >> 24;

    iv[12] = iv32[3];
    iv[13] = iv32[3] >> 8;
    iv[14] = iv32[3] >> 16;
    iv[15] = iv32[3] >> 24;

    password[0] = password32[0];
    password[1] = password32[0] >> 8;
    password[2] = password32[0] >> 16;
    password[3] = password32[0] >> 24;

    password[4] = password32[1];
    password[5] = password32[1] >> 8;
    password[6] = password32[1] >> 16;
    password[7] = password32[1] >> 24;

    password[8] = password32[2];
    password[9] = password32[2] >> 8;
    password[10] = password32[2] >> 16;
    password[11] = password32[2] >> 24;

    password[12] = password32[3];
    password[13] = password32[3] >> 8;
    password[14] = password32[3] >> 16;
    password[15] = password32[3] >> 24;

    uint8_t cmd = 0;

    // Memory is always initialized as 1s, so on the first startup we need to set the current version to the oldest as defined by the host
    uint32_t current_version = *(uint32_t *)FIRMWARE_VERSION_PTR;
    if (current_version == 0xFFFFFFFF){
        flash_write_word((uint32_t)OLDEST_VERSION, FIRMWARE_VERSION_PTR);
    }
    
    // Initialize IO components
    uart_init();

    // Handle host commands
    while (1) {
        cmd = uart_readb(HOST_UART);

        switch (cmd) {
        case 'C':
            handle_configure();
            break;
        case 'U':
            handle_update();
            break;
        case 'R':
            handle_readback();
            break;
        case 'B':
            handle_boot();
            break;
        default:
            break;
        }
    }
}
