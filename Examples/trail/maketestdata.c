#include <stdio.h>

int main(void)
{
  FILE *pF = fopen("testdata.bin", "wb");
  unsigned char header[4] = {0xff, 0x00, 0x00, 0xff};
  fwrite(header, 1, 4, pF);

  unsigned char buffer[256];
  for(int i = 0; i < 256; i++)
    buffer[i] = i;
  
  for(int p =0; p < (140000/256); p++)
      fwrite(buffer, 1, 256, pF);
      
  fclose(pF);
}

