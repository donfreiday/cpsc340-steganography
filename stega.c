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
#define ASCII_ETX 0x03    // ASCII end of text

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
 * Hide text in bmp
 *******************************************************/
void hide(char **argv) {
  u8 *bmp_buf = NULL, *txt_buf = NULL;
  int bmp_size, txt_size;

  // Load files into memory
  if (!read_file(argv[2], &txt_buf, &txt_size)) {
    printf("Error loading %s\n", argv[2]);
    return;
  }
  if (!read_file(argv[3], &bmp_buf, &bmp_size)) {
    printf("Error loading %s\n", argv[3]);
    return;
  }

  // Is the BMP big enough to contain the text?
  // txt_size + 1 to account for ETX marker
  if ((bmp_size - BMP_DATA_START) / 8 < txt_size + 1) {
    printf("BMP is too small to hide data\n");
    return;
  }

  int bmp_idx = BMP_DATA_START;

  // Each character is encoded as the LSB in a block of 8 RGB bytes.
  for (int txt_idx = 0; txt_idx < txt_size; txt_idx++) {
    for (u8 pos = 0; pos < 8; pos++) {
      // Set LSB of BMP byte depending on bit at pos in text character
      if (txt_buf[txt_idx] & (1 << pos))
        bmp_buf[bmp_idx] |= 0x01; // 0000 0001
      else
        bmp_buf[bmp_idx] &= 0xFE; // 1111 1110
      bmp_idx++;
    }
  }

  // Mark end of text with ASCII ETX character
  for (u8 pos = 0; pos < 8; pos++) {
    // Set LSB of BMP byte depending on bit at pos in ETX character
    if (ASCII_ETX & (1 << pos))
      bmp_buf[bmp_idx] |= 0x01; // 0000 0001
    else
      bmp_buf[bmp_idx] &= 0xFE; // 1111 1110
    bmp_idx++;
  }

  // Write bmp to new file
  if (!write_file("out.bmp", bmp_buf, bmp_size)) {
    printf("Error writing BMP file\n");
  }

  // Cleanup
  free(bmp_buf);
  free(txt_buf);
}

/*******************************************************
 * Show hidden text in bmp
 *******************************************************/
void show(char **argv) {
  u8 *bmp_buf = NULL;
  int bmp_size;

  // Load file into memory
  if (!read_file(argv[2], &bmp_buf, &bmp_size)) {
    printf("Error loading %s\n", argv[2]);
    if (bmp_buf)
      free(bmp_buf);
    return;
  }
  
  // Each character is encoded as the LSB in a block of 8 RGB bytes.
  int bmp_idx = BMP_DATA_START;
  while (bmp_idx < bmp_size) {
    // Rebuild character bit by bit
    u8 c = 0;
    for (u8 pos = 0; pos < 8; pos++) {
      // Write LSB of BMP byte to bit at pos in character
      if (bmp_buf[bmp_idx] & 0x01)
        c |= (1 << pos);
      bmp_idx++;
    }
    if (c == ASCII_ETX)
      break;
    if (c < 128) // Don't print non-ASCII values
      printf("%c", c);
  }

  // Clean up
  free(bmp_buf);
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
    printf("Usage:\n stega hide <text file> <bmp file>\n stega show <bmp file>\n");

  return 0;
}