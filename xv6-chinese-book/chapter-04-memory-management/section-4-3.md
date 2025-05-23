## 4.3 页表切换与管理工具 (Page Table Switching and Management Utilities)

在理解了物理页分配和页表如何为内核及用户进程构建虚拟地址空间之后，本节将探讨操作系统如何在不同进程间切换地址空间（即切换页表），以及内核中用于操作和管理这些页表的关键工具函数。

### 1. 上下文切换与页表 (Context Switching and Page Tables)

操作系统通过让 CPU 在不同进程之间快速切换执行，来实现多任务的假象。这个切换过程称为**上下文切换 (Context Switch)**。由于每个进程都拥有自己独立的虚拟地址空间，由其自身的页目录和页表定义，因此在上下文切换时，操作系统必须确保 CPU 的 MMU 使用目标进程的页表来进行后续的地址翻译。

#### a. 为何需要切换页表？

*   **独立地址空间**：每个进程都有其私有的虚拟地址范围（从0到`KERNBASE`）。进程A的虚拟地址 `0x1000` 和进程B的虚拟地址 `0x1000` 通常映射到不同的物理内存位置，或者映射到相同物理内存但内容不同（例如共享库，尽管xv6中不常见）。
*   **MMU 的角色**：CPU 的 MMU 在进行地址翻译时，依赖于当前加载到 `%cr3` 寄存器（在x86中）的页目录的物理地址。如果不切换页表，MMU 仍然会使用前一个进程的页表来解释新进程的虚拟地址，这将导致新进程访问到错误的内存数据，破坏进程隔离性，并可能导致系统崩溃。

因此，当操作系统从一个用户进程切换到另一个用户进程时，必须更新 `%cr3` 寄存器，使其指向新进程的页目录。

#### b. `switchuvm(struct proc *p)`：切换到用户进程页表

当内核调度器（`scheduler()`）决定运行一个用户进程 `p` 时，它会调用 `switchuvm(p)` 函数（位于 `kernel/proc.c`，但其核心操作是加载页目录到 `%cr3`）。

```c
// kernel/proc.c (部分, 在 scheduler 函数内)
// ...
// p 是选中的要运行的进程
// cpu->proc = p; // 设置当前CPU运行的进程
// switchuvm(p); // 切换到该进程的地址空间
// ...

// switchuvm 的简化表示 (实际可能在 switchkvm 后，或直接用 lcr3)
void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0) // 进程必须有一个页目录
    panic("switchuvm: no pgdir");

  pushcli(); // 关中断，防止在切换过程中发生中断导致问题
  // 将进程 p 的页目录的物理地址加载到 %cr3 寄存器
  // V2P 会将内核虚拟地址转换为物理地址
  lcr3(V2P(p->pgdir)); 
  popcli(); // 开中断
}
```
*   **`switchuvm(p)`** 的核心任务是将进程 `p` 的页目录 (`p->pgdir`) 的物理地址加载到 `%cr3` 寄存器中。
*   `p->pgdir` 存储的是页目录的内核虚拟地址，`V2P()`宏将其转换为物理地址，因为 `%cr3` 需要物理地址。
*   一旦 `lcr3()` 执行完毕，MMU 就会开始使用进程 `p` 的页表进行所有后续的内存地址（特别是用户空间地址）的翻译。
*   **注意**：`switchuvm` 通常在内核已经决定要运行某个用户进程，并且即将通过 `swtch()` 切换到该用户进程的上下文（在其内核栈上执行，然后通过 `trapret` 返回用户态）之前被调用。在 `scheduler` 中，这个操作通常紧随 `cpu->proc = p;` 之后。

#### c. `switchkvm(void)`：切换回内核页表

当CPU从用户态陷入内核态时（例如，发生系统调用、硬件中断或异常），内核需要确保它在一个安全且一致的内存视图下运行。虽然用户进程的页表中已经包含了内核空间的映射，但在某些情况下，或者为了设计的清晰性，内核可能会显式切换回它自己的全局页目录 `kpgdir`。

