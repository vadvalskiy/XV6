// The prot argument describes the desired memory protection of the
// mapping (and must not conflict with the open mode of the file).

//hash defines should be unique for bitwise operations!!!

#define PROT_EXEC 0x1
// Pages may be executed.

#define PROT_READ 0x2 
// Pages may be read.

#define PROT_WRITE 0x4
// Pages may be written.

#define PROT_NONE 0x0
// Pages may not be accessed.


// The flags argument determines whether updates to the mapping are
//        visible to other processes mapping the same region, and whether
//        updates are carried through to the underlying file. 

#define MAP_SHARED   0x01 
// Updates to the mapping are visible to other processes mapping the same region

#define MAP_PRIVATE 0x02 
// Create a private copy-on-write mapping.

#define MAP_ANONYMOUS  = 0x20 //(0b100000
// The mapping is not backed by any file; its contents are initialized to zero.


// prot = PROT_READ | PROT_WRITE = 0x3
// flags = MAP_ANON | MAP_SHARED = 0x21
// These are just integer bitmasks, not arrays.

struct mmapdata
{
    void *addr;   
    int length;   
    int prot;    
    int flags;    
    int fd;      
    int offset;
};
