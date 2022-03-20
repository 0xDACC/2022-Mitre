# Getting started with our design

## Introduction
*This document will assume that you have already have an up to date developement environent*

Each file we have modified contains a brief description of the file, which should give a general overview of how the code works. Now, we know that no code "documents itself," so we have included several comments throughout each file to describe its operations. This file is purely to get your toes wet in running our design, which should be functionally identical to the example, from the outside.

*Note: You may have to change the file permissions on some of the host tools in order for this to work. For some reason github likes to remove the execute permission from a them*

## Running the emulated device
### Build the system
```bash
python3 tools/run_saffire.py build-system --emulated --sysname 0xDACC-test --oldest-allowed-version 2
```
This command tells the host to build the system. The name isn't important, and the oldest allowed version is not super important either, as long as you dont plan to use anything older than that version!

### load the device
```bash
python3 tools/run_saffire.py load-device --emulated --sysname 0xDACC-test
```
Since this is the emulated devide this loads everything we've made into the emulated device and opens ourself a little socket to connect to it with.

### Launch the bootloader
```bash
mkdir socks
```
This will make a folder called socks, which is where the socket will be. You only need to do this if you've deleted the folder or if it's your first time running our stuff.

```bash
python3 tools/run_saffire.py launch-bootloader --emulated  --sysname 0xDACC-test --sock-root socks/ --uart-sock 1234
```
This will launch our beautiful bootloader. Here you also set the socket you want to use to connect to this specific version of the device. That number is important so remember it!

If you want to use gdb to debug:
```bash
python3 tools/run_saffire.py launch-bootloader-gdb --emulated  --sysname 0xDACC-test --sock-root socks/ --uart-sock 1234
```

And if you want to use an interactive version:
```bash
python3 tools/run_saffire.py launch-bootloader-interactive --emulated  --sysname 0xDACC-test --sock-root socks/ --uart-sock 1234
```

### Protect the firmware
```bash
python3 tools/run_saffire.py fw-protect --emulated --sysname 0xDACC-test --fw-root firmware/ --raw-fw-file example_fw.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'This is version 4'
```
Odds are you won't be doing this if you're an attacker. But if you want to you can go ahead and protect firmware for a device using our code with that command

This adds an authentication password to the end of the version number and firmware data and then encrypts those and packs them into a json file along with some other metadata. The password allows us to check both authenticity and integrity in one step. Efficiency!

### Protect the configuration
```bash
python3 tools/run_saffire.py cfg-protect --emulated --sysname 0xDACC-test --cfg-root configuration/ --raw-cfg-file example_cfg.bin --protected-cfg-file example_cfg.prot
```
Again, you likely won't be doing much of this command, but it's here if you need it.

This also adds a password to the end of the data before encryption, and serves the same dual purpose authenticity and integrity checks as with the firmware. This writes the output straight to a file rather than to a json file, though.

### Update the firmware
*This is where the fun begins!*
```bash
python3 tools/run_saffire.py fw-update --emulated --sysname 0xDACC --fw-root firmware/ --uart-sock 1234 --protected-fw-file example_fw.prot
```
So here you have to make sure to have the correct `sysname`, `sock`, and `protected-fw-file` so you dont install the wrong stuff on the wrong device

If the bootloader detects that the firmare or version number have been tampered with (using the afformentioned authentication password) then it will reject the update BEFORE any flash memory is written. The bootloader will also cut off any data recieved at a certain length so as to protect against buffer overflow attacks.

### Update the flight configuration
```bash
python3 tools/run_saffire.py cfg-load --emulated --sysname 0xDACC-test --cfg-root configuration/ --uart-sock 1234 --protected-cfg-file example_cfg.prot
```
Again, the `sysname`, `sock`, and `protected-cfg-file` are important here

This will also reject any updates if the authentication password is not decrypted correctly, and also cuts off any excessively long data sent to it.

### Read back the firmware
```bash
python3 tools/run_saffire.py fw-readback --emualted --sysname 0xDACC-test --uart-sock 1234 --rb-len 100
```
When you run this command, there's a little bit of negotiation between the bootloader and the host, after which the bootloader will wait to recieve the authentication password from the host device. If the correct password is supplied it sends back the requested data over the uart interface right to your terminal! This is capped at the firmware length limit of 16k (0x4000), so you can't see any data you shouldn't.

### Read back the configuration
```bash
python3 tools/run_saffire.py cfg-readback --emualted --sysname 0xDACC-test --uart-sock 1234 --rb-len 100
```
When you run this command, there's a little bit of negotiation between the bootloader and the host, after which the bootloader will wait to recieve the authentication password from the host device. If the correct password is supplied it sends back the requested data over the uart interface right to your terminal! This is capped at the firmware length limit of 64k (0xFFFF), so you can't see any data you shouldn't.

### Boot the firmware
```bash
python3 tools/run_saffire.py boot --emulated --sysname 0xDACC-test --uart-sock 1234 --boot-msg-file boot.txt
```
When you issue this command the bootloader checks if there installed firmware has been tampered with by again checking for that authentication password, since the firmware is stored in an encrypted state in the flash. If it finds the password the bootloader, loads the firmware to boot, as it's name would imply. It also writes the boot message to the file specified

