#include <stdio.h>      /* printf atoi */
#include <stdlib.h>     /* strtol */
#include <string.h>     /* memset */
#include <sys/stat.h>

#ifdef __arm__
#define FILE_NAME "/home/rmm"
#else
#define FILE_NAME "rmm"
#define _DEBUG_
#endif

#define VERSION "2.0"

// main -> rmm_init -> rmm_init_venc 

// #define FILE_SIZE 1914296   // 1851N
// #define SEEK_POS0 0x0273a8u // 1851N
// #define SEEK_POS1 0x027434u // 1851N
// #define FW_VERSION "1851N"
// #define PATCH_DATE "26.01.2016"

// #define FILE_SIZE 1914304   // 1851L
// #define SEEK_POS0 0x0272FCu // 1851L
// #define SEEK_POS1 0x027370u // 1851L
// #define FW_VERSION "1851L"
// #define PATCH_DATE "16.02.2016"

// #define FILE_SIZE 1915448   // 1861A
// #define SEEK_POS0 0x0273D8u // 1851A
// #define SEEK_POS1 0x027464u // 1861A
// #define FW_VERSION "1861A"
// #define PATCH_DATE "16.02.2016"

// #define FILE_SIZE 1918392   // 1861B
// #define SEEK_POS0 0x02745Cu // 1861B
// #define SEEK_POS1 0x0274E8u // 1861B
// #define FW_VERSION "1861B"
// #define PATCH_DATE "22.03.2016"

// #define FILE_SIZE 1918392   // 1861C
// #define SEEK_POS0 0x02745Cu // 1861C
// #define SEEK_POS1 0x0274E8u // 1861C
// #define FW_VERSION "1861C"
// #define PATCH_DATE "21.06.2016"

// main -> (rmm_venc_create)

#define FILE_SIZE 1384660   // 1870D US
#define SEEK_POS0 0x01e554u // 1870D US
#define SEEK_POS1 0x01e67cu // 1870D US
#define FW_VERSION "1870D 27US"
#define PATCH_DATE "15.10.2017"

#define MIN_VALUE 80
#define MAX_VALUE 6080

typedef unsigned char BYTE;


unsigned int ror(unsigned int n, unsigned short int i) {
    return ((n >> i) | (n << (32 - i))) >> 0;
}

unsigned int rol(unsigned int n, unsigned short int i) {
    return ((n << i) | (n >> (32 - i))) >> 0;
}

int encode(unsigned int n) {
    unsigned int m, i = 0;

    while (i < 16) {
      m = rol(n, i * 2);
      if (m < 256) {
        return (i << 8) | m;
      }
      i++;
    };
    return -1;
}

unsigned int mln(unsigned int val) {
    unsigned int i = 0, m = 3 << 30;
    while (i < 13) { // (32 - 8) / 2 = 12
      if ((val & m) > 0) {
         return 30 - i * 2 - 6;
      }
      i++;
      m >>= 2;
    };
    return 0;
}


int main(int argc, char* argv[]){
   int val;
   unsigned int adr;
   unsigned short int sft, reg, ch = 0;
   BYTE buf[4];
   FILE *fd;
   struct stat st;

   if (argc > 1) {
     fd = fopen(FILE_NAME,"rb+");
     if (fd) {
       if (!fstat(fileno(fd), &st)) {
         val = st.st_size;
         if (val == FILE_SIZE) { 
           ch = abs(atoi(argv[1])) & 0x01;
           adr = ch ? SEEK_POS1 : SEEK_POS0;
#ifdef _DEBUG_
           printf("ch = %d adr 0x%06x\n", ch, adr);
#endif
           memset(&buf, 0, 4);
           fseek(fd, adr, SEEK_SET);
           val = fread(&buf, 4, 1, fd);
#ifdef _DEBUG_
           printf("get [%d] %02X %02X %02X %02X : ", val, buf[0], buf[1], buf[2], buf[3]);
#endif
           if (buf[2] == 0xa0 && buf[3] == 0xe3) {
             val = (unsigned int)buf[0];
             sft = (unsigned int)buf[1] & 0x0f;
             reg = (unsigned int)buf[1] & 0xf0;
#ifdef _DEBUG_
             printf("reg R%-2d shift 0x%X val 0x%02X [%d] : ", reg >> 4, sft, val, val);
#endif
             val = ror(val, (sft * 2));
#ifdef _DEBUG_
             printf("%d 0x%06x\n", val, val);
#else
             printf("ch:%d bitrate %d%s", ch, val, (argc == 3 ? " : ": "\n"));
#endif

             if (argc == 3) {
               val = abs(atoi(argv[2]));
#ifdef _DEBUG_
               printf("new %d 0x%06x : ", val, val);
#endif
               if (val < MIN_VALUE || val > MAX_VALUE) {
                 printf("Incorrect value\n");
               } else {
                 val = encode(val);
                 if (val >= 0) {
                   sft = (val >> 8) & 0x0f;
                   val = val & 0xff;
                 } else {
                   printf("Unencodable value\n");
                   val = abs(atoi(argv[2]));
                   sft = mln(val);
                   printf("val %d : ", (val & (0xFF << sft)));
                   val >>= sft;
                   sft = ((0x20 - sft) >> 1) & 0x0F;
                 }
#ifdef _DEBUG_
                   printf("shift 0x%X val 0x%02X : ", sft, val);
#endif
                   buf[0] = val;
                   buf[1] = reg + sft;
#ifdef _DEBUG_
                   printf("%02X %02X %02X %02X : ", buf[0], buf[1], buf[2], buf[3]);
#endif
                   fseek(fd, adr, SEEK_SET);
                   if (fwrite(&buf, 2, 1, fd)) {
                     printf("set\n");
                   } else {
                     printf("error!\n");
                   }
               }
             } // set end
           } else {
             printf("Incorrect file!\n");
           }
         } else {
           printf("Incorrect file size!\n");
         }
       } else {
         printf("Unable to get file!\n");
       }
     } else {
       printf("Unable to open file!\n");
     }
     if (fd) {
       if (argc == 3)
         fflush(fd);
      fclose(fd);
     }
   } else {
     printf("RMM bitrate patcher ver:%s for %s by MP77V 4pda.ru %s\n", VERSION, FW_VERSION, PATCH_DATE);
     printf("\t %s <ch> - get current value for channel [0,1]\n\t %s <ch> <val> - set new value (%d - %d)\n", argv[0], argv[0], MIN_VALUE, MAX_VALUE);
   }

   return 0;
}
