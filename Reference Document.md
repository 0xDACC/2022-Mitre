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
3. Pad data to make it a multiple of 1024
4. Split data into 1KB chunks
5. Add authentication password to end of each chunk
6. Encrypt each chunk individually
7. Combine into 1 protected data package
8. Write out to file

Below is a diagram representing the layout of a protected CFG file.

```
[1024 Bytes of CFG data]
[16 Bytes of authentication data]

[1024 Bytes of CFG data]
[16 Bytes of authentication data]

[1024 Bytes of CFG data]
[16 Bytes of authentication data]

[1024 Bytes of CFG data]
[16 Bytes of authentication data]

[1024 Bytes of CFG data]
[16 Bytes of authentication data]
```

These chunks are combined into one monolithic .prot file which will be deciphered by the CFG load tool and bootloader

This repetition of the authentication password for every 1kb chunk also means that for every 1KB of CFG data, the size of the protected file will be 16B longer than the unprotected version.

The reasoning for this added complexity is due to the memory limitations on the tiva device. The device only has 32KB of RAM, and at any given moment, its not reasonable to hold more than about 4KB in memory without special care being taken, and even that is frustrating. In order to A) Meet functionality requirements and B) Prevent bugs from automatic allocation of this much memory, we have elected to split the config into chunks of 1KB of data + 16B of authentication data.  

## CFG Load
1. Connect to bootloader
2. Negotiate with bootloader to enter configure mode
3. Send CFG size
4. Send over the CFG data in 1KB + 16B blocks (1KB of data and 16B of authentication data)
5. Rinse and repeat until the whole file has been sent and checked for authenticity

Rather than send all of the (potentially 64KB) data at once, the tool breaks the protected file into 1040 byte long chunks, and further breaks those down into a 1KB data frame, and a 16B password frame and sends these in seperate actions and awaits a boot loader response for each individual frame. 

```
[1KB data frame]        -->     [Bootloader]
[16B password frame]    -->     [Bootloader]

[Bootloader determins authenticity] --> [Data frame] --> [Flash memory]
```

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

The fw protection is a little more simple, due to the smaller size of the firmware

Below is a diagram of a protected firmware file

```
Json File:
    [2B Firmware size],
    [16B Encrypted Firmware Version] + [16B Version password],
    [Up to 1KB release message],
    [Up to 16KB encrypted firmware] + [16B Authentication password]
```

## Fw Update
1. Negotiate with bootloader to enter update mode
2. Send version number, and wait for authentication
3. Send firmware size
4. Send release message
5. Send firmware data, and wait for authentication

In contrast to the configuration, the bootloader recieves and digests the ENTIRE firmware data at once, rather than splitting into 1KB chunks. Due to limitations of automatic memory allocation, there is a pointer (named LONG_BUFFER_START_PTR) pointing to the latest point in memory where 16KB + 16B can be stored in a continuous chain in memory without overwriting anything else, or without trying to access invalid memory. After about 4KB, the compiler and bootloader gets confused when trying to allocate a buffer location, and it tries to read invalid memmory addresses. 

## Readback
1. Negotiate with bootloader to enter readback mode
2. Send password to bootloader for authentication
3. If password is correct continue
4. Send the region of data we want (Firmware or config)
5. Send number of bytes of data we want
6. Recieve firmare data 

In order to prevent a readback from showing data outside of what it should (for example, a readback of the firmware seeing configuartion data) the bootloader is designed to fill any data outside of the requested region with blank bytes. For example, if an installed firmware image is 127 bytes long, but the user requests 64KB of data, the bootloader will supply the correct 127 bytes of firmware data, but any extra will be supplied as blank bytes (in the form of all 1s, or 0xFF)

