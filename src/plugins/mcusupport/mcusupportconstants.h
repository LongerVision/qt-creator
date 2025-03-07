/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

namespace McuSupport::Internal::Constants {

const char DEVICE_TYPE[]{"McuSupport.DeviceType"};
const char DEVICE_ID[]{"McuSupport.Device"};
const char RUNCONFIGURATION[]{"McuSupport.RunConfiguration"};
const char SETTINGS_ID[]{"CC.McuSupport.Configuration"};

const char KIT_MCUTARGET_VENDOR_KEY[]{"McuSupport.McuTargetVendor"};
const char KIT_MCUTARGET_MODEL_KEY[]{"McuSupport.McuTargetModel"};
const char KIT_MCUTARGET_SDKVERSION_KEY[]{"McuSupport.McuTargetSdkVersion"};
const char KIT_MCUTARGET_KITVERSION_KEY[]{"McuSupport.McuTargetKitVersion"};
const char KIT_MCUTARGET_COLORDEPTH_KEY[]{"McuSupport.McuTargetColorDepth"};
const char KIT_MCUTARGET_OS_KEY[]{"McuSupport.McuTargetOs"};
const char KIT_MCUTARGET_TOOLCHAIN_KEY[]{"McuSupport.McuTargetToolchain"};

const char SETTINGS_GROUP[]{"McuSupport"};
const char SETTINGS_KEY_PACKAGE_PREFIX[]{"Package_"};
const char SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK[]{"QtForMCUsSdk"}; // Key known by SDK installer
const char SETTINGS_KEY_AUTOMATIC_KIT_CREATION[]{"AutomaticKitCreation"};

} // namespace McuSupport::Internal::Constants
