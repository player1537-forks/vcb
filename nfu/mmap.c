Here is the example of using mmap() to implement file copy (e.g., UNIX
cp) that I discussed in class. 

Two different programs can also map the same file, for example, and
use this as a form of inter-process communication (but making sure to
control concurrency, e.g., using semaphores).

Note how we write a byte at the last location (statbuf.st_size - 1) of
the output file here to extend its size before mmap()ing the output
file. This (dummy) byte will be overwritten when we memcpy().


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>

int main (int argc, char *argv[])
{
 int fdin, fdout;
 char *src, *dst;
 struct stat statbuf;

 if (argc != 3)
   err_quit ("usage: a.out <fromfile> <tofile>");

 /* open the input file */
 if ((fdin = open (argv[1], O_RDONLY)) < 0)
   err_sys ("can't open %s for reading", argv[1]);

 /* open/create the output file */
 if ((fdout = open (argv[2], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0)
   err_sys ("can't create %s for writing", argv[2]);

 /* find size of input file */
 if (fstat (fdin,&statbuf) < 0)
   err_sys ("fstat error");

 /* go to the location corresponding to the last byte */
 if (lseek (fdout, statbuf.st_size - 1, SEEK_SET) == -1)
   err_sys ("lseek error");
 
 /* write a dummy byte at the last location */
 if (write (fdout, "", 1) != 1)
   err_sys ("write error");

 /* mmap the input file */
 if ((src = mmap (0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0))
   == (caddr_t) -1)
   err_sys ("mmap error for input");

 /* mmap the output file */
 if ((dst = mmap (0, statbuf.st_size, PROT_READ | PROT_WRITE,
   MAP_SHARED, fdout, 0)) == (caddr_t) -1)
   err_sys ("mmap error for output");

 /* this copies the input file to the output file */
 memcpy (dst, src, statbuf.st_size);

} /* main */





