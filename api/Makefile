#
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

.PHONY: default
default: all

.PHONY: all
all:
	yace --c-to-yace ../include/libxnvme.h
	yace --c-to-yace ../include/libxnvme.h 2>&1 | grep ERROR | wc
