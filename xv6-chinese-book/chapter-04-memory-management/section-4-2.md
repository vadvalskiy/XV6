## 4.2 页表与虚拟内存机制 (Page Tables and Virtual Memory Mechanisms)

在上一节中，我们学习了 xv6 内核如何管理和分配物理内存页。本节将深入探讨这些物理页是如何通过页表机制被组织和映射，从而为内核自身以及每个用户进程构建起独立的虚拟地址空间的。

### 1. 分页硬件回顾 (Recap of Paging Hardware)

正如我们在 3.1 节（进程地址空间）中提到的，现代 CPU 包含一个**内存管理单元 (Memory Management Unit, MMU)**，它负责将程序使用的虚拟地址转换为实际的物理地址。xv6 运行在 x86 架构上，并利用其分页机制。回顾一下关键组件：

*   **页 (Pages)**：虚拟地址空间和物理内存都被划分为固定大小的块，即页（xv6 中为 4KB）。
*   **页表 (Page Tables)**：这是一种由操作系统维护、由 MMU 使用的数据结构，用于存储虚拟页号到物理页号的映射关系。
*   **两级页表结构 (Two-Level Page Tables)**：xv6 在 x86 上使用两级页表：
    *   **页目录 (Page Directory)**：第一级表。每个进程有一个页目录。CPU 中的 `%cr3` 寄存器（页目录基址寄存器）指向当前活动进程的页目录的物理地址。页目录包含多个**页目录项 (Page Directory Entries, PDEs)**。
    *   **页表 (Page Table)**：第二级表。每个 PDE 可以指向一个页表。页表包含多个**页表项 (Page Table Entries, PTEs)**。
    *   每个 PTE 最终指向一个 4KB 的物理页帧，或者标记该虚拟页未被映射。
*   **页表项标志位 (PTE Flags)**：每个 PDE 和 PTE 都包含一些重要的标志位，用于控制对相应内存区域的访问：
    *   **`PTE_P` (Present)**：如果设置，表示该项有效，即对应的页（或页表）存在于物理内存中。如果未设置，访问该虚拟地址会导致页错误 (Page Fault)。
    *   **`PTE_W` (Writable)**：如果设置，表示该页允许写入。如果未设置，尝试写入该页会导致页错误。
    *   **`PTE_U` (User)**：如果设置，表示该页允许用户态代码访问。如果未设置，只有当 CPU 处于内核态时才能访问该页；用户态访问会导致页错误。

MMU 通过查找页目录和页表，将一个32位虚拟地址分解为：页目录索引（高10位）、页表索引（中间10位）和页内偏移（低12位），从而找到对应的物理地址。

### 2. 内核地址空间 (Kernel Address Space)

操作系统内核本身也运行在虚拟地址空间中。xv6 的内核地址空间设计相对简单但有效。

#### a. 内核如何拥有地址空间

当计算机启动时，xv6 的引导加载程序首先会进行一些基本的硬件初始化，然后开启一个临时的、简单的内存映射（通常是直接映射物理内存的前几MB），以便加载和运行内核的初始代码。一旦内核的核心部分开始执行（如 `main()` 函数），它就会着手建立自己正式的、更完善的地址空间和页表。

#### b. 物理内存的直接映射

xv6 内核地址空间的一个关键特性是它将大部分可用的物理内存**直接映射 (Direct Mapping)** 到一个较高的虚拟地址范围。
*   这个较高的虚拟地址范围从 `KERNBASE` 开始。`KERNBASE` 在 `memlayout.h` 中定义，对于 x86 通常是 `0x80000000` (2GB)。
*   映射规则是：**内核虚拟地址 = 物理地址 + `KERNBASE`**。
    *   例如，物理地址 `0x0` 被映射到内核虚拟地址 `KERNBASE` (`0x80000000`)。
    *   物理地址 `0x100000` (1MB) 被映射到内核虚拟地址 `KERNBASE + 0x100000` (`0x80100000`)。
*   这种直接映射覆盖了从物理地址 `0` 到 `PHYSTOP`（系统可用的最高物理内存地址）的范围。
*   **好处**：
    *   内核可以方便地访问任何物理内存位置，只需通过 `KERNBASE` 偏移即可。
    *   `kalloc()` 返回的是物理页的内核虚拟地址，内核可以直接使用这个地址。
    *   简化了内核代码对物理内存的操作。

#### c. `kvmalloc()`：构建内核页表

内核的地址空间是通过一个专门的内核页目录（`kpgdir`）来管理的。这个页目录在系统启动时由 `kvmalloc()` 函数（位于 `kernel/vm.c`）创建和初始化。

