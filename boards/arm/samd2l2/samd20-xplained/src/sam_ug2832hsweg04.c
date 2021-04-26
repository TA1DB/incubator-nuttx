/****************************************************************************
 * boards/arm/samd2l2/samd20-xplained/src/sam_ug2832hsweg04.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* OLED1 Connector:
 *
 *  OLED1 CONNECTOR
 *  ----------------- ---------------------- ----------------------
 *  OLED1             EXT1                   EXT2
 *  ----------------- ---------------------- ----------------------
 *  1  ID             1                      1
 *  ----------------- ---------------------- ----------------------
 *  2  GND            2       GND            2  GND
 *  ----------------- ---------------------- ----------------------
 *  3  BUTTON2        3  PB00 AIN[8]         3  PA10 AIN[18]
 *  ----------------- ---------------------- ----------------------
 *  4  BUTTON3        4  PB01 AIN[9]         4  PA11 AIN[19]
 *  ----------------- ---------------------- ----------------------
 *  5  DATA_CMD_SEL   5  PB06 PORT           5  PA20 PORT
 *  ----------------- ---------------------- ----------------------
 *  6  LED3           6  PB07 PORT           6  PA21 PORT
 *  ----------------- ---------------------- ----------------------
 *  7  LED1           7  PB02 TC6/WO[0]      7  PA22 TC4/WO[0]
 *  ----------------- ---------------------- ----------------------
 *  8  LED2           8  PB03 TC6/WO[1]      8  PA23 TC4/WO[1]
 *  ----------------- ---------------------- ----------------------
 *  9  BUTTON1        9  PB04 EXTINT[4]      9  PB14 EXTINT[14]
 *  ----------------- ---------------------- ----------------------
 *  10 DISPLAY_RESET  10 PB05 PORT           10 PB15 PORT
 *  ----------------- ---------------------- ----------------------
 *  11 N/C            11 PA08 SERCOM2 PAD[0] 11 PA08 SERCOM2 PAD[0]
 *                            I�C SDA                I�C SDA
 *  ----------------- ---------------------- ----------------------
 *  12 N/C            12 PA09 SERCOM2 PAD[1] 12 PA09 SERCOM2 PAD[1]
 *                            I�C SCL                I�C SCL
 *  ----------------- ---------------------- ----------------------
 *  13 N/C            13 PB09 SERCOM4 PAD[1] 13 PB13 SERCOM4 PAD[1]
 *                            USART RX               USART RX
 *  ----------------- ---------------------- ----------------------
 *  14 N/C            14 PB08 SERCOM4 PAD[0] 14 PB12 SERCOM4 PAD[0]
 *                            USART TX               USART TX
 *  ----------------- ---------------------- ----------------------
 *  15 DISPLAY_SS     15 PA05 SERCOM0 PAD[1] 15 PA17 SERCOM1 PAD[1]
 *                            SPI SS                 SPI SS
 *  ----------------- ---------------------- ----------------------
 *  16 SPI_MOSI       16 PA06 SERCOM0 PAD[2] 16 PA18 SERCOM1 PAD[2]
 *                            SPI MOSI               SPI MOSI
 *  ----------------- ---------------------- ----------------------
 *  17 N/C            17 PA04 SERCOM0 PAD[0] 17 PA16 SERCOM1 PAD[0]
 *                            SPI MISO               SPI MISO
 *  ----------------- ---------------------- ----------------------
 *  18 SPI_SCK        18 PA07 SERCOM0 PAD[3] 18 PA19 SERCOM1 PAD[3]
 *                            SPI SCK                SPI SCK
 *  ----------------- ---------------------- ----------------------
 *  19 GND            19      GND               GND
 *  ----------------- ---------------------- ----------------------
 *  20 VCC            20      VCC               VCC
 *  ----------------- ---------------------- ----------------------
 *
 * OLED1 signals
 *
 * DATA_CMD_SEL - Data/command select. Used to choose whether the
 *   communication is data to the display memory or a command to the LCD
 *   controller. High = data, low = command
 * DISPLAY_RESET - Reset signal to the OLED display, active low. Used during
 *   initialization of the display.
 * DISPLAY_SS - SPI slave select signal, must be held low during SPI
 *   communication.
 * SPI_MOSI - SPI master out, slave in signal. Used to write data to the
 *   display
 * SPI_SCK SPI - clock signal, generated by the master.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>

#include <nuttx/board.h>
#include <nuttx/spi/spi.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/ssd1306.h>

#include "sam_port.h"
#include "sam_spi.h"

#include "samd20-xplained.h"

#ifdef CONFIG_SAMD20_XPLAINED_OLED1MODULE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* The pin configurations here require that SPI1 is selected */

#ifndef CONFIG_LCD_SSD1306
#  error "The OLED driver requires CONFIG_LCD_SSD1306 in the configuration"
#endif

#ifndef CONFIG_LCD_UG2832HSWEG04
#  error "The OLED driver requires CONFIG_LCD_UG2832HSWEG04 in the configuration"
#endif

#ifndef SAMD2L2_HAVE_SPI0
#  error "The OLED driver requires SAMD2L2_HAVE_SPI0 in the configuration"
#endif

#ifndef CONFIG_SPI_CMDDATA
#  error "The OLED driver requires CONFIG_SPI_CMDDATA in the configuration"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_graphics_setup
 *
 * Description:
 *   Called by NX initialization logic to configure the OLED.
 *
 ****************************************************************************/

FAR struct lcd_dev_s *board_graphics_setup(unsigned int devno)
{
  FAR struct spi_dev_s *spi;
  FAR struct lcd_dev_s *dev;

  /* Configure the OLED PORTs. This initial configuration is RESET low,
   * putting the OLED into reset state.
   */

  sam_configport(PORT_OLED_RST);

  /* Wait a bit then release the OLED from the reset state */

  up_mdelay(20);
  sam_portwrite(PORT_OLED_RST, true);

  /* Get the SPI1 port interface */

  spi = sam_spibus_initialize(OLED_CSNO);
  if (!spi)
    {
      lcderr("ERROR: Failed to initialize SPI port 1\n");
    }
  else
    {
      /* Bind the SPI port to the OLED */

      dev = ssd1306_initialize(spi, NULL, devno);
      if (!dev)
        {
          lcderr("ERROR: Failed to bind SPI port 1 to OLED %d\n", devno);
        }
     else
        {
          lcdinfo("Bound SPI port 1 to OLED %d\n", devno);

          /* And turn the OLED on */

          dev->setpower(dev, CONFIG_LCD_MAXPOWER);
          return dev;
        }
    }

  return NULL;
}
#endif /* CONFIG_SAMD20_XPLAINED_OLED1MODULE */
