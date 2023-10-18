
#include "otp.h"
#include "base32.h"
#include "mbedtls/md.h"     // HMAC hashing used for the pairing process
#include "mbedtls/base64.h" // Base64 encoding/decoding for the paring process
#include "string.h"

// See
// https://medium.com/zeals-tech-blog/implementing-your-own-time-based-otp-generator-3b971a31330b
// https://medium.com/@nicola88/two-factor-authentication-with-totp-ccc5f828b6df


// encryption and hashing
uint8_t hmacResult[32];
mbedtls_md_context_t ctx;
mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;



uint32_t getOTP(const unsigned char* key, size_t key_len, uint64_t interval_no) {
  interval_no = __builtin_bswap64(interval_no);
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1); 
  mbedtls_md_hmac_starts(&ctx, key, key_len);

  mbedtls_md_hmac_update(&ctx, (const unsigned char *) &interval_no, 8);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  uint8_t offset = hmacResult[19] & 0X0F;
  uint32_t *truncated = (uint32_t*) &hmacResult[offset];
  uint32_t tt = __builtin_bswap32(*truncated) & 0x7FFFFFFF;
  uint32_t opt = tt % 1000000;

  return opt;
}
