# SAFFIRe Bootloader
The SAFFIRe bootloader is the heart and soul of this years challenge, and as such deserves a respectable readme! In this file we will go over each function in the bootloader in a nice neat fashion that is easy to read!

## Firmware Update
1. Negotiate with the host to enter update mode
2. Read the encrypted version from the host
3. Read the size of the firmware from the host
4. Read the release message from the host
5. Decrypt the version for authentication
   *If the version or the authentication password is invalid the bootloader rejects the update*
6. Load the firmware
7. Decrypt the firmware for storage
   *If the authentication password is invalid or not present then the bootloader rejects the update*
8. Encrypt firmware again
9. Write encrypted firmware to flash
10. Write size to flash
11. Write version to flash
12. Write release message to flash

## Firmware Configuration
1. Negotiate with the host to enter configure mode
2. Read the config size from the host
3. Read the configuration from the host
4. Decrypt the configuration for authentication
   *If the authentication password is invalid or not present then the bootloader rejects the configuration*
5. Save configuration to flash
6. Save size to flash
*Note: Config files are stored and encrypted in 1KB + 16B blocks, so this process is repeated until the whole file has been read*

## Readback
Firmware readback and configuration readback are based in one function in the bootloader, and the bootloader sets the address we need to read from for either the firmware of congfiguration based on what the host tells it during the negotiation phase

1. Negotiate with the host to enter readback mode and determin the address to read from 
2. Wait for the authentication password to be supplied
   *If the incorrect password is supplied, then the readback is rejected*
3. Send requested data over uart. The data is capped to a certain size based off if the host requests firmware or config data.

## Boot
1. Negotiate with the host to enter boot mode
2. Copy the encrypted flash to RAM
3. Decrypt this section of RAM
   *If the authentication password is invalid or not present then the bootloader rejects the boot*
4. Move data to Boot section of RAM
5. Write the release message over uart
6. Boot the firmware