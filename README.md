# Custom Bootloader
This project implements a custom bootloader on the STM32 NUCLEO-F767ZI platform to demonstrate embedded firmware update and OTA-related concepts using FreeRTOS-based application software.  

# Functional Requirements  

->  The system shall implement a simple LED toggle application running on FreeRTOS to demonstrate application software execution.  
->  The system shall implement a UART-based communication interface between the host PC and target controller for firmware transfer.  
->  The bootloader shall validate the firmware image using CRC verification and application header checks prior to flashing.  
->  Upon successful firmware update, the bootloader shall safely transfer execution to the application software.  

# Features  

Custom STM32 bootloader  
FreeRTOS-based application  
UART firmware flashing  
Application image validation  
Bootloader-to-application jump mechanism  
Firmware header handling  
CRC validation  
