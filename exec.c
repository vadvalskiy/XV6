#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  //cprintf("exec called: %s\n", argv[0]);
  char *s, *last, *ustackpg;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
  struct elfprof *ep = 0;

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto release_ilock;
  if(elf.magic != ELF_MAGIC)
    goto release_ilock;

  if((pgdir = setupkvm()) == 0)
    goto release_ilock;

  // Record relevant data for loadable segments.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto release_ilock;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto release_ilock;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto release_ilock;
    if(ph.vaddr % PGSIZE != 0)
      goto release_ilock;
    if((ep = recorduvm(ep, ph.vaddr, ph.off, ph.filesz, ph.memsz)) == 0)
      goto release_ilock;
  }
  iunlock(ip);
  end_op();

  // Update the program's virtual size.
  sz = ep->end_vaddr;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  /* excuse the two stack pages from demand paging
    as these are immediately required by exec for argv */
  sz = PGROUNDUP(sz);
  for(i = 0; i < 2; i++){
    if((ustackpg = kalloc()) == 0)
      goto bad;
    mappages(pgdir, (char *)sz, PGSIZE, V2P(ustackpg), PTE_P|PTE_W|PTE_U);
    sz += PGSIZE;
  }
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  // inode to elf file of process
  if(curproc->elf)
    iput(curproc->elf);
  curproc->elf = ip;
  // Info page about loadable segments
  freeprof(curproc->ep);
  curproc->ep = ep;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 release_ilock:
  iunlock(ip);
 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iput(ip);
    end_op();
  }
  return -1;
}
