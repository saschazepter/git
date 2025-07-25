# -*- mode: makefile; coding: utf-8 -*-
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

# Update changelog via CLI:
# export DEBEMAIL=volker.theile@openmediavault.org; export DEBFULLNAME="Volker Theile"; dch -i -D stable -u low

OMV_SUBDIRS := $(shell find . -maxdepth 1 -name 'openmediavault*' -type d)

# Targets to process all OpenMediaVault plugin packages.
clean binary omv_tx_pull_po omv_tx_push_pot omv_build_pot:
	for dir in $(OMV_SUBDIRS); do \
		cd $(CURDIR)/$$dir && fakeroot debian/rules $@; \
	done

install:
	echo "Installing required packages ..."
	sudo apt-get update
	sudo apt-get install -y debhelper fakeroot gettext python3-pip npm
	pip3 install --upgrade --break-system-packages autopep8 isort pylint

.PHONY: install clean binary omv_tx_pull_po omv_tx_push_pot omv_build_pot
