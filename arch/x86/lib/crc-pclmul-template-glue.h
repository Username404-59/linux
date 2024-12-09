/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Macros for accessing the [V]PCLMULQDQ-based CRC functions that are
 * instantiated by crc-pclmul-template.S
 *
 * Copyright 2024 Google LLC
 *
 * Author: Eric Biggers <ebiggers@google.com>
 */
#ifndef _CRC_PCLMUL_TEMPLATE_GLUE_H
#define _CRC_PCLMUL_TEMPLATE_GLUE_H

#include <asm/cpufeatures.h>
#include <crypto/internal/simd.h>
#include <linux/static_call.h>

#define DECLARE_CRC_PCLMUL_FUNCS(prefix, crc_t)				\
crc_t prefix##_pclmul_sse(crc_t crc, const u8 *p, size_t len,		\
			  const void *consts_ptr);			\
crc_t prefix##_vpclmul_avx2(crc_t crc, const u8 *p, size_t len,		\
			    const void *consts_ptr);			\
crc_t prefix##_vpclmul_avx10_256(crc_t crc, const u8 *p, size_t len,	\
				 const void *consts_ptr);		\
crc_t prefix##_vpclmul_avx10_512(crc_t crc, const u8 *p, size_t len,	\
				 const void *consts_ptr);		\
									\
DEFINE_STATIC_CALL(prefix##_pclmul, prefix##_pclmul_sse)

#define INIT_CRC_PCLMUL(prefix)						\
do {									\
	if (IS_ENABLED(CONFIG_AS_VPCLMULQDQ) &&				\
	    boot_cpu_has(X86_FEATURE_VPCLMULQDQ) &&			\
	    boot_cpu_has(X86_FEATURE_AVX2) &&				\
	    cpu_has_xfeatures(XFEATURE_MASK_YMM, NULL)) {		\
		if (boot_cpu_has(X86_FEATURE_AVX512BW) &&		\
		    boot_cpu_has(X86_FEATURE_AVX512VL) &&		\
		    cpu_has_xfeatures(XFEATURE_MASK_AVX512, NULL)) {	\
			if (boot_cpu_has(X86_FEATURE_PREFER_YMM))	\
				static_call_update(prefix##_pclmul,	\
						   prefix##_vpclmul_avx10_256); \
			else						\
				static_call_update(prefix##_pclmul,	\
						   prefix##_vpclmul_avx10_512); \
		} else {						\
			static_call_update(prefix##_pclmul,		\
					   prefix##_vpclmul_avx2);	\
		}							\
	}								\
} while (0)

/*
 * Call a [V]PCLMULQDQ optimized CRC function if SIMD is usable and the CPU has
 * PCLMULQDQ support, and the length is not very small.
 *
 * The SIMD functions require len >= 16.  However, if the fallback
 * implementation uses slice-by-8 instead of slice-by-1 (which makes it much
 * faster, assuming the larger tables stay in dcache...), then roughly len >= 64
 * is needed for the overhead of the kernel_fpu_{begin,end}() to be worth it.
 *
 * (64 is just a rough estimate.  The exact breakeven point varies by factors
 * such as the CPU model; how many FPU sections are executed before returning to
 * userspace, considering that only one XSAVE + XRSTOR pair is executed no
 * matter how many FPU sections there are; whether the userspace thread used ymm
 * or zmm registers which makes the XSAVE + XRSTOR more expensive; and whether
 * the thread is a kernel thread, which never needs the XSAVE + XRSTOR.)
 */
#define CRC_PCLMUL(crc, p, len, prefix, consts,				\
		   have_pclmulqdq, is_fallback_sliced)			\
do {									\
	if ((len) >= ((is_fallback_sliced) ? 64 : 16) &&		\
	    static_branch_likely(&(have_pclmulqdq)) &&			\
	    crypto_simd_usable()) {					\
		const void *consts_ptr;					\
									\
		consts_ptr = (consts).fold_across_128_bits_consts;	\
		kernel_fpu_begin();					\
		crc = static_call(prefix##_pclmul)((crc), (p), (len),	\
						   consts_ptr);		\
		kernel_fpu_end();					\
		return crc;						\
	}								\
} while (0)

#endif /* _CRC_PCLMUL_TEMPLATE_GLUE_H */
