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
3. Pad data to make it a multiple of 1024
4. Split data into 1KB chunks
5. Add authentication password to end of each chunk
6. Encrypt each chunk individually
7. Combine into 1 protected data package
8. Write out to file

## CFG Load
1. Connect to bootloader
2. Negotiate with bootloader to enter configure mode
3. Send CFG size
4. Send over the CFG data in 1KB + 16B blocks (1KB of data and 16B of authentication data)
5. Rinse and repeat until the whole file has been sent and checked for authenticity

## FW Protect
1. Read in raw FW Binary
2. Read crypto information
3. Pad firmware to be compatably with AES-128 bit
4. Add authentication password to end
5. Encrypt
6. Perform logic on capping off version number
7. Pad version number
8. Add authentication password to version number
9. Pack firmware size, version number, release message, and firmware data into JSON file
10. Write out file

## Fw Update
1. Negotiate with bootloader to enter update mode
2. Send version number, and wait for authentication
3. Send firmware size
4. Send release message
5. Send firmware data, and wait for authentication

## Readback
1. Negotiate with bootloader to enter readback mode
2. Send password to bootloader for authentication
3. If password is correct continue
4. Send the region of data we want (Firmware or config)
5. Send number of bytes of data we want
6. Recieve firmare data (Note: We will not recieve data that is not in the region we requested, even if we ask for more bytes of data. E.G. if the firmware is only 127 bytes, and we ask for 300 bytes of data, we will only recieve 127 bytes of real data, and the rest will be blank bytes.)