```c
// kernel/vm.c (或 proc.c 中类似功能的封装)
void
switchkvm(void)
{
  // 将内核页目录 kpgdir (由 setupkvm/kvmalloc 创建) 的物理地址加载到 %cr3
  lcr3(V2P(kpgdir)); 
}
```
*   **`switchkvm()`** 将全局内核页目录 `kpgdir`（在 `kvmalloc()` 或 `setupkvm()` 中初始化）的物理地址加载到 `%cr3`。
*   **何时调用？**
    *   在 `scheduler()` 从一个用户进程切换到调度器自身的上下文（准备选择下一个进程）之后，会调用 `switchkvm()`。这样，调度器循环和空闲时的操作都在内核的规范地址空间下进行。
    *   在处理陷阱（`trap()` 函数）的早期，CPU 已经切换到内核栈，并保存了用户寄存器。此时，虽然当前 `%cr3` 仍然是用户进程的页目录（其中包含了内核映射），但如果需要一个“纯净”的内核视角，或者在某些没有当前用户进程上下文的场景（如CPU空闲时处理中断），`switchkvm` 可以确保这一点。然而，在xv6的典型陷阱处理流程中，由于用户页目录已包含内核映射，通常不立即切换到`kpgdir`，而是直接在用户进程的内核映射下操作。`switchkvm` 更多地用于调度器返回到其主循环时。

通过 `switchuvm` 和 `switchkvm`（或直接使用 `lcr3`），xv6 确保了在执行用户代码时使用用户页表，在执行核心内核代码（如调度器）时使用内核页表（或至少保证内核映射可用）。

### 2. 关键页表操作函数 (Key Page Table Manipulation Functions)

xv6 内核中有几个核心的C函数，用于创建、修改和管理页表。这些函数都位于 `kernel/vm.c`。

#### a. `walkpgdir(pde_t *pgdir, const void *va, int alloc)` - 查找PTE

我们已经在 4.2 节中介绍过 `walkpgdir`，这里再次强调其核心功能和 `alloc` 标志的重要性。
*   **目的**：为给定的页目录 `pgdir` 和虚拟地址 `va`，找到对应的页表项 (PTE) 的地址。
*   **过程**：
    1.  从 `va` 中提取页目录索引 (PDX)。
    2.  定位到 `pgdir` 中的相应 PDE (`pde = &pgdir[PDX(va)];`)。
    3.  检查 PDE 是否存在 (`*pde & PTE_P`)。
        *   **如果 PDE 存在**：从中提取页表的物理地址，转换为内核虚拟地址，得到页表 `pgtab`。
        *   **如果 PDE 不存在**：
            *   若 `alloc` 参数为 **0**，表示不允许分配，则 `walkpgdir` 直接返回 `0` (NULL)，表示找不到 PTE（因为其所属的页表不存在）。
            *   若 `alloc` 参数为 **1**，则 `walkpgdir` 会：
                1.  调用 `kalloc()` 分配一个新的物理页作为页表。
                2.  如果 `kalloc()` 失败，返回 `0`。
                3.  用 `memset()` 将新分配的页表清零。
                4.  用新页表的物理地址和权限 (`PTE_P | PTE_W | PTE_U`) 更新当前的 PDE，使其指向这个新页表。
                5.  `pgtab` 指向这个新分配并初始化的页表。
    4.  从 `va` 中提取页表索引 (PTX)，返回 `&pgtab[PTX(va)]`，即该虚拟地址对应的 PTE 在页表中的地址。
*   **返回值**：成功则返回 PTE 的内核虚拟地址，失败则返回 `0`。
*   **重要性**：`walkpgdir` 是所有页表操作的基础。无论是想读取一个映射、建立一个新映射还是取消一个映射，首先都需要通过它找到目标 PTE。`alloc` 标志的灵活性使得它既能用于查询也能用于创建。

#### b. `mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)` - 创建映射

