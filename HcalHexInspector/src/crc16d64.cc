//
// Calculate CRC on a FED 64-bit word
//

int crc16d64_(unsigned int high_word, unsigned int low_word, int crc){

  int d[64] = {0x8005, 0x800f, 0x801b, 0x8033, 0x8063, 0x80c3, 0x8183, 0x8303,
	       0x8603, 0x8c03, 0x9803, 0xb003, 0xe003, 0x4003, 0x8006, 0x8009,
	       0x8017, 0x802b, 0x8053, 0x80a3, 0x8143, 0x8283, 0x8503, 0x8a03,
	       0x9403, 0xa803, 0xd003, 0x2003, 0x4006, 0x800c, 0x801d, 0x803f,
	       0x807b, 0x80f3, 0x81e3, 0x83c3, 0x8783, 0x8f03, 0x9e03, 0xbc03,
	       0xf803, 0x7003, 0xe006, 0x4009, 0x8012, 0x8021, 0x8047, 0x808b,
	       0x8113, 0x8223, 0x8443, 0x8883, 0x9103, 0xa203, 0xc403, 0x0803,
	       0x1006, 0x200c, 0x4018, 0x8030, 0x8065, 0x80cf, 0x819b, 0x8333};

  int c[16] = {0x8113, 0x8223, 0x8443, 0x8883, 0x9103, 0xa203, 0xc403, 0x0803,
	       0x1006, 0x200c, 0x4018, 0x8030, 0x8065, 0x80cf, 0x819b, 0x8333};
  int crcnew,i;

  crcnew = 0;
  for (i=32;i<64;i++){
    if(high_word%2 == 1)crcnew ^= d[i];
    high_word >>= 1;
  }
  for (i=0;i<32;i++){
    if(low_word%2 == 1)crcnew ^= d[i];
    low_word >>= 1;
  }
  for (i=0;i<16;i++){
    if(crc%2 == 1)crcnew ^= c[i];
    crc >>= 1;
  }
  return crcnew;
}     
