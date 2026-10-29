#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

SHELL := /bin/sh
.DEFAULT_GOAL := framework
.DELETE_ON_ERROR:

TARGET ?= iphonesimulator
MATCH ?= $(TARGET)
CONFIGURATION ?= Debug
OF_SYSTEM_NAMES ?= NO
PYTHON ?= python3

BUILD = $(PYTHON) Tools/build.py --target $(TARGET) --match $(MATCH) --configuration $(CONFIGURATION) --system-names $(OF_SYSTEM_NAMES)

.PHONY: framework test validate coexistence check prefix-check progress format format-check project project-check xcode verify clean help

framework test validate coexistence:
	$(BUILD) $@

check: format-check project-check
	$(PYTHON) Tools/check.py

prefix-check:
	$(PYTHON) Tools/prefix-mapping.py check

progress:
	$(PYTHON) Tools/progress.py

format:
	$(PYTHON) Tools/format.py

format-check:
	$(PYTHON) Tools/format.py --check

project:
	$(PYTHON) Tools/generate-project.py

project-check:
	$(PYTHON) Tools/generate-project.py --check

xcode:
	$(BUILD) xcode

verify:
	$(PYTHON) Tools/verify.py

clean:
	$(PYTHON) Tools/build.py clean

help:
	@echo 'make [framework|test|validate|coexistence|check|format|project|xcode|verify|clean]'
	@echo 'TARGET=iphonesimulator MATCH=iphonesimulator CONFIGURATION=Debug|Release OF_SYSTEM_NAMES=NO|YES'
	@echo 'The bootstrap exports zero restored APIs. See Documentation/Method.md.'