```c
// kernel/vm.c
// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns new allocated size, or 0 if it fails.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  // ... (详情见后文或源码)
}

// kernel/vm.c
// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  // 分配一页物理内存作为页目录
  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE); // 清零页目录

  // 检查 PHYSTOP 是否超过 KERNBASE (直接映射区域不能太大)
  if (P2V(PHYSTOP) > (void*)DEVSPACE) // DEVSPACE 通常是 KERNBASE 下的一个区域
    panic("PHYSTOP too high");

  // 遍历 kmap 数组，映射内核所需的各个物理内存区域
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start, 
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir); // 如果映射失败，释放已分配的页目录
      return 0;
    }
  return pgdir;
}

// kernel/vm.c (setupkvm 的旧称，在某些版本中可能是 kvmalloc)
// 在现代 xv6 (riscv/x86) 中，setupkvm 通常是创建页目录并映射内核空间的主要函数
// 而 kvmalloc 这个名字可能指代更早期的或特定用途的函数。
// 本节主要关注 setupkvm 的功能，它建立了内核的核心映射。
// 如果存在一个 kvmalloc() 专门用于初始化 kpgdir，其核心也是调用 mappages。
// 我们假设 kvmalloc 的核心逻辑与 setupkvm 类似，或者 setupkvm 就是其现代体现。

// 我们用 kpgdir 指代由 setupkvm 创建的内核页目录
static pde_t *kpgdir; // 全局变量，存储内核页目录的指针

void
kvmalloc(void)
{
  kpgdir = setupkvm(); // 调用 setupkvm 来构建内核页目录
  if (kpgdir == 0)
    panic("kvmalloc\n");
  // switchkvm(); // 早期可能在这里切换到 kpgdir，现代 xv6 在 mpmain 中切换
}
```

**`kvmalloc()`/`setupkvm()` 的核心步骤**：
1.  **分配页目录 (`kalloc()`)**: 调用 `kalloc()` 分配一个 4KB 的物理页作为内核的页目录。如果失败，则 panic（因为内核无法为自己建立地址空间）。
2.  **清零页目录**: 将刚分配的页目录内容全部清零。
3.  **定义映射区域 (`kmap`)**: xv6 使用一个 `struct kmap` 数组（定义在 `kernel/vm.c` 的 `kmap` 静态数组或 `kernel/memlayout.h` 中）来描述内核需要映射的关键物理内存区域。`kmap` 数组的每个条目指定了：
    *   `virtual address start (k->virt)`：这段区域在内核虚拟地址空间中的起始地址。
    *   `physical address start (k->phys_start)`：对应的物理内存起始地址。
    *   `physical address end (k->phys_end)`：对应的物理内存结束地址。
    *   `permissions (k->perm)`：该区域的页表项权限（如 `PTE_W` 表示可写）。
    `kmap` 通常会包含以下映射：
    *   **内核代码和只读数据**：从 `KERNBASE` 开始，映射到物理地址 `V2P(KERNBASE)`（即物理地址0）开始的内核代码和只读数据区。权限通常是只读（`PTE_W` 不设置）。
    *   **内核读写数据和BSS**：紧随其后，映射到相应的物理内存区域。权限是可写。
    *   **所有其他物理内存直到 `PHYSTOP`**：映射到 `KERNBASE + physical_address`，权限通常是可写。这实现了对所有可用物理内存的直接映射。
    *   **I/O 设备空间 (MMIO)**：某些硬件设备的控制寄存器是通过内存映射 I/O (Memory-Mapped I/O) 访问的。这些特定的物理地址范围（如 `DEVSPACE` 区域）也会被映射到内核虚拟地址空间中，通常设置为不可缓存且可读写。

4.  **调用 `mappages()` 进行映射**：
    对于 `kmap` 数组中的每一个条目，`setupkvm()` 会调用 `mappages(pgdir, va, size, pa, perm)`。
    *   `pgdir`: 要填充的页目录（即 `kpgdir`）。
    *   `va`: 虚拟地址的起始。
    *   `size`: 要映射的区域大小。
    *   `pa`: 物理地址的起始。
    *   `perm`: 该区域的 PTE 权限 (如 `PTE_W`, `PTE_U` - 对于内核映射，`PTE_U` 通常不设置)。

