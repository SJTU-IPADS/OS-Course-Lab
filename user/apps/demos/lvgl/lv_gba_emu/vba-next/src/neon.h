#ifndef __NEON_H__
#define __NEON_H__

#if HAVE_NEON
#if __cplusplus
extern "C" {
#endif

void neon_memcpy(void*, const void*, size_t);

#if __cplusplus
}
#endif

#endif

#endif

