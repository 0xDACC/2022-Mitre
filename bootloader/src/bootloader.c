/**
 * @file bootloader.c
 * @author 0xDACC Team (github.com/0xDACC)
 * @brief  The secure bootloader which meets all the functional and security requirements
 * @version 1.0
 * @date 2022-03-20
 * 
 * @copyright Copyright (c) 2022
 * 
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
 * Configuration:
 *      Size:    0x0002FC00 : 0x0003000 (1KB = 4B + pad)
 *      Cfg:     0x00030000 : 0x0004000 (64KB)
 */
#define FIRMWARE_METADATA_PTR      ((uint32_t)(FLASH_START + 0x0002B400))
#define FIRMWARE_SIZE_PTR          ((uint32_t)(FIRMWARE_METADATA_PTR + 0))
#define FIRMWARE_VERSION_PTR       ((uint32_t)(FIRMWARE_METADATA_PTR + 4))
#define FIRMWARE_RELEASE_MSG_PTR   ((uint32_t)(FIRMWARE_METADATA_PTR + 8))
#define FIRMWARE_RELEASE_MSG_PTR2  ((uint32_t)(FIRMWARE_METADATA_PTR + FLASH_PAGE_SIZE))

#define FIRMWARE_STORAGE_PTR       ((uint32_t)(FIRMWARE_METADATA_PTR + (FLASH_PAGE_SIZE*2)))
#define FIRMWARE_BOOT_PTR          ((uint32_t)0x20003FEC)

#define CONFIGURATION_METADATA_PTR ((uint32_t)(FIRMWARE_STORAGE_PTR + (FLASH_PAGE_SIZE*16)))
#define CONFIGURATION_SIZE_PTR     ((uint32_t)(CONFIGURATION_METADATA_PTR + 0))

#define CONFIGURATION_STORAGE_PTR  ((uint32_t)(CONFIGURATION_METADATA_PTR + FLASH_PAGE_SIZE))

// Firmware update constants
#define FRAME_OK 0x00
#define FRAME_BAD 0x01

// EEPROM storage layout
/*
 * AES information:
 *      Key:     0x00000000 : 0x0000000F (16B) 
 *      IV:      0x00000010 : 0x0000001F (16B)
 */

#define EEPROM_START_PTR        ((uint32_t)0x00000000)
#define KEY_OFFSET_PTR          ((uint32_t)EEPROM_START_PTR + 0)
#define IV_OFFSET_PTR           ((uint32_t)EEPROM_START_PTR + 16)
#define PASSWORD_OFFSET_PTR     ((uint32_t)EEPROM_START_PTR + 32)

//Latest point where we can store 16kb + 16b of data for loading
#define FW_RAM_START_PTR        ((uint32_t)0x20003FEC)

// where we will hold cfg in memory for decryption. I hate this so much
#define CFG_RAM_START_PTR       ((uint32_t)0x0000ffff)

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
    
    // If the size is -1 then we know that there has not been a firmware installed
    if(size == 0xFFFFFFFF){
        //no firmware installed
        uart_writeb(HOST_UART, FRAME_BAD);
    }

    // Copy the firmware into the Boot RAM section
    for (i = 0; i < size; i++) {
        *((uint8_t *)(FIRMWARE_BOOT_PTR + i)) = *((uint8_t *)(FIRMWARE_STORAGE_PTR + i));
    }

    // Decrypt
    struct AES_ctx firmware_ctx;
    AES_init_ctx_iv(&firmware_ctx, key, iv);
    AES_CBC_decrypt_buffer(&firmware_ctx, (uint8_t *)(FIRMWARE_BOOT_PTR), size);

    // check password
    for(i = 0; i < 16; i++){
        if(*((uint8_t *)(FIRMWARE_BOOT_PTR + (size-16) + i)) != password[i]){
            // password is incorrect, so the firmware was tampered with
            uart_writeb(HOST_UART, FRAME_BAD);
        }
    }

    // acknowledge host
    uart_writeb(HOST_UART, 'M');

    // Print the release message
    rel_msg = (uint8_t *)FIRMWARE_RELEASE_MSG_PTR;
    while (*rel_msg != 0) {
        uart_writeb(HOST_UART, *rel_msg);
        rel_msg++;
    }
    uart_writeb(HOST_UART, '\0');

    // Execute the firmware
    void (*firmware)(void) = (void (*)(void))(FIRMWARE_BOOT_PTR + 1);
    firmware();
}


/**
 * @brief Send the firmware data over the host interface.
 * 
 * again using the firmware boot ptr as a buffer location
 */