**`mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)` 函数**:
*   这个静态函数（在 `kernel/vm.c` 中）负责在一个给定的页目录 `pgdir` 中，为一段虚拟地址范围 `[va, va+size)` 建立到物理地址范围 `[pa, pa+size)` 的映射，并设置指定的权限 `perm`。
*   它会遍历这个范围内的每一个虚拟页。
*   对于每个虚拟页：
    1.  调用 `walkpgdir(pgdir, va_page, 1)` 获取该虚拟页对应的 PTE 的地址。`walkpgdir` 如果发现对应的页表尚不存在，会因为第三个参数 `alloc=1` 而分配一个新的物理页作为页表，并将其链接到页目录中。
    2.  如果 `walkpgdir` 失败（例如 `kalloc` 分配页表失败），`mappages` 返回错误。
    3.  如果该 PTE 已经存在（`*pte & PTE_P`），则 `panic`，因为内核初始化时不应出现重复映射。
    4.  将物理页地址 `pa_page` 和权限 `perm | PTE_P`（确保 Present 位被设置）填入这个 PTE 中。
*   `mappages` 确保了虚拟地址、物理地址和大小都按页对齐处理。

初始化完成后，`kpgdir` 就包含了内核地址空间的完整映射。在多处理器系统中，每个 CPU 启动后都会执行 `switchkvm()`（或类似操作，如在 `mpmain` 中加载 `kpgdir` 到 `%cr3`），使其 MMU 使用这个共享的内核页目录。

### 3. 用户进程页表 (User Process Page Tables)

每个用户进程都需要自己独立的地址空间，因此每个进程都有其自己的页目录和相应的页表。

#### a. `proc->pgdir`：进程的页目录

`struct proc` 结构体中有一个成员 `pde_t *pgdir;`，它指向该进程的页目录的（内核虚拟）地址。当内核调度一个用户进程运行时，它会调用 `switchuvm(proc)`（或类似函数 `lcr3(V2P(proc->pgdir))`），将该进程页目录的物理地址加载到 `%cr3` 寄存器中，从而激活该进程的地址空间。

#### b. 创建进程时的页表管理

1.  **`allocproc()`**: 当通过 `fork()` 创建一个新进程时，`allocproc()`（在 `kernel/proc.c` 中）首先会为新的 `struct proc` 分配空间。此时，`proc->pgdir` 通常是 `0` 或未初始化。

2.  **`setupkvm()`**: `allocproc()` 会调用 `setupkvm()` 来为新进程创建一个页目录，并在这个页目录中**仅仅复制内核空间的映射**。
    *   这意味着子进程的页目录一开始只包含了对内核代码、数据和直接映射物理内存的 PDE，这些 PDE 与 `kpgdir` 中的相应 PDE 是相同的（指向相同的物理页表或物理页帧）。
    *   此时，子进程的用户空间（虚拟地址 0 到 `KERNBASE`）是完全空的，没有任何映射。
    *   如果 `setupkvm` 失败，`allocproc` 会失败。

3.  **`inituvm()` (用于第一个用户进程 `initproc`)**:
    *   对于系统创建的第一个用户进程 `initproc`，`userinit()`（在 `kernel/proc.c` 中）在调用 `allocproc()` 之后，会调用 `inituvm(proc->pgdir, initcode_start, initcode_size)`。
    *   `inituvm()`（在 `kernel/vm.c` 中）负责为 `initproc` 的极小代码（`initcode.S`，一段用于执行 `exec("/init", ...)` 的汇编代码）分配一页物理内存，并将这段代码复制到该页。
    *   它调用 `mappages()` 将这一页物理内存映射到 `initproc` 页目录的虚拟地址 0 处，权限为可读、可执行、用户可访问 (`PTE_W` 不设置, `PTE_U` 设置)。
    *   `inituvm` 会将 `proc->sz` 设置为这一页的大小。

4.  **`copyuvm()` (用于 `fork()`)**:
    *   当一个已存在的用户进程调用 `fork()` 时，在 `allocproc()` 为子进程创建了包含内核映射的页目录之后，`fork()` 函数（在 `kernel/proc.c` 中）会调用 `copyuvm(child_pgdir, parent_pgdir, parent_size)`。
    *   `copyuvm()`（在 `kernel/vm.c` 中）负责将父进程的用户空间内存完整地复制到子进程。它会遍历父进程页目录中所有指向用户空间（低于 `KERNBASE`）的有效 PDE。
    *   对于每个这样的 PDE，如果它指向一个页表：
        *   `copyuvm` 会递归地遍历该页表中的所有有效 PTE。
        *   对于每个有效的 PTE，它会：
            1.  调用 `kalloc()` 为子进程分配一个新的物理页。
            2.  将父进程对应物理页的内容复制到这个新分配的子进程物理页中。
            3.  调用 `mappages()` 在子进程的相应页表中创建一个新的 PTE，将子进程的虚拟地址映射到这个新的物理页，并复制父进程 PTE 的权限（如 `PTE_W`, `PTE_U`）。
    *   这样，子进程就拥有了与父进程完全相同的用户空间内存副本，但它们位于不同的物理页帧上。

