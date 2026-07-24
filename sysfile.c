//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

#define SORT_BUFSZ PGSIZE
#define SORT_MAX_NUMS 1024
#define SORT_OUTPATH_MAX 128

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
static int
unlink_path(char *path)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ];
  uint off;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();
  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  char *path;

  if(argstr(0, &path) < 0)
    return -1;
  return unlink_path(path);
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

static int
is_space(char c)
{
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static void
sort_ints(int *a, int n)
{
  int i;

  for(i = 1; i < n; i++){
    int key = a[i];
    int j = i - 1;
    while(j >= 0 && a[j] > key){
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = key;
  }
}

static int
append_suffix_user(const char *src, char *dst, int dstsz, const char *suf)
{
  int i = 0;
  int j = 0;
  char c;

  for(;; i++){
    if(i >= dstsz - 1)
      return -1;
    if(fetchbyte((uint)(src + i), &c) < 0)
      return -1;
    dst[i] = c;
    if(c == 0)
      break;
  }

  while(suf[j]){
    if(i >= dstsz - 1)
      return -1;
    dst[i++] = suf[j++];
  }
  dst[i] = 0;
  return 0;
}

static int
int_to_line(int v, char *dst, int dstsz)
{
  char tmp[16];
  int n = 0;
  int i;
  uint x;

  if(dstsz < 3)
    return -1;

  if(v < 0){
    dst[n++] = '-';
    x = (uint)(-(v + 1)) + 1;
  } else {
    x = (uint)v;
  }

  i = 0;
  do{
    if(i >= (int)sizeof(tmp))
      return -1;
    tmp[i++] = '0' + (x % 10);
    x /= 10;
  } while(x > 0);

  if(n + i + 1 >= dstsz)
    return -1;

  while(i > 0)
    dst[n++] = tmp[--i];
  dst[n++] = '\n';
  return n;
}

static struct file*
open_kernel_file_ro(char *path)
{
  struct inode *ip;
  struct file *f;

  begin_op();
  if((ip = namei(path)) == 0){
    end_op();
    return 0;
  }
  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return 0;
  }
  if((f = filealloc()) == 0){
    iunlockput(ip);
    end_op();
    return 0;
  }
  f->type = FD_INODE;
  f->readable = 1;
  f->writable = 0;
  f->ip = ip;
  f->off = 0;
  iunlock(ip);
  end_op();
  return f;
}

static struct file*
create_kernel_file_wo(char *path)
{
  struct inode *ip;
  struct file *f;

  // Make repeated runs deterministic: xv6 O_CREATE does not truncate an
  // existing file, so remove an old output before creating the new one.
  unlink_path(path);

  begin_op();
  if((ip = create(path, T_FILE, 0, 0)) == 0){
    end_op();
    return 0;
  }
  if((f = filealloc()) == 0){
    iunlockput(ip);
    end_op();
    return 0;
  }
  f->type = FD_INODE;
  f->readable = 0;
  f->writable = 1;
  f->ip = ip;
  f->off = 0;
  iunlock(ip);
  end_op();
  return f;
}

int
sys_sort_numbers(void)
{
  char *path;
  char buf[128];
  char outpath[SORT_OUTPATH_MAX];
  char line[32];
  int *nums = 0;
  int count = 0;
  int nread;
  int i;
  int sign = 1;
  int value = 0;
  int seen_digit = 0;
  int in_number = 0;
  int status = -1;
  struct file *fp_in = 0;
  struct file *fp_out = 0;

  if(argstr(0, &path) < 0)
    return -1;

  if(append_suffix_user(path, outpath, sizeof(outpath), ".kernel.sorted") < 0)
    return -1;

  nums = (int*)kalloc();
  if(nums == 0)
    return -1;

  fp_in = open_kernel_file_ro(path);
  if(fp_in == 0)
    goto done;

  while((nread = fileread(fp_in, buf, sizeof(buf))) > 0){
    for(i = 0; i < nread; i++){
      char ch = buf[i];

      if(is_space(ch)){
        if(in_number){
          if(!seen_digit)
            goto done;
          if(count >= SORT_MAX_NUMS)
            goto done;
          nums[count++] = sign * value;
          in_number = 0;
          seen_digit = 0;
          sign = 1;
          value = 0;
        }
        continue;
      }

      if(!in_number){
        in_number = 1;
        seen_digit = 0;
        sign = 1;
        value = 0;
        if(ch == '-'){
          sign = -1;
          continue;
        }
        if(ch == '+')
          continue;
      }

      if(ch < '0' || ch > '9')
        goto done;
      seen_digit = 1;
      value = value * 10 + (ch - '0');
    }
  }
  if(nread < 0)
    goto done;

  if(in_number){
    if(!seen_digit || count >= SORT_MAX_NUMS)
      goto done;
    nums[count++] = sign * value;
  }

  fileclose(fp_in);
  fp_in = 0;

  sort_ints(nums, count);

  fp_out = create_kernel_file_wo(outpath);
  if(fp_out == 0)
    goto done;

  for(i = 0; i < count; i++){
    int n = int_to_line(nums[i], line, sizeof(line));
    if(n <= 0 || filewrite(fp_out, line, n) != n)
      goto done;
  }

  status = 0;

done:
  if(fp_in)
    fileclose(fp_in);
  if(fp_out)
    fileclose(fp_out);
  if(nums)
    kfree((char*)nums);
  return status;
}
