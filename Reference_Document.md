# Reference Document
Throughout history, for as long as there have been programs, programmers, and documentation for those programs, there has been one phrase uttered at least once by every programmer: *"This code is so good it documents itself!"*

Now we all know better than to believe that our code *truly* documents itself - Some explination is always needed. Throughout the course of development for this particular project the need to provide *good* documentation was always in the back of our minds. Therefor in addition to this reference guide, there is a getting started guide, and a readme file for the host tools and for the bootloader. Hopefully this shall be enough for you, dear reader, to wholey and completely understand our design.  

That being said some of it is pretty jank... You've been warned...

# Host tools

Below is a step by step guide on how each host tool works, including diagrams where needed

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

Below is a diagram representing the layout of a protected CFG file.

```
[16B password][Config Data][16B password]
``` 

Due to the nature of CBC encryption, if any byte within the protected file is changed, then at least one (usually the second) authentication password will be incorrect, thus allowing for a type of integrity check

## CFG Load
1. Negotiate with the host to enter configure mode
2. Read the config size from the host
3. Host sends first 16 bytes of encrypted config (the first authentication password) to bootloader
4. Bootloader decrypts and confirms this authentication password
5. If the password is correct, the bootloader sends over the cryptographic key and iv to the host
6. Host decrypts config
7. Host sends last 16 bytes of decrypted config (the second authentication password) for confirmation from bootloader
8. If the bootloader confirms this password, then the host sends the now unencrypted configuration to the bootloader for installation

Originally there was no host side decryption, which would be vastly more secure. However, due to the slow speed of the TIVA device (32khz) it would take nearly 2 minutes to decrypt and load a full 64K configuration file. The decision to make most decryption host side was made in order to fit into the 25 second timing requirement.

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

The fw protection is a little more complicated due to extra things like the release message being required to be packed alongside the actual firmware data.

Below is a diagram of a protected firmware file

```
Json File:
    [2B Firmware size],
    [16B Encrypted Firmware Version][16B Version password],
    [Up to 1KB release message],
    [16B password][Firmware Data][16B password]
```

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

Similarly to the configuration, there previously was no host side decryption. Again, the timing requirements made this a neccessity.

Originally, the entire 16K firmware was sent at one time, with an authentication password attatched. Due to RAM limitations and quirks of the way the RAM works on the physical device, though, this proved to be unreliable on anything but the emulated environment.

## Readback
1. Negotiate with bootloader to enter readback mode
2. Send password to bootloader for authentication
3. If password is correct continue
4. Send the region of data we want (Firmware or config)
5. Send number of bytes of data we want
6. Recieve firmare data 

In order to prevent a readback from showing data outside of what it should (for example, a readback of the firmware seeing configuartion data) the bootloader is designed to fill any data outside of the requested region with blank bytes. For example, if an installed firmware image is 127 bytes long, but the user requests 64KB of data, the bootloader will supply the correct 127 bytes of firmware data, but any extra will be supplied as blank bytes (in the form of all 1s, or 0xFF)

