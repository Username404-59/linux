/* SPDX-License-Identifier: GPL-2.0-or-later */
// Copyright 2024 Google LLC

#include "crc-pclmul-template.S"

DEFINE_CRC_PCLMUL_FUNCS(crc32_lsb, /* bits= */ 32, /* lsb= */ 1)
DEFINE_CRC_PCLMUL_FUNCS(crc32_msb, /* bits= */ 32, /* lsb= */ 0)