*   **目的**：在指定的页目录 `pgdir` 中，将一段连续的虚拟地址范围 `[va, va+size)` 映射到一段连续的物理地址范围 `[pa, pa+size)`，并赋予指定的权限 `perm`。
*   **过程**：
    1.  将起始虚拟地址 `va_start` 和结束虚拟地址 `va_end` 都对齐到页边界。
    2.  循环遍历从 `va_start` 到 `va_end` 的每一个虚拟页。
    3.  在每次循环中：
        *   调用 `walkpgdir(pgdir, current_va, 1)` 获取当前虚拟页 `current_va` 对应的 PTE 地址。这里 `alloc` 为 1，表示如果中间的页表不存在，则应创建它。
        *   如果 `walkpgdir` 失败（例如 `kalloc` 失败），则 `mappages` 释放已做的部分映射（通过 `deallocuvm` 的某种形式或手动清理）并返回错误。
        *   检查获取到的 PTE 是否已经存在 (`*pte & PTE_P`)。如果已存在，表示该虚拟地址已被映射，这是一个错误（通常在初始化映射时不应发生），`mappages` 会 panic。
        *   将当前的物理页地址 `current_pa` 和指定的权限 `perm`（并确保 `PTE_P` 位被设置）写入到 PTE 中：`*pte = current_pa | perm | PTE_P;`。
        *   `current_va` 增加 `PGSIZE`，`current_pa` 也增加 `PGSIZE`，进入下一轮循环。
*   **使用者**：`setupkvm`（建立内核地址空间）、`inituvm`（为第一个用户进程映射初始代码）、`allocuvm`（在`sbrk`或`exec`中为用户段分配和映射新页时，虽然`allocuvm`通常自己调用`kalloc`并直接设置PTE，但其逻辑与`mappages`相似，即找到PTE并填充）。

#### c. `allocuvm(pde_t *pgdir, uint oldsz, uint newsz)` - 扩展用户虚拟内存

*   **目的**：为页目录 `pgdir` 所代表的用户地址空间，将其大小从 `oldsz` 扩展到 `newsz` (`newsz > oldsz`)。它负责分配物理内存并建立映射。
*   **过程**：
    1.  检查 `newsz` 是否超过 `KERNBASE`，若是则无效。
    2.  将 `oldsz` 向上对齐到页边界，作为实际开始分配的起始虚拟地址。
    3.  循环处理从对齐后的 `oldsz` 到 `newsz` 的每一页：
        *   调用 `kalloc()` 分配一个物理页。如果失败，则调用 `deallocuvm(pgdir, newsz, oldsz_original)` 回滚已分配的页，并返回 `0`。
        *   用 `memset()` 将新分配的物理页清零（确保用户进程得到干净的内存）。
        *   调用 `mappages(pgdir, current_va, PGSIZE, V2P(kalloc_result), PTE_W | PTE_U)` 将这个物理页映射到当前虚拟页 `current_va`，权限为可写、用户可访问。严格来说，`allocuvm` 内部通常是直接调用 `walkpgdir` 获取PTE然后设置，而不是再调用一层`mappages`。但逻辑上等同于为每一页做一次单页的`mappages`。
            ```c
            // 实际 allocuvm 的核心循环更像这样：
            // for(a = PGROUNDUP(oldsz); a < newsz; a += PGSIZE){
            //   mem = kalloc();
            //   if(mem == 0){ deallocuvm(pgdir, newsz, oldsz); return 0; }
            //   memset(mem, 0, PGSIZE);
            //   if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
            //      kfree(mem); deallocuvm(pgdir, newsz, oldsz); return 0;
            //   }
            // }
            // 更精简的写法是直接用 walkpgdir 创建映射：
            // pte = walkpgdir(pgdir, (void*)a, 1);
            // *pte = V2P(mem) | PTE_P | PTE_W | PTE_U;
            ```
*   **返回值**：成功则返回 `newsz`，失败则返回 `0`。
*   **使用者**：`growproc` (响应 `sbrk` 系统调用)、`exec` (为新程序的段和初始栈分配内存)。

#### d. `deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)` - 收缩用户虚拟内存

*   **目的**：为页目录 `pgdir` 所代表的用户地址空间，将其大小从 `oldsz` 收缩到 `newsz` (`newsz < oldsz`)。它负责释放物理内存并取消映射。
*   **过程**：
    1.  检查 `newsz` 是否超过 `oldsz`，若是则无效。
    2.  将 `newsz` 向上对齐到页边界。
    3.  循环处理从对齐后的 `newsz` 到 `oldsz` 的每一页（即要被释放的区域）：
        *   调用 `walkpgdir(pgdir, current_va, 0)` 获取当前虚拟页 `current_va` 对应的 PTE 地址。这里 `alloc` 为 0，因为我们只是在查找已存在的映射。
        *   如果找到了 PTE 并且它是有效的 (`*pte & PTE_P`)：
            *   从 PTE 中提取物理地址 `pa = PTE_ADDR(*pte)`。
            *   调用 `kfree(P2V(pa))` 释放该物理页。（`P2V` 将物理地址转为内核虚拟地址给 `kfree`）。
            *   将 PTE 清零 (`*pte = 0;`)，使其无效。