void handle_readback(void)
{
    uint8_t region;
    uint8_t *address;
    uint32_t fsize = 0;
    uint32_t size = 0;
    uint8_t pbuff[16];
    
    // Acknowledge the host
    uart_writeb(HOST_UART, 'R');

    // Read the password supplied by the host
    uart_read(HOST_UART, pbuff, 16);

    //Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // Check is the password supplied is the correct one
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
        fsize = *((uint32_t *)FIRMWARE_SIZE_PTR);
        address = (uint8_t *)FIRMWARE_STORAGE_PTR;
        // Acknowledge the host
        uart_writeb(HOST_UART, 'F');
    } else if (region == 'C') {
        // Set the base address for the readback
        fsize = *((uint32_t *)CONFIGURATION_SIZE_PTR);
        address = (uint8_t *)CONFIGURATION_STORAGE_PTR;
        // Acknowledge the hose
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

    if(region == 'C'){
        // Read out the data
        uart_write(HOST_UART, address, size);
        // and exit because the rest of this function will break if it tries to do it on 64k of unencrypted config data
        return;
    }

    // setting buffer to whatever is bigger so we dont see memory we dont need to see
    uint32_t buffsize = 0;
    if(size > fsize){
        buffsize = size;
    } else {
        buffsize = fsize;
    }

    // buffer for the data
    //uint8_t readback_buffer[buffsize];

    // Fill the buffer
    for(int i = 0; i < buffsize; i++){
        if(i < fsize-16){
            *((uint8_t *)(FW_RAM_START_PTR + i)) = address[i];
        } else {
            *((uint8_t *)(FW_RAM_START_PTR + i)) = 0xFF;
        }
        
    }

    // Since the config isnt encrytped (because really we cant tell the firmware how to decrypt it) we only want to decrypt the firmware
    if(region == 'F'){
        // Decrypt
        struct AES_ctx readback_ctx;
        AES_init_ctx_iv(&readback_ctx, key, iv);
        AES_CBC_decrypt_buffer(&readback_ctx, (uint8_t *)(FW_RAM_START_PTR), fsize-16);
    }

    // Read out the data
    uart_write(HOST_UART, (uint8_t *)(FW_RAM_START_PTR), size);
}

/**
 * @brief Load the firmware into flash storage
 * 
 * @param interface is the host uart to read from
 * @param size is the ammount of bytes to read
 */
