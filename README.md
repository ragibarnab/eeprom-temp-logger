# eeprom-temp-logger
## Description
A STM32 microcontroller project. Used [Nucleo-F446RE](https://www.st.com/en/evaluation-tools/nucleo-f446re.html) development board. 
The MCU reads from a digital temperature sensor ([TMP75B](https://www.ti.com/lit/ds/symlink/tmp75b.pdf?ts=1611471999214&ref_url=https%253A%252F%252Fwww.google.com%252F)) 
with I2C and logs it into a serial EEPROM chip ([25AA640A](http://ww1.microchip.com/downloads/en/DeviceDoc/21830F.pdf)) using SPI communication. USART also logs the temperature
value into the serial port. This project should be compatible with most Ti I2C temperature sensors and Microchip serial EEPROMs. Also HAL drivers makes it easy to be compatible
with other STM32 MCUs that possess the same used peripherals.

Prototyped in [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) and exported as a makefile project using the [CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) generator.

## Build Options
1. Download [GNU ARM Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) and use make. 
Then upload using the .bin, .hex, or .elf files. 
2. The .ioc file is also there to generate code using CubeMX for other IDEs. Simply copy the [main.c](Core/Src/main.c) source file after generation.


## Temperature Sensing

![Serial Com](/images/serial_com.png)

Above shows the temperature reading of the sensor in the serial monitor after I have let go off my thumb from the sensor for less than a second. 12-bit or higher resolution digital temperature sensors are usually only found in surface mount packages, like the one I bought, so I used a SMD-to-DIP adapter that I soldered the sensor onto for this project. The digital temperature readings are in two's complement format, according to datasheet by Ti, with 0.0625 degrees Celsius/LSB. Decoding the 12-bit digital format is specified as well if you want to read the values in the EEPROM.
