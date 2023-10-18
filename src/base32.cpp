
#include "base32.h"
#include "Arduino.h"


size_t decode32(const char* coded, size_t len, char* out, size_t out_len) {
  size_t padded_len = len + 8 - len%8;
  size_t outsize = (padded_len*5) >> 3;
  if(out_len < outsize) {
    // output buffer not large enough
    Serial.println("Error, output buffer too small!");
    return 0;
  }
  for(int i=0; i<outsize; i++) {
    out[i] = 0;
  }

  size_t bit_no = 0;

  for(int i=0;i<padded_len; i++) {
    char c;
    if(i < len) {
      c = coded[i];
    } else {
      // add padding if needed
      c = '=';
    }
    uint8_t dec;
    if(c >= 'A' && c <= 'Z') {
      dec = c - 'A';
    } else if(c >= '2' && c <= '7') {
      dec = c - '2' + 26;
    } else if(c == '=') {
      dec = 0;
      //printf("padding!\n");
    } else {
      Serial.printf("Unknown character %c\n", c);
      return 0;
    }

    size_t byte_no_start = bit_no / 8;

    size_t byte_no_end = (bit_no+5) / 8;
    size_t shift_a = (bit_no+5) - 8*(byte_no_start+1);
    size_t shift_b = 8*(byte_no_end+1) - (bit_no+5);

    out[byte_no_start] |= dec >> shift_a;
    out[byte_no_end]   |= dec << shift_b;

    bit_no += 5;
  }

  return outsize;
}
