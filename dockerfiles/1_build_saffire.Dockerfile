# 2022 eCTF
# Host-Tools and Bootloader Creation Dockerfile
# 0xDACC
#
# (c) 2022 The MITRE Corporation
#
# This file now meets all functional and security requirements. Not much was changed here, just added eeprom support and adding keygen

FROM ubuntu:focal

# NOTE: do this first so Docker can used cached containers to skip reinstalling everything
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y python3 \
    binutils-arm-none-eabi gcc-arm-none-eabi make \
    python3-pip

# Install aes encryption
RUN python3 -m pip install PyCryptodome

# Create bootloader binary folder
RUN mkdir /bootloader

# Add any system-wide secrets here
RUN mkdir /secrets

# Add host tools and bootloader source to container
ADD host_tools/ /host_tools
ADD bootloader /bl_build

# Make sure there are empty files for key and iv and password
RUN echo "" > /secrets/key
RUN echo "" > /secrets/iv
RUN echo "" > /secrets/password

#Generate Keys
RUN python3 /host_tools/keygen

# Create EEPROM contents
RUN echo "" > /bootloader/eeprom.bin

# Add secrets to eeprom
RUN python3 /host_tools/eepromsetup

# Compile bootloader
WORKDIR /bl_build

ARG OLDEST_VERSION
RUN make OLDEST_VERSION=${OLDEST_VERSION} | tee /host_tools/makelog.log
RUN mv /bl_build/gcc/bootloader.bin /bootloader/bootloader.bin
RUN mv /bl_build/gcc/bootloader.axf /bootloader/bootloader.elf
