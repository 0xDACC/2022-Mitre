# SAFFIRe Bootloader
The SAFFIRe bootloader is the heart and soul of this years challenge, and as such deserves a respectable readme! In this file we will go over each function in the bootloader in a nice neat fashion that is easy to read!

## Firmware Update
1. Negotiate with the host to enter update mode
2. Read the encrypted version from the host
3. Read the size of the firmware from the host
4. Read the release message from the host
5. Decrypt the version for authentication
   *If the version or the authentication password is invalid the bootloader rejects the update*
6. Host sends first 16 bytes of encrypted firmware (the first authentication password) to bootloader
7. Bootloader decrypts and confirms this authentication password
8. If the password is correct, the bootloader sends over the cryptographic key and iv to the host
9. Host decrypts firmware
10. Host sends last 16 bytes of decrypted firmware (the second authentication password) for confirmation from bootloader
11. If the bootloader confirms this password, then the host sends the now unencrypted firmware to the bootloader for installation

## Firmware Configuration
1. Negotiate with the host to enter configure mode
2. Read the config size from the host
3. Host sends first 16 bytes of encrypted config (the first authentication password) to bootloader
4. Bootloader decrypts and confirms this authentication password
5. If the password is correct, the bootloader sends over the cryptographic key and iv to the host
6. Host decrypts config
7. Host sends last 16 bytes of decrypted config (the second authentication password) for confirmation from bootloader
8. If the bootloader confirms this password, then the host sends the now unencrypted configuration to the bootloader for installation

## Readback
Firmware readback and configuration readback are based in one function in the bootloader, and the bootloader sets the address we need to read from for either the firmware of congfiguration based on what the host tells it during the negotiation phase

1. Negotiate with the host to enter readback mode and determin the address to read from 
2. Wait for the authentication password to be supplied
   *If the incorrect password is supplied, then the readback is rejected*
3. Send requested data over uart. The data is capped to a certain size based off if the host requests firmware or config data.

## Boot
1. Negotiate with the host to enter boot mode
2. Move data to Boot section of RAM
3. Write the release message over uart
4. Boot the firmware