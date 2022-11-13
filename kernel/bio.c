// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13
#define HASHID(no) (no % NBUCKETS)

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;

  struct bucket buckets[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (struct bucket* buck=bcache.buckets; buck < bcache.buckets+NBUCKETS; buck ++) {
    char name[10];
    snprintf(name, sizeof(name), "bcache_%d", buck - bcache.buckets);
    initlock(&buck->lock, name);
    buck->head.prev = &buck->head;
    buck->head.next = &buck->head;
  }
  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  //  bcache.head.prev = &bcache.head;
  //  bcache.head.next = &bcache.head;
  //  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //    b->next = bcache.head.next;
  //    b->prev = &bcache.head;
  //    initsleeplock(&b->lock, "buffer");
  //    bcache.head.next->prev = b;
  //    bcache.head.next = b;
  //  }
  struct bucket* buck0 = bcache.buckets;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = buck0->head.next;
    b->prev = &buck0->head;
    initsleeplock(&b->lock, "buffer");
    buck0->head.next->prev = b;
    buck0->head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);
  uint bid = HASHID(blockno);
  //printf("bid: %d\n", bid);
  acquire(&bcache.buckets[bid].lock);
  // Is the block already cached?
  for(b = bcache.buckets[bid].head.next; b != &bcache.buckets[bid].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buckets[bid].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.lock);
  b = 0;
  struct buf * tmp;
  for(uint i=0; i<NBUCKETS; i++){
    uint id = (i + bid)%NBUCKETS;
    //printf("search bucket %d\n", id);
    for (tmp = bcache.buckets[id].head.next; tmp != &bcache.buckets[id].head;
         tmp = tmp->next) {
      if (tmp->refcnt == 0) {
        b = tmp;
        if (id != bid) {
          b->next->prev = b->prev;
          b->prev->next = b->next;
          b->next = bcache.buckets[bid].head.next;
          b->prev = &bcache.buckets[bid].head;
          bcache.buckets[bid].head.next->prev = b;
          bcache.buckets[bid].head.next = b;
        }
        break;
      }
    }
    if (b) {
      break;
    }
  }
  release(&bcache.lock);
  if (b) {
    b->refcnt = 1;
    b->blockno = blockno;
    b->dev = dev;
    b->valid = 0;
    release(&bcache.buckets[bid].lock);
    acquiresleep(&b->lock);
    return b;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  bunpin(b);

  //  acquire(&bcache.lock);
  //  b->refcnt--;
  //  if (b->refcnt == 0) {
  //    // no one is waiting for it.
  //    b->next->prev = b->prev;
  //    b->prev->next = b->next;
  //    b->next = bcache.head.next;
  //    b->prev = &bcache.head;
  //    bcache.head.next->prev = b;
  //    bcache.head.next = b;
  //  }
  //
  //  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  uint bid = HASHID(b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt++;
  release(&bcache.buckets[bid].lock);
}

void
bunpin(struct buf *b) {
  uint bid = HASHID(b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt--;
  release(&bcache.buckets[bid].lock);
}