void load_firmware(uint32_t interface, uint32_t size){
    int i;
    int j;
    uint32_t frame_size;
    uint8_t page_buffer[FLASH_PAGE_SIZE];
    uint32_t dst = FIRMWARE_STORAGE_PTR;
    uint32_t pos = 0;
    int32_t remaining = size;

    // Fill the firmware buffer
    while(remaining > 0) {
        // calculate frame size
        frame_size = remaining > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : remaining;
        // read frame into buffer
        uart_read(HOST_UART, page_buffer, frame_size);
        // pad buffer if frame is smaller than the page
        for(i = frame_size; i < FLASH_PAGE_SIZE; i++) {
            page_buffer[i] = 0xFF;
        }
        // add the page buffer to the firmware buffer
        for(j = 0; j < frame_size; j++){
            *((uint8_t *)(FW_RAM_START_PTR + j + pos)) = page_buffer[j];
        }
        pos += FLASH_PAGE_SIZE;
        remaining -= frame_size;
        j = 0;

        // Acknowledge the host
        uart_writeb(HOST_UART, FRAME_OK);

    }

    // Decrypt
    struct AES_ctx firmware_ctx;
    AES_init_ctx_iv(&firmware_ctx, key, iv);
    AES_CBC_decrypt_buffer(&firmware_ctx, (uint8_t *)(FW_RAM_START_PTR), size);

    // Check signature
    for(i = 0; i < 16; i++){
        if(password[i] != *((uint8_t *)(FW_RAM_START_PTR + ((size)-16) +i ))){
            // Firmware is not signed with the correct password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    // encrypt again for storage on the flash
    struct AES_ctx refirmware_ctx;
    AES_init_ctx_iv(&refirmware_ctx, key, iv);
    AES_CBC_encrypt_buffer(&refirmware_ctx, (uint8_t *)(FW_RAM_START_PTR), size);
    
    remaining = size;
    pos = 0;

    // Save size
    flash_write_word(size, FIRMWARE_SIZE_PTR);

    // Write firmware to flash
    while(remaining > 0) {
        // calculate frame size
        frame_size = remaining > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : remaining;
        // clear flash page
        flash_erase_page(dst);
        // write flash page
        flash_write((uint32_t *)(FW_RAM_START_PTR + pos), dst, FLASH_PAGE_SIZE >> 2);
        // next page and decrease size
        dst += FLASH_PAGE_SIZE;
        remaining -= frame_size;
        pos += frame_size;
    }
}

/**
 * @brief Update the firmware.
 */
void handle_update(void)
{
    int i;
    uint8_t vbuff[32];  // 8 bit array that will hold the version for decryption
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

    // Decrypt the version number, and put it in a 32 bit unsigned int
    struct AES_ctx version_ctx;
    AES_init_ctx_iv(&version_ctx, key, iv);
    AES_CBC_decrypt_buffer(&version_ctx, vbuff, 32);

    // since the version can actually take up to 16 bits we need to turn the two 8 bit bytes into one larger byte
    version = vbuff[0] << 8;
    version |= vbuff[1];

    // Check for password
   for(i = 0; i<16; i++){
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

    //Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);

    // Clear firmware metadata
    flash_erase_page(FIRMWARE_METADATA_PTR);

    //load firmware
    load_firmware(HOST_UART, size);

    // Only save new version if it is not 0
    if (version != 0) {
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

    //Acknowledge
    uart_writeb(HOST_UART, FRAME_OK);
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
 * @brief Load configuration data.
 * 
 * We repurpose the firmware boot pointer as a buffer for holding the loading data, as it tries to allocate invalid memory addresses automatically
 */
void handle_configure(void)
{
    int i;
    //int j;
    uint32_t size = 0;
    //uint32_t frame_size;
    uint8_t page_buffer[FLASH_PAGE_SIZE];
    uint8_t pass_buffer[16];
    uint8_t frame_buffer[1030];
    uint32_t dst = CONFIGURATION_STORAGE_PTR;
    //uint32_t pos = 0;
    int32_t remaining;
    uint8_t badpass = 0;

    // Acknowledge the host
    uart_writeb(HOST_UART, 'C');

    // Receive size
    size = (((uint32_t)uart_readb(HOST_UART)) << 24);
    size |= (((uint32_t)uart_readb(HOST_UART)) << 16);
    size |= (((uint32_t)uart_readb(HOST_UART)) << 8);
    size |= ((uint32_t)uart_readb(HOST_UART));

    remaining = size;   

    // Acknowledge the host
    uart_writeb(HOST_UART, FRAME_OK);

    // recieve the frames, receive the password, combine, decrypt, store in flash, rinse and repeat
    while(remaining > 0){
        // Data frame
        uart_read(HOST_UART, page_buffer, FLASH_PAGE_SIZE);

        // password frame
        uart_read(HOST_UART, pass_buffer, 16);

        //combine
        for(i = 0; i < FLASH_PAGE_SIZE; i++){
            frame_buffer[i] = page_buffer[i];
        }
        for(i = 0; i < 16; i++){
            frame_buffer[i+FLASH_PAGE_SIZE] = pass_buffer[i];
        }

        // Decrypt
        struct AES_ctx config_ctx;
        AES_init_ctx_iv(&config_ctx, key, iv);
        AES_CBC_decrypt_buffer(&config_ctx, frame_buffer, 1040);

        //check password
        for(i = 0; i < 16; i++){
            if(page_buffer[i+FLASH_PAGE_SIZE] != password[i]){
                // Bad password
                uart_writeb(HOST_UART, FRAME_BAD);
                badpass = 0xFF;
            }
        }

        // If the config was bad we want to erase anything we've written so we dont have ANY malicious data in there
        if(badpass != 0){
            for(i = 0; i < 64; i++){
                flash_erase_page(CONFIGURATION_STORAGE_PTR+(i*FLASH_PAGE_SIZE));
            }
            return;
        }

        // write this frame to the flash
        flash_erase_page(dst);
        flash_write((uint32_t *)page_buffer, dst, FLASH_PAGE_SIZE >> 2);

        dst += FLASH_PAGE_SIZE;
        remaining -= 1030;

        //acknowledge
        uart_writeb(HOST_UART, FRAME_OK);
    }

    /*
    // Fill the config buffer
    while(remaining > 0) {
        // calculate frame size
        frame_size = remaining > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : remaining;
        // read frame into buffer
        uart_read(HOST_UART, page_buffer, frame_size);
        // pad buffer if frame is smaller than the page
        for(i = frame_size; i < FLASH_PAGE_SIZE; i++) {
            page_buffer[i] = 0xFF;
        }
        // add the page buffer to the config buffer
        for(j = 0; j < frame_size; j++){
            *((uint8_t *)CFG_RAM_START_PTR + j + pos) = page_buffer[j];
        }
        pos += FLASH_PAGE_SIZE;
        remaining -= frame_size;
        j = 0;

        // Acknowledge the host
        uart_writeb(HOST_UART, FRAME_OK);
    }

    // Decrypt
    struct AES_ctx config_ctx;
    AES_init_ctx_iv(&config_ctx, key, iv);
    AES_CBC_decrypt_buffer(&config_ctx, (uint8_t *)(CFG_RAM_START_PTR), size);

    // Check signature
    for(i = 0; i < 16; i++){
        if(password[i] != *((uint8_t *)(CFG_RAM_START_PTR + ((size)-16) +i ))){
            // Firmware is not signed with the correct password
            uart_writeb(HOST_UART, FRAME_BAD);
            return;
        }
    }

    //clear config before writing
    flash_erase_page(CONFIGURATION_METADATA_PTR);

    // Save size to the flash
    flash_write_word(size, CONFIGURATION_SIZE_PTR);

    remaining = size;
    pos = 0;

    // Write firmware to flash
    while(remaining > 0) {
        // calculate frame size
        frame_size = remaining > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : remaining;
        // clear flash page
        flash_erase_page(dst);
        // write flash page
        flash_write((uint32_t *)(CFG_RAM_START_PTR + pos), dst, FLASH_PAGE_SIZE >> 2);
        // next page and decrease size
        dst += FLASH_PAGE_SIZE;
        remaining -= frame_size;
        pos += frame_size;
    }

    // Acknowledge the host
    uart_writeb(HOST_UART, FRAME_OK);

    */
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
