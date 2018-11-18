/* headers used for mmap */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <unistd.h>
/* end of headers used for mmap */

static int mapoption = MAP_SHARED;

char * mmap_infile(char * filename, int * binfile_len)
{
  int fd;
  char * data;
  struct stat statbuf;

  if ((fd = open (filename, O_RDWR)) < 0){
     perror("mmap_binfile can't open file for reading");
     return NULL;
  }
 
  /* find size of input file */
  if (fstat (fd,&statbuf) < 0){
     perror("fstat error");
     return NULL;
  }

  (*binfile_len) = statbuf.st_size;

  /* mmap the input file */
  if ((data = mmap (0, statbuf.st_size, PROT_READ|PROT_WRITE, mapoption, fd, 0))
      == (caddr_t) -1){
     perror("mmap error");
     return NULL;
  }

  return data;

}


char * mmap_outfile(char * filename, int binfile_len)
{

  int fd;
  char * dst;

  /* open/create the output file */
  if ((fd = open (filename, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
    perror("mmap_outfile open error");
    return NULL;
  }

 /* go to the location corresponding to the last byte */
  if (lseek (fd, binfile_len - 1, SEEK_SET) == -1) {
    perror("mmap_outfile lseek error");
    return NULL;
  }
 
 /* write a dummy byte at the last location */
  if (write (fd, "", 1) != 1) {
    perror("mmap_outfile write error");
    return NULL;
  }

  /* mmap the output file */
  if ((dst = mmap (0, binfile_len, PROT_READ | PROT_WRITE, mapoption, fd, 0))
      == (caddr_t) -1) {
    perror("mmap_outfile mmap error");
    return (NULL);
  }

  return dst;
}
