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

constexpr auto armgcc_nxp_1050_json = R"({
  "qulVersion": "2.0.0",
  "compatVersion": "1",
  "platform": {
    "id": "MIMXRT1050-EVK-FREERTOS",
    "vendor": "NXP",
    "colorDepths": [
      16
    ],
    "pathEntries": [],
    "environmentEntries": [],
    "cmakeEntries": [
      {
        "id": "Qul_DIR",
        "label": "Qt for MCUs SDK",
        "type": "path",
        "cmakeVar": "Qul_ROOT",
        "envVar": "Qul_DIR",
        "optional": false
      },
      {
        "id": "MCU_XPRESSO_PATH",
        "label": "MCUXpresso IDE",
        "type": "path",
        "cmakeVar": "MCUXPRESSO_IDE_PATH",
        "defaultValue": {
          "windows": "$ROOT/nxp/MCUXpressoIDE*",
          "unix": "/usr/local/mcuxpressoide/"
        },
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
        "defaultValue": "/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/armgcc.cmake",
        "visible": false,
        "optional": false
      }
  },
  "boardSdk": {
    "envVar": "EVKB_IMXRT1050_SDK_PATH",
    "versions": [ "2.11.0" ],
    "id": "NXP_SDK_DIR",
    "label": "Board SDK for MIMXRT1050-EVK",
    "cmakeVar": "QUL_BOARD_SDK_DIR",
    "envVar": "EVKB_IMXRT1050_SDK_PATH",
    "setting": "EVKB_IMXRT1050_SDK_PATH",
    "type": "path",
    "optional": false
  },
  "freeRTOS": {
    "envVar": "IMXRT1050_FREERTOS_DIR",
    "cmakeEntries": [
      {
        "id": "NXP_FREERTOS_DIR",
        "label": "FreeRTOS SDK for MIMXRT1050-EVK",
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "$QUL_BOARD_SDK_DIR/rtos/freertos/freertos_kernel",
        "type": "path",
        "optional": false
      }
    ]
  }
})";
