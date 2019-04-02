/**
 * Don Freiday
 * CPSC340 : Steganography
 * Usage:
 * stega hide <text file> <bmp file>
 * stega show <bmp file>
 **/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define BMP_DATA_START 54 // Skip header
#define ASCII_STX 0x02    // Start of text
#define ASCII_ETX 0x03    // End of text

typedef unsigned char u8;

/*******************************************************
 * Read file into buffer
 *******************************************************/
u8 read_file(char *name, u8 **buf, int *size) {
  // Open file
  int fd = 0;
  if ((fd = open(name, O_RDONLY, 0)) < 0) {
    printf("file open failed: %s\n", strerror(errno));
    return FALSE;
  }

  // Get file size
  *size = lseek(fd, 0, SEEK_END);
  if (*size < 0) {
    printf("file seek failed: %s\n", strerror(errno));
    return FALSE;
  }
  if (lseek(fd, 0, SEEK_SET) < 0) {
    printf("file seek failed: %s\n", strerror(errno));
    return FALSE;
  }

  // Allocate and initialize buffer
  *buf = malloc(sizeof(char) * *size);
  if (*buf == NULL) {
    printf("failed to allocate %d bytes: %s\n", *size, strerror(errno));
    return FALSE;
  }

  // Read file, print buffer
  if (read(fd, *buf, *size) < 0) {
    printf("file read failed: %s\n", strerror(errno));
    free(buf);
    return FALSE;
  }

  // Clean up
  close(fd);
  return TRUE;
}

/*******************************************************
 * Write buffer to file
 *******************************************************/
u8 write_file(char *name, u8 *buf, int size) {
  // Open file
  int fd = 0;
  if ((fd = open(name, O_CREAT | O_RDWR, S_IRWXU)) < 0) {
    printf("file open failed: %s\n", strerror(errno));
    return FALSE;
  }

  // Write file
  if (write(fd, buf, size) < 0) {
    printf("file write failed: %s\n", strerror(errno));
    return FALSE;
  }

  // Clean up
  close(fd);
  return TRUE;
}

/*******************************************************
 * Encode character in LSB of 8bytes at offset in BMP
 *******************************************************/
void encodeChar(u8* bmp, int* offset, u8 character) {
  // For each bit in character
  for (u8 pos = 0; pos < 8; pos++) {
    // Set LSB of BMP byte depending on bit at pos in character
    if (character & (1 << pos))
      bmp[*offset] |= 0x01; // 0000 0001
    else
      bmp[*offset] &= 0xFE; // 1111 1110
    (*offset)++; // Next BMP byte
  }
}

/*******************************************************
 * Hide text file in bmp file
 *******************************************************/
void hide(char **argv) {
  u8* bmp = NULL, *txt = NULL;
  int bmp_size, txt_size;

  // Load files into memory
  if (!read_file(argv[2], &txt, &txt_size)) {
    printf("Error loading %s\n", argv[2]);
    return;
  }
  if (!read_file(argv[3], &bmp, &bmp_size)) {
    printf("Error loading %s\n", argv[3]);
    return;
  }

  // Is the BMP big enough to contain the text?
  // txt_size + 1 to account for ETX marker
  if ((bmp_size - BMP_DATA_START) / 8 < txt_size + 1) {
    printf("BMP is too small to hide data\n");
    return;
  }

  // Skip header
  int bmp_offset = BMP_DATA_START;

  // Mark start of hidden text with ASCII STX character
  hideChar(bmp, &bmp_offset, ASCII_STX);

  // Encode each character from txt in bmp
  for (int txt_idx = 0; txt_idx < txt_size; txt_idx++) {
    hideChar(bmp, &bmp_offset, txt[txt_idx]);
  }

  // Mark end of text with ASCII ETX character
  hideChar(bmp, &bmp_offset, ASCII_ETX);

  // Write bmp to new file
  if (!write_file("out.bmp", bmp, bmp_size)) {
    printf("Error writing BMP file\n");
  }

  // Cleanup
  free(bmp);
  free(txt);
}

/*******************************************************
 * Decode character from LSB of 8bytes at offset in BMP
 *******************************************************/
u8 decodeChar(u8* bmp, int* offset) {
    u8 c = 0;
    // For each bit in character
    for (u8 pos = 0; pos < 8; pos++) {
      // Write LSB of BMP byte to bit at pos in character
      if (bmp[*offset] & 0x01)
        c |= (1 << pos);
      (*offset)++; // Next byte
    }
    return c;
}

/*******************************************************
 * Show hidden text in bmp
 *******************************************************/
void show(char **argv) {
  u8 *bmp = NULL;
  int bmp_size;

  // Load file into memory
  if (!read_file(argv[2], &bmp, &bmp_size)) {
    printf("Error loading %s\n", argv[2]);
    if (bmp)
      free(bmp);
    return;
  }

  // Skip header
  int bmp_offset = BMP_DATA_START;

  // Check for STX marker denoting hidden message
  if(decodeChar(bmp, &bmp_offset) != ASCII_STX) {
      free(bmp);
      return;
  }

  // Decode until ETX reached
  while (bmp_offset < bmp_size) {
    u8 c = decodeChar(bmp, &bmp_offset);
    if (c == ASCII_ETX)
      break;
    if (c < 128) // Don't print non-ASCII values
      printf("%c", c);
  }

  // Clean up
  free(bmp);
}

/*******************************************************
 * main
 *******************************************************/
int main(int argc, char *argv[]) {
  if (argc == 3 && strncmp(argv[1], "show", 4) == 0)
    show(argv);
  else if (argc == 4 && strncmp(argv[1], "hide", 4) == 0)
    hide(argv);
  else
    printf(
        "Usage:\n stega hide <text file> <bmp file>\n stega show <bmp file>\n");

  return 0;
}