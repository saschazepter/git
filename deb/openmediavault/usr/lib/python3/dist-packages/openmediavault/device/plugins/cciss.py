# -*- coding: utf-8 -*-
#
# This file is part of OpenMediaVault.
#
# @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
# @author    Volker Theile <volker.theile@openmediavault.org>
# @copyright Copyright (c) 2009-2025 Volker Theile
#
# OpenMediaVault is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# OpenMediaVault is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OpenMediaVault. If not, see <https://www.gnu.org/licenses/>.
import os
import re

import openmediavault.device


class StorageDevicePlugin(openmediavault.device.IStorageDevicePlugin):
    def match(self, device_file):
        # Examples:
        # - /dev/cciss/c0d0
        # - /dev/cciss/c0d0p2
        device_name = re.sub(r'^/dev/', '', device_file)
        return (
            re.match(
                r'^cciss/c[0-9]+d[0-9]+(p[0-9]+)?$',
                device_name,
            )
            is not None
        )

    def from_device_file(self, device_file):
        return StorageDevice(device_file)


class StorageDevice(openmediavault.device.StorageDevice):
    """
    Implements the storage device for HP Smart Array RAID controllers
    using the cciss driver.
    """

    @property
    def parent(self):
        # /dev/cciss/c0d0p2 -> /dev/cciss/c0d0
        parent_device_file = re.sub(r'(p\d+)$', '', self.canonical_device_file)
        if parent_device_file != self.canonical_device_file:
            return self.__class__(parent_device_file)
        return None

    def device_name(self, canonicalize=False):
        # Get the device name and convert '/' character to '!', e.g.
        # cciss/c0d0 => cciss!c0d0.
        return super().device_name(canonicalize).replace('/', '!')

    @property
    def is_raid(self):
        return True

    @property
    def smart_device_type(self):
        m = re.match(r'^cciss!c(\d)d(\d)$', self.device_name())
        if m:
            return 'cciss,{}'.format(m.group(2))
        return ''
