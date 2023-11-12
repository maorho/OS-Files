#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "riscv.h"
#include "defs.h"

struct {
  struct spinlock lock;
  uint8 lfsr;
} rand;

uint8 lfsr_char(uint8 lfsr)
{
  uint8 bit;
  bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01;
  lfsr = (lfsr >> 1) | (bit << 7);
  return lfsr;
}

int
randomwrite(int fd, uint64 src, int n)
{
  if (n == 1) {
    char c;
    if (either_copyin(&c, fd, src, 1) == -1)
        return -1;
    acquire(&rand.lock);
    rand.lfsr = c;
    release(&rand.lock);
    return 0;
  }
  return -1;
}

int
randomread(int user_dst, uint64 dst, int n)
{
  if (n <= 0)
    return 0;
  acquire(&rand.lock);
  for (int i = 0; i < n; i++) {
    rand.lfsr = lfsr_char(rand.lfsr);
    if(either_copyout(user_dst, dst + i, &rand.lfsr, 1) == -1) {
      release(&rand.lock);
      return i;
    }
  }
  release(&rand.lock);
  return n;
}

void
randominit(void)
{
  initlock(&rand.lock, "rand");
  rand.lfsr = 0x2A;
  devsw[RANDOM].read = randomread;
  devsw[RANDOM].write = randomwrite;
}