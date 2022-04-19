# Host Tools
This stuff is pretty jank, so we are going to try to explain it as well as possible...

There is also an obscene amount of comments throughout all the code as well

## Boot
1. Connect to bootloader
2. Negotiate with bootloader to enter boot mode
3. Wait for bootloader to boot FW
4. Receive release message
5. Write release message to file
6. Profit

## CFG Protect
1. Read raw CFG binary
2. Read crypto information from host secrets
3. Pad data to make it a multiple of 16
4. Add authentication password to beginning and end of data
5. Encrypt padded and signed data

## CFG Load
1. Negotiate with the host to enter configure mode
2. Read the config size from the host
3. Host sends first 16 bytes of encrypted config (the first authentication password) to bootloader
4. Bootloader decrypts and confirms this authentication password
5. If the password is correct, the bootloader sends over the cryptographic key and iv to the host
6. Host decrypts config
7. Host sends last 16 bytes of decrypted config (the second authentication password) for confirmation from bootloader
8. If the bootloader confirms this password, then the host sends the now unencrypted configuration to the bootloader for installation

## FW Protect
1. Read in raw FW Binary
2. Read crypto information
3. Pad firmware to be compatably with AES-128 bit
4. Add authentication password to beginning and end
5. Encrypt
6. Perform logic on capping off version number
7. Pad version number
8. Add authentication password to version number
9. Pack firmware size, version number, release message, and firmware data into JSON file
10. Write out file

## Fw Update
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

## Readback
1. Negotiate with bootloader to enter readback mode
2. Send password to bootloader for authentication
3. If password is correct continue
4. Send the region of data we want (Firmware or config)
5. Send number of bytes of data we want
6. Recieve firmare data (Note: We will not recieve data that is not in the region we requested, even if we ask for more bytes of data. E.G. if the firmware is only 127 bytes, and we ask for 300 bytes of data, we will only recieve 127 bytes of real data, and the rest will be blank bytes.)
