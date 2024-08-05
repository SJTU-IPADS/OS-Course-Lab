/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef COMMON_ENDIANNESS_H
#define COMMON_ENDIANNESS_H

#ifdef BIG_ENDIAN
#define be16_to_cpu(x) (x)
#define be32_to_cpu(x) (x)
#define be64_to_cpu(x) (x)

#define le16_to_cpu(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))
#define le32_to_cpu(x) ((le16_to_cpu((x)) << 16) | (le16_to_cpu((x) >> 16)))
#define le64_to_cpu(x) ((le32_to_cpu((x)) << 32) | (le32_to_cpu((x) >> 32)))

#define le128ptr_to_cpu_lo(x) (le64_to_cpu(*(u64 *)(x)))
#define le128ptr_to_cpu_hi(x) (le64_to_cpu(*((u64 *)(x) + 1)))

#define le96ptr_to_cpu_lo(x) (be32_to_cpu(*(u32 *)(x)))
#define le96ptr_to_cpu_hi(x) (((u64)(le32_to_cpu(*((u32 *)(x) + 1)))) << 32 | \
			      (be32_to_cpu(*((u32 *)(x)) + 2)))

#define leXptr_to_cpu(bits, n) \
	({ \
	 (bits == 32) ? le32_to_cpu(*(u32 *)(n)) : \
	 (bits == 64) ? le64_to_cpu(*(u64 *)(n)) : \
	 ({ BUG("invalid X"); 0; }); \
	 })

#define set_leXptr_to_cpu(bits, n, hi, lo) do {     \
	if (bits > 64) {                            \
		if (bits == 96) {                   \
			lo = le96ptr_to_cpu_lo(n);  \
			hi = le96ptr_to_cpu_hi(n);  \
		} else if (bits == 128) {           \
			lo = le128ptr_to_cpu_lo(n); \
			hi = le128ptr_to_cpu_hi(n); \
		} else {                            \
			BUG("invalid X");           \
		}                                   \
	} else {                                    \
		lo = 0;                             \
		hi = leXptr_to_cpu(bits, n);        \
	}                                           \
} while (0)
#else
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)

#define be16_to_cpu(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))
#define be32_to_cpu(x) ((be16_to_cpu((x)) << 16) | (be16_to_cpu((x) >> 16)))
#define be64_to_cpu(x) ((be32_to_cpu((x)) << 32) | (be32_to_cpu((x) >> 32)))


#define be128ptr_to_cpu_hi(x) (be64_to_cpu(*(u64 *)(x)))
#define be128ptr_to_cpu_lo(x) (be64_to_cpu(*((u64 *)(x) + 1)))

#define be96ptr_to_cpu_hi(x) (be32_to_cpu(*(u32 *)(x)))
#define be96ptr_to_cpu_lo(x) (((u64)(be32_to_cpu(*((u32 *)(x) + 1)))) << 32 | \
			      (be32_to_cpu(*((u32 *)(x)) + 2)))

#define beXptr_to_cpu(bits, n) \
	({ \
	 ((bits) == 32) ? be32_to_cpu(*(u32 *)(n)) : \
	 ((bits) == 64) ? be64_to_cpu(*(u64 *)(n)) : \
	 ({ BUG("invalid X"); 0; }); \
	 })

#define set_beXptr_to_cpu(bits, n, hi, lo) do {     \
	if ((bits) > 64) {                            \
		if ((bits) == 96) {                   \
			lo = be96ptr_to_cpu_lo(n);  \
			hi = be96ptr_to_cpu_hi(n);  \
		} else if ((bits) == 128) {           \
			lo = be128ptr_to_cpu_lo(n); \
			hi = be128ptr_to_cpu_hi(n); \
		} else {                            \
			BUG("invalid X");           \
		}                                   \
	} else {                                    \
		lo = 0;                             \
		hi = beXptr_to_cpu((bits), n);        \
	}                                           \
} while (0)
#endif

#endif /* COMMON_ENDIANNESS_H */