#### c. PDE 和 PTE 的结构

在 xv6 (x86) 中，`pde_t` (页目录项) 和 `pte_t` (页表项) 都是 32 位无符号整数 (`uint`)。它们的结构（位域）由 x86 架构定义：
*   **位 0**: `PTE_P` (Present) - 1 表示存在，0 表示不存在。
*   **位 1**: `PTE_W` (Writable) - 1 表示可写，0 表示只读。
*   **位 2**: `PTE_U` (User) - 1 表示用户态可访问，0 表示仅内核态可访问。
*   **位 3**: `PTE_PWT` (Page Write-Through) - 控制写策略。
*   **位 4**: `PTE_PCD` (Page Cache Disable) - 控制是否缓存该页。
*   **位 5**: `PTE_A` (Accessed) - CPU 在访问该页时会自动设置此位。
*   **位 6**: `PTE_D` (Dirty) - (仅 PTE) CPU 在写入该页时会自动设置此位。
*   **位 7**: `PTE_PS` (Page Size) - (仅 PDE) 如果设置，PDE 直接映射一个 4MB 的大页，而不是指向一个页表。xv6 不使用大页。
*   **位 12-31**: 物理基地址 (Physical Base Address) - 页对齐的（低12位为0），指向页表或物理页帧的起始物理地址。

xv6 在 `mmu.h` 中定义了这些标志位的宏。

#### d. `walkpgdir()`：查找 PTE

内核经常需要为一个给定的虚拟地址找到其对应的 PTE，以便检查映射、修改权限或获取物理地址。`walkpgdir(pde_t *pgdir, const void *va, int alloc)` 函数（在 `kernel/vm.c` 中）实现了这个功能。

```c
// kernel/vm.c
// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  // va 的高10位是页目录索引 (PDX)
  pde = &pgdir[PDX(va)]; 

  if(*pde & PTE_P){ // 如果 PDE 存在 (指向一个页表)
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde)); // 获取页表的内核虚拟地址
  } else { // PDE 不存在，需要分配一个新的页表
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0) // 如果不允许分配，或kalloc失败
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE); // 清零新分配的页表
    // The permissions here are overly generous, but they can
    // be further restricted by the PTE in the page table.
    // 将新页表的物理地址和权限填入 PDE
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U; 
  }
  // va 的中间10位是页表索引 (PTX)，返回该 PTE 的地址
  return &pgtab[PTX(va)]; 
}
```
*   它首先使用虚拟地址 `va` 的高 10 位（`PDX(va)`）作为索引在页目录 `pgdir` 中找到对应的 PDE。
*   如果这个 PDE 是有效的（`PTE_P` 位被设置），它就从中提取出页表的物理地址，转换为内核虚拟地址，得到页表指针 `pgtab`。
*   如果 PDE 无效：
    *   若 `alloc` 参数为 0（表示不允许分配），则返回 `NULL`。
    *   若 `alloc` 非 0，则调用 `kalloc()` 分配一个新的物理页作为页表。如果分配失败，返回 `NULL`。新分配的页表会被清零。然后，该新页表的物理地址和默认权限（`PTE_P | PTE_W | PTE_U`）会被填入当前的 PDE 中，使其生效。
*   最后，它使用虚拟地址 `va` 的中间 10 位（`PTX(va)`）作为索引，在页表 `pgtab` 中找到并返回目标 PTE 的地址。

### 4. 将内核空间映射到用户进程中

如前所述，当为用户进程创建页目录时 (`setupkvm`)，内核空间的映射（从 `KERNBASE` 到物理内存顶端以及 I/O 设备）会被复制到每个用户进程的页目录中。

**原因**：

*   **高效的系统调用和中断处理**：当用户进程通过系统调用或硬件中断陷入内核时，CPU 从用户态切换到内核态。此时，内核代码需要能够访问内核的数据结构（如进程表、文件表）以及当前进程的用户空间内存（例如，复制系统调用参数或结果）。如果内核空间没有映射在当前地址空间中，就需要进行昂贵的地址空间切换（切换到纯内核的页表，完成操作，再切换回用户的页表）。通过将内核映射到每个进程的地址空间的高位部分，内核代码在代表用户进程执行时，可以直接访问所需的所有数据，无需切换页表。
*   **共享内核**：所有进程共享同一份内核代码和数据。将内核空间映射到所有进程的页表中，确保了这种共享性。这些 PDE 都指向相同的物理页表（如果存在二级映射）或物理页帧。

