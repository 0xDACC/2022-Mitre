# 2022 eCTF
# Host-Tools and Bootloader Creation Dockerfile
# Andrew Mirghassemi
#
# (c) 2022 The MITRE Corporation
#
# This source file is part of an example system for MITRE's 2022 Embedded System
# CTF (eCTF). This code is being provided only for educational purposes for the
# 2022 MITRE eCTF competition, and may not meet MITRE standards for quality.
# Use this code at your own risk!

FROM ubuntu:focal

# Add environment customizations here
# NOTE: do this first so Docker can used cached containers to skip reinstalling everything
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y python3 \
    binutils-arm-none-eabi gcc-arm-none-eabi make

RUN apt-get install python3-pip

# Install aes encryption
RUN python3 -m pip install PyCryptodome

# Create bootloader binary folder
RUN mkdir /bootloader

# Add any system-wide secrets here
RUN mkdir /secrets

# Add host tools and bootloader source to container
ADD host_tools/ /host_tools
ADD bootloader /bl_build

# Make sure there are empty files for key and iv
RUN echo "" > /secrets/key
RUN echo "" > /secrets/iv

#Generate Keys
RUN python3 /host_tools/keygen

# Create EEPROM contents
RUN echo "" > /bootloader/eeprom.bin

# Add secrets to eeprom
RUN python3 /host_tools/eepromsetup

# Compile bootloader
WORKDIR /bl_build

ARG OLDEST_VERSION
RUN make OLDEST_VERSION=${OLDEST_VERSION}
RUN mv /bl_build/gcc/bootloader.bin /bootloader/bootloader.bin
RUN mv /bl_build/gcc/bootloader.axf /bootloader/bootloader.elf