*   **返回值**：成功则返回 `newsz`，失败则返回 `oldsz`（或0，取决于具体实现）。
*   **使用者**：`growproc` (响应 `sbrk` 传入负值)、`freevm` (在进程退出时)。

#### e. `copyuvm(pde_t *from_pgdir, pde_t *to_pgdir, uint sz)` - 复制用户虚拟内存 (用于 `fork`)

*   **目的**：在 `fork()` 期间，将父进程 (`from_pgdir`) 的用户空间内存（大小为 `sz`）完整地复制到子进程的新页目录 (`to_pgdir`)。
*   **过程**：
    1.  循环遍历父进程用户空间（从虚拟地址 0 到 `sz`）的每一页。
    2.  对于每一虚拟页 `current_va`：
        *   调用 `walkpgdir(from_pgdir, current_va, 0)` 获取父进程中该虚拟页对应的 PTE。
        *   如果 PTE 不存在或无效，则跳过（理论上不应发生，除非 `sz` 超出了父进程实际映射的范围）。
        *   如果 PTE 有效：
            1.  从父进程 PTE 中提取物理地址 `pa = PTE_ADDR(*pte)` 和权限 `flags = PTE_FLAGS(*pte)`。
            2.  调用 `kalloc()` 为子进程分配一个新的物理页 `child_mem`。如果失败，则 `copyuvm` 失败，需要清理已为子进程分配的所有资源（包括页表和物理页），然后返回错误。
            3.  使用 `memmove(child_mem, P2V(pa), PGSIZE)` 将父进程物理页的内容完整复制到子进程的新物理页中。
            4.  调用 `mappages(to_pgdir, current_va, PGSIZE, V2P(child_mem), flags)` 在子进程的页目录 `to_pgdir` 中，将相同的虚拟地址 `current_va` 映射到新分配并复制好的物理页 `child_mem`，并赋予与父进程相同的权限 `flags`。如果 `mappages` 失败，同样需要清理并返回错误。
*   **返回值**：成功则返回 `0`，失败则返回 `-1`。
*   **使用者**：`fork()`。

#### f. `freevm(pde_t *pgdir)` - 释放整个用户虚拟内存

*   **目的**：当一个进程退出时（在 `exit()` 中），需要释放其整个用户地址空间所占用的所有物理内存，并释放其页表本身。
*   **过程**：
    1.  如果 `pgdir` 为空，则直接返回。
    2.  **释放用户页**：循环遍历用户地址空间的所有可能的虚拟页（从 0 到 `KERNBASE`）。
        *   调用 `walkpgdir(pgdir, current_va, 0)` 获取 PTE。
        *   如果 PTE 存在且有效 (`*pte & PTE_P`)：
            *   从 PTE 中提取物理地址 `pa = PTE_ADDR(*pte)`。
            *   调用 `kfree(P2V(pa))` 释放该用户数据物理页。
            *   将PTE清零 (`*pte = 0;`)，但此时还不能释放页表页本身，因为可能还有其他PTE在该页表页中。
    3.  **释放页表页**：在 xv6 的实现中，`freevm` 会更进一步，它会遍历页目录中的所有 PDE。如果一个 PDE 指向一个用户空间的页表（即 PDE 本身有 `PTE_U` 标志，并且该页表不是内核共享的），那么在清除了该页表内所有用户页的映射后，这个页表页本身也应该被 `kfree`。
        ```c
        // 简化逻辑：
        // for (i = 0; i < NPDENTRIES; i++) { // NPDENTRIES 是页目录中的条目数, 1024
        //   pde = &pgdir[i];
        //   if (*pde & PTE_P) {
        //     pte_t *pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
        //     // 实际的 xv6 freevm 会先遍历 pgtab 中的所有 PTE，释放它们指向的物理页
        //     // ...如上面第2点所述...
        //     // 然后，如果这个 pgtab 是用户页表（不是内核共享的），则释放它
        //     // （xv6的实现是，如果PDE指向的是用户空间的地址，则其对应的pgtab可以释放）
        //     // 并且在deallocuvm中，如果一个页表页变空，它不会被立即回收。
        //     // freevm则更彻底，它会释放所有用户映射的页和这些页所属的页表页。
        //     // 对于setupkvm创建的pgdir，用户部分的PDE开始是空的。
        //     // 当allocuvm分配用户页时，可能会分配新的页表页（通过walkpgdir的alloc=1）。
        //     // freevm会kfree这些页表页。
        //     // 它通过检查PDE的PTE_U位，如果设置了，说明是用户页表，可以释放。
        //     if(*pde & PTE_U) // 这是一个简化判断，实际xv6可能更复杂
        //        kfree((void*)pgtab); // 释放页表页本身
        //     *pde = 0; // 清除PDE
        //   }
        // }
        // xv6 的 freevm 实现更直接：它首先调用 deallocuvm(pgdir, KERNBASE, 0) 来释放所有用户页和页表页。
        // deallocuvm 在清空一个页表的所有PTE后，会kfree掉该页表页。
        // 然后，freevm 最后 kfree(pgdir) 释放页目录本身。
        ```
        xv6 `freevm` 的实际做法是先调用 `deallocuvm(pgdir, KERNBASE, 0)`，这个调用会从 `KERNBASE` 向下直到地址0，释放所有用户映射的物理页，并且如果一个页表页（二级表）因此变空，`deallocuvm` 的逻辑（或其调用的 `walkpgdir` 的清理部分）会负责释放该页表页本身。
    4.  **释放页目录**：最后，调用 `kfree((char*)pgdir)` 释放进程的页目录页本身。