### Monitor
```bash
python3 tools/run_saffire.py monitor --emulated --sysname 0xDACC-test --uart-sock 1234 --boot-msg-file boot.txt
```
This hasn't been modified in any way, and just similuates a flight based on whatever firmware is installed.

### Perform a soft reset
```bash
python3 tools/emulator_reset.py --restart-sock socks/restart.sock
```
Simply resets the sockets in the socks folder

### Kill the system
```bash
python3 tools/run_saffire.py kill-system --emulated --sysname 0xDACC-test
```

### Hard reset
```bash
./reset.sh
```
Sometimes you just want a clean slate. This shell script we made will completely reset everything you've done with the bootloader, so use as your own risk. The version we supplied assumes a sysname of `0xDACC-test` so be sure to change that if you want to use a different name for your tests.

## Running the physical device
### Build the system
```bash
python3 tools/run_saffire.py build-system --physical --sysname 0xDACC-test --oldest-allowed-version 2
```
This command tells the host to build the system. The name isn't super important, and the oldest allowed version is not super important either, as long as you dont plan to use anything older than that version! Make sure to remember that name though!

### load the device
```bash
python3 tools/run_saffire.py load-device --physical --sysname 0xDACC-test
```
Starts the socket bridge. This will also tell you the device serial port name that the device is on, so remember that for the next command!

### Launch the bootloader
```bash
mkdir socks
```
This will make a folder called socks, which is where the socket will be. You only need to do this if you've deleted the folder or if it's your first time running our stuff.

```bash
python3 tools/run_saffire.py launch-bootloader --physical  --sysname 0xDACC-test --sock-root socks/ --uart-sock 1234 --serial-port <device_serial_port_name>
```
This will launch our beautiful bootloader. Here you also set the socket you want to use to connect to this specific version of the device. That number is important so remember it! 

### Protect the firmware
```bash
python3 tools/run_saffire.py fw-protect --physical --sysname 0xDACC-test --fw-root firmware/ --raw-fw-file example_fw.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'This is version 4'
```
Odds are you won't be doing this if you're an attacker. But if you want to you can go ahead and protect firmware for a device using our code with that command

This adds an authentication password to the end of the version number and firmware data and then encrypts those and packs them into a json file along with some other metadata. The password allows us to check both authenticity and integrity in one step. Efficiency!

### Protect the configuration
```bash
python3 tools/run_saffire.py cfg-protect --physical --sysname 0xDACC-test --cfg-root configuration/ --raw-cfg-file example_cfg.bin --protected-cfg-file example_cfg.prot
```
Again, you likely won't be doing much of this command, but it's here if you need it.

This also adds a password to the end of the data before encryption, and serves the same dual purpose authenticity and integrity checks as with the firmware. This writes the output straight to a file rather than to a json file, though.

### Update the firmware
*This is where the fun begins!*
```bash
python3 tools/run_saffire.py fw-update --physical --sysname 0xDACC --fw-root firmware/ --uart-sock 1234 --protected-fw-file example_fw.prot
```
So here you have to make sure to have the correct `sysname`, `sock`, and `protected-fw-file` so you dont install the wrong stuff on the wrong device

If the bootloader detects that the firmare or version number have been tampered with (using the afformentioned authentication password) then it will reject the update BEFORE any flash memory is written. The bootloader will also cut off any data recieved at a certain length so as to protect against buffer overflow attacks.

### Update the flight configuration
```bash
python3 tools/run_saffire.py cfg-load --physical --sysname 0xDACC-test --cfg-root configuration/ --uart-sock 1234 --protected-cfg-file example_cfg.prot
```
Again, the `sysname`, `sock`, and `protected-cfg-file` are important here

This will also reject any updates if the authentication password is not decrypted correctly, and also cuts off any excessively long data sent to it.

### Read back the firmware
```bash
python3 tools/run_saffire.py fw-readback --physical --sysname 0xDACC-test --uart-sock 1234 --rb-len 100
```
When you run this command, there's a little bit of negotiation between the bootloader and the host, after which the bootloader will wait to recieve the authentication password from the host device. If the correct password is supplied it sends back the requested data over the uart interface right to your terminal! This is capped at the firmware length limit of 16k (0x4000), so you can't see any data you shouldn't.

### Read back the configuration
```bash
python3 tools/run_saffire.py cfg-readback --physical --sysname 0xDACC-test --uart-sock 1234 --rb-len 100
```
When you run this command, there's a little bit of negotiation between the bootloader and the host, after which the bootloader will wait to recieve the authentication password from the host device. If the correct password is supplied it sends back the requested data over the uart interface right to your terminal! This is capped at the firmware length limit of 64k (0xFFFF), so you can't see any data you shouldn't.

### Boot the firmware
```bash
python3 tools/run_saffire.py boot --physical --sysname 0xDACC-test --uart-sock 1234 --boot-msg-file boot.txt
```
When you issue this command the bootloader checks if there installed firmware has been tampered with by again checking for that authentication password, since the firmware is stored in an encrypted state in the flash. If it finds the password the bootloader, loads the firmware to boot, as it's name would imply. It also writes the boot message to the file specified

### Monitor
```bash
python3 tools/run_saffire.py monitor --physical --sysname 0xDACC-test --uart-sock 1234 --boot-msg-file boot.txt
```
This hasn't been modified in any way, and just similuates a flight based on whatever firmware is installed.

### Perform a soft reset
In order to soft reset the physical device hit the RESET switch on the board