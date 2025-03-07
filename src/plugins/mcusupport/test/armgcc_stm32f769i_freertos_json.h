/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

constexpr auto armgcc_stm32f769i_freertos_json = R"({
  "qulVersion": "@CMAKE_PROJECT_VERSION@",
  "compatVersion": "@COMPATIBILITY_VERSION@",
  "platform": {
    "id": "STM32F769I-DISCOVERY-FREERTOS",
    "vendor": "ST",
    "colorDepths": [
      32
    ],
    "pathEntries": [
      {
        "id": "STM32CubeProgrammer_PATH",
        "id": "STM32CubeProgrammer_PATH",
        "label": "STM32CubeProgrammer",
        "type": "path",
        "defaultValue": {
          "windows": "$PROGRAMSANDFILES/STMicroelectronics/STM32Cube/STM32CubeProgrammer/",
          "unix": "$HOME/STMicroelectronics/STM32Cube/STM32CubeProgrammer/"
        },
        "optional": false
      }
    ],
    "environmentEntries": [],
    "cmakeEntries": [
      {
        "id": "Qul_DIR",
        "label": "Qt for MCUs SDK",
        "type": "path",
        "cmakeVar": "Qul_ROOT",
        "optional": false
      }
    ]
  },
  "toolchain": {
    "id": "armgcc",
    "versions": [
      "9.3.1",
      "10.3.1"
    ],
    "compiler": {
        "id": "ARMGCC_DIR",
        "label": "GNU Arm Embedded Toolchain",
        "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
        "envVar": "ARMGCC_DIR",
        "setting": "GNUArmEmbeddedToolchain",
        "type": "path",
        "optional": false
      },
      "file": {
        "id": "ARMGCC_CMAKE_TOOLCHAIN_FILE",
        "label": "CMake Toolchain File",
        "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
        "type": "file",
        "defaultValue": "/opt/qtformcu/2.2//lib/cmake/Qul/toolchain/armgcc.cmake",
        "visible": false,
        "optional": false
      }
  },
  "boardSdk": {
    "envVar": "STM32Cube_FW_F7_SDK_PATH",
    "setting": "STM32Cube_FW_F7_SDK_PATH",
    "versions": [
      "1.16.0"
    ],
    "id": "ST_SDK_DIR",
    "label": "Board SDK for STM32F769I-Discovery",
    "cmakeVar": "QUL_BOARD_SDK_DIR",
    "type": "path",
    "optional": false
  },
  "freeRTOS": {
    "envVar": "STM32F7_FREERTOS_DIR",
    "cmakeEntries": [
      {
        "id": "ST_FREERTOS_DIR",
        "label": "FreeRTOS SDK for STM32F769I-Discovery",
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "$QUL_BOARD_SDK_DIR/Middlewares/Third_Party/FreeRTOS/Source",
        "type": "path",
        "optional": false
      }
    ]
  }
})";