*   **使用者**：`exit()` (在清理进程资源时)、`exec()` (在加载新程序前，释放旧地址空间时)。

### 3. TLB (Translation Lookaside Buffer) - 地址翻译的缓存

MMU 为了加速虚拟地址到物理地址的转换，通常包含一个小型的、高速的硬件缓存，称为 **TLB (Translation Lookaside Buffer)**。

*   **作用**：TLB 缓存了最近使用过的虚拟页到物理页的映射关系（即一部分活跃的 PTE 内容）。当 MMU 需要翻译一个虚拟地址时，它首先并行地在 TLB 中查找该虚拟页的映射。
    *   **TLB命中 (TLB Hit)**：如果在 TLB 中找到了有效的映射，MMU 可以直接使用这个缓存的物理地址和权限，无需访问内存中的页表。这非常快。
    *   **TLB未命中 (TLB Miss)**：如果在 TLB 中没有找到映射，MMU 才需要执行完整的页表遍历（访问页目录和页表，这可能涉及多次内存读取），找到PTE后，将结果存入TLB以备后续使用，然后再进行地址翻译。
*   **上下文切换与 TLB**：当操作系统切换进程时（例如，通过 `lcr3(V2P(new_pgdir))` 加载了新的页目录基址到 `%cr3`），旧进程的 TLB 条目对于新进程来说大部分是无效的（因为新进程有不同的地址空间映射，除非是共享的内核空间条目）。
    *   **TLB 刷新 (TLB Flush)**：x86 架构规定，当 `%cr3` 寄存器被重新加载时，处理器会自动刷新（或部分刷新，取决于具体实现和 TLB 设计）TLB 中非全局的条目。这确保了后续的地址翻译会使用新页表的正确映射，而不是旧的、过时的 TLB 条目。
    *   全局页（PTE 中设置了 Global 标志的页，通常用于内核代码）的 TLB 条目在 `%cr3` 加载时可能不会被刷新，因为它们在所有地址空间中都是有效的。xv6 对内核页的映射通常不设置 Global 位，所以 `lcr3` 基本上会刷新所有用户和内核的 TLB 条目，确保一致性。

TLB 的存在对虚拟内存系统的性能至关重要，它使得分页机制在实践中能够高效运行，尽管页表查找本身可能需要多次内存访问。操作系统在管理页表和进行上下文切换时，需要考虑到 TLB 的行为以保证正确性。

这些页表管理函数和页表切换机制共同构成了 xv6 虚拟内存系统的核心操作层，使得内核能够灵活、安全地为每个进程提供独立的地址空间视图，并有效地利用物理内存。