**保护机制**：

虽然内核空间被映射到每个进程的地址空间，但用户态代码不能直接访问它。这是通过页表项中的 `PTE_U` (User) 标志位实现的：
*   对于内核空间的所有映射（无论是内核代码、数据还是直接映射的物理内存），其对应的 PDE 和/或 PTE 中的 `PTE_U` 位**都不会被设置**。
*   当 MMU 进行地址翻译时，如果 CPU 处于用户态，并且尝试访问一个 `PTE_U` 位为 0 的页面，MMU 会产生一个页错误，阻止这次访问。
*   只有当 CPU 处于内核态时（在系统调用或中断处理期间），才能访问 `PTE_U` 位为 0 的页面。

这样就保证了内核内存的安全性，防止用户程序破坏内核。

### 5. 加载可执行文件到内存 (`loaduvm`)

当 `exec()` 系统调用执行一个新程序时，它需要将 ELF 可执行文件的各个段（如代码段 `.text`，数据段 `.data`）加载到为该进程新分配的用户空间内存中。这个任务由 `loaduvm()` 函数（位于 `kernel/vm.c`）完成。

```c
// kernel/vm.c
// Load program segment from ip into virtual address va in page table pgdir.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
int
loaduvm(pde_t *pgdir, char *va, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint)va % PGSIZE != 0) // 虚拟地址必须页对齐
    panic("loaduvm: va must be page aligned");

  // 遍历要加载的每一页
  for(i = 0; i < sz; i += PGSIZE){
    // 获取当前虚拟地址 va+i 对应的 PTE
    if((pte = walkpgdir(pgdir, va + i, 0)) == 0) // 不允许分配，页必须已由 allocuvm 映射好
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte); // 从 PTE 获取物理地址

    // 确定要从文件中读取多少字节到当前物理页
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;

    // 调用 readi 从 inode 的 offset 处读取 n 字节到物理地址 pa
    if(readi(ip, P2V(pa), offset + i, n) != n) // P2V(pa) 将物理地址转为内核可直接访问的虚拟地址
      return -1; // 读取失败
  }
  return 0;
}
```
`exec()` 在调用 `loaduvm()` 之前，会先为每个需要加载的程序段调用 `allocuvm()`。`allocuvm` 的作用是：
1.  为该程序段覆盖的虚拟地址范围分配物理页（通过 `kalloc()`）。
2.  在进程的新页目录 `pgdir` 中建立这些虚拟页到新分配的物理页的映射，并设置好权限（例如，代码段只读，数据段可读写，两者都允许用户访问）。

然后，`exec()` 才调用 `loaduvm(pgdir, segment_va, inode, segment_offset_in_file, segment_filesz)`：
*   `pgdir`: 进程的新页目录。
*   `segment_va`: 该段在用户虚拟地址空间中的起始地址（必须页对齐）。
*   `inode`: 指向可执行文件的 inode。
*   `segment_offset_in_file`: 该段数据在 ELF 文件中的偏移量。
*   `segment_filesz`: 该段数据在文件中的实际大小。

`loaduvm` 的工作流程：
1.  它遍历从 `segment_va` 开始，长度为 `segment_filesz` 的区域，每次处理一页。
2.  对于每一虚拟页，它调用 `walkpgdir(pgdir, current_va, 0)` 来找到对应的 PTE。这里 `alloc` 参数为 0，因为 `allocuvm` 应该已经创建好了这些映射。如果找不到 PTE，说明之前的 `allocuvm` 有问题，会 panic。
3.  从有效的 PTE 中提取出该虚拟页映射的物理页地址 `pa`。
4.  计算当前页需要从文件中读取多少字节 `n`（最后一页可能不满一页）。
5.  调用 `readi(inode, P2V(pa), file_offset_for_current_page, n)` 从可执行文件的 `inode` 中，读取从 `file_offset_for_current_page` 开始的 `n` 字节数据，直接写入到物理页 `pa`（通过其内核虚拟地址 `P2V(pa)` 访问）。
6.  如果 `readi` 读取的字节数不等于期望的 `n`，则表示加载失败，返回 -1。

通过这个过程，ELF 文件的内容被准确地加载到了为新程序准备的、已映射好的物理内存页中。BSS 段（`filesz < memsz`）则依赖于 `kalloc` 分配的页是清零的（或 `allocuvm` 中显式清零的步骤）。

理解内核如何为自身和为每个用户进程管理页表，是掌握 xv6 虚拟内存系统的核心。这些机制确保了地址空间的隔离、内存保护，并为高级功能如 `fork` 和 `exec` 提供了基础。
