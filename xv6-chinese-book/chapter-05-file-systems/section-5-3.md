## 5.3 日志系统：崩溃恢复 (Logging for Crash Recovery)

文件系统存储着至关重要的数据。如果在文件系统操作（如创建文件、写入数据）的途中系统发生崩溃（例如，突然断电），磁盘上的数据结构可能会处于不一致的状态，导致数据损坏或丢失。xv6 实现了一个简单的日志系统（主要在 `kernel/log.c` 中）来解决这个问题，确保文件系统在崩溃后能够恢复到一致的状态。

### 1. 文件系统一致性问题 (The Problem of File System Consistency)

一个文件系统操作通常需要修改磁盘上的多个不同位置的块。例如，创建一个新文件可能涉及：

1.  **分配一个 inode**: 在 inode 表中找到一个空闲的 inode，并将其标记为已使用。
2.  **初始化 inode**: 设置 inode 的类型（如 `T_FILE`）、链接数、初始大小等。
3.  **分配数据块 (可选)**: 如果写入了数据，可能需要分配数据块。
4.  **更新数据块位图**: 将分配的数据块在位图中标记为已使用。
5.  **添加到目录**: 在父目录的数据块中添加一个新的目录项，包含新文件的名称和其 inode 号。
6.  **更新父目录 inode**: 更新父目录 inode 的大小和修改时间。

如果在这些步骤中的任何一点发生系统崩溃，文件系统就可能处于不一致的状态。例如：

*   **inode 已分配但未链接到目录**: 一个 inode 可能被标记为已使用，但没有目录项指向它，导致一个“丢失”的文件，并且该 inode 无法被正常回收。
*   **数据块已分配但 inode 未指向它**: 一个数据块可能在位图中被标记为已使用，但没有任何 inode 指向它，导致空间浪费。
*   **inode 指向一个空闲块**: 一个 inode 的地址块可能指向一个在位图中仍标记为空闲的块。
*   **目录项指向一个无效的 inode 号**: 或者目录项中的文件名与 inode 中的类型不匹配。

这些不一致性可能导致数据丢失、文件系统无法正常工作，甚至内核崩溃。在没有日志的情况下，检测和修复这些不一致性（例如，通过像 `fsck` 这样的工具）可能非常复杂和耗时，并且不一定能完美恢复数据。

### 2. 日志作为解决方案 (Journaling/Logging as a Solution)

日志（Journaling，也常称为预写日志 Write-Ahead Logging, WAL）是一种广泛用于数据库和文件系统的技术，用于保证操作的**原子性 (atomicity)** 和**持久性 (durability)**，从而实现崩溃一致性。

其基本思想是：

1.  **预写 (Write-Ahead)**：在对文件系统的实际数据结构（如 inode 表、位图、数据块）进行任何修改之前，先将描述这些**意图进行的修改**的详细信息写入磁盘上的一个称为“日志 (log)”的专用区域。
2.  **提交 (Commit)**：当所有描述一个完整操作（或一组相关操作，称为一个“事务”）的日志条目都安全地写入磁盘后，一个特殊的“提交记录 (commit record)”也会被写入日志。这标志着该事务在逻辑上已完成。
3.  **应用到文件系统 (Checkpointing/Applying)**：在提交之后，系统才开始将这些修改实际应用到磁盘上文件系统的相应位置。
4.  **清除日志**: 当修改成功应用到文件系统后，日志中的相应条目可以被标记为已完成或被清除，以便为新的日志腾出空间。

**崩溃恢复 (Crash Recovery)**：
*   如果在将修改应用到文件系统的过程中发生崩溃（即在步骤3中崩溃）：系统重启后，会检查日志。它会发现一个已提交但可能未完全应用的事务。通过重新读取日志中的修改信息（这个过程称为“重放日志”，replaying the log），系统可以将这些修改重新应用到文件系统，确保操作的完整性。
*   如果在写入日志或提交记录的过程中发生崩溃（即在步骤1或2中崩溃）：系统重启后，会发现一个未提交的（或不完整的）事务。这些未完成的修改会被忽略或回滚（简单地不重放它们），因为它们从未被逻辑上确认为已完成。文件系统会保持在事务开始之前的状态。

通过这种方式，无论崩溃发生在哪个阶段，文件系统都能恢复到一个已知的、一致的状态。

### 3. xv6 中的日志系统 (`log.c`)

xv6 实现了一个相对简单的、基于物理块的日志系统。这意味着它记录的是对整个磁盘块的修改，而不是更细粒度的逻辑操作。

#### a. 日志头部 (`struct logheader` 与 `log.lh`)

日志在磁盘上有一个头部，用于跟踪当前未完成（outstanding）的事务中涉及哪些块。

```c
// kernel/log.c
// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  int n;             // 当前事务中已记录的块的数量
  int block[LOGSIZE]; // 记录这些块在磁盘上的实际块号 (LOGSIZE 在 param.h 中定义)
};

struct log {
  struct spinlock lock;
  int start;            // 日志区域在磁盘上的起始块号 (从 superblock 读取)
  int size;             // 日志区域的总块数 (从 superblock 读取)
  int outstanding;      // 当前有多少个文件系统调用正在进行操作 (未提交)
  int committing;       // 标志位，表示是否正在进行提交操作
  int dev;              // 设备号
  struct logheader lh;  // 内存中的日志头部副本
};
struct log log; // 全局日志结构体
```

*   **`struct logheader`**:
    *   **`n`**: 一个整数，表示当前内存中的日志头部 `log.lh` 中记录了多少个块号。
    *   **`block[LOGSIZE]`**: 一个数组，存储了当前事务中所有被修改过的、且其修改内容已暂存（通常在缓冲区缓存中）并准备写入日志数据区的那些块的**实际磁盘块号**。`LOGSIZE` 是日志系统能一次处理的最大块数。

*   **`struct log log`**: 这是内核中全局的日志状态结构。
    *   **`lock`**: 一个自旋锁，用于保护对 `log` 结构体内部字段（如 `outstanding`, `committing`, `lh`）的并发访问。
    *   **`start`**: 日志区在磁盘上的起始块号。
    *   **`size`**: 日志区的总大小（以块为单位）。日志数据块和日志头块都在这个区域内。通常，日志区的第一个块是日志头块，其余是日志数据块。`log.size` 必须大于 `LOGSIZE + 1`。
    *   **`outstanding`**: 记录当前有多少个文件系统操作（如 `create`, `write`）已经调用了 `begin_op` 但尚未调用 `end_op`。
    *   **`committing`**: 一个标志位。如果为 1，表示当前正在执行 `commit()` 操作，此时新的文件系统操作不能开始（`begin_op` 会等待）。
    *   **`dev`**: 文件系统所在的设备号。
    *   **`lh`**: 一个 `struct logheader` 的内存副本。当文件系统操作通过 `log_write()` 记录块时，块号会被添加到 `log.lh.block[]` 中，`log.lh.n` 会递增。

#### b. 磁盘上的日志区域 (On-Disk Log)

*   由超级块 (`sb.logstart` 和 `sb.nlog`) 定义。
*   第一个块是**日志头块 (On-disk Log Header Block)**，它存储了一个 `struct logheader` 的内容。
*   其余的 `sb.nlog - 1` 个块是**日志数据块 (Log Data Blocks)**。当 `log_write()` 记录一个块 `b` 时，`b` 的内容最终会在 `commit()` 期间被写入到这些日志数据块中的某一个。`log.lh.block[i]` 存储的是块 `b` 在文件系统中的**最终位置**的块号，而块 `b` 的数据会被临时写入到日志区的第 `i+1` 个数据块中（即 `log.start + 1 + i`）。

#### c. `begin_op()` - 开始一个事务

任何可能修改文件系统的操作序列（例如，一个系统调用的实现）都必须首先调用 `begin_op()`。

```c
// kernel/log.c
void
begin_op(void)
{
  acquire(&log.lock);
  while(1){
    if(log.committing){ // 如果正在提交，则等待
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      // 如果当前日志头中的块数 + (未完成操作数+当前操作)*每个操作最大块数 > 日志空间
      // 这表示可能没有足够的空间容纳这个新操作可能产生的所有日志块
      // (MAXOPBLOCKS 是一个估计值，表示单个系统调用可能修改的最大块数)
      sleep(&log, &log.lock);
    } else { // 可以开始新操作
      log.outstanding++;
      release(&log.lock);
      break;
    }
  }
}
```
1.  **获取日志锁 `log.lock`**。
2.  **循环等待条件满足**：
    *   **检查 `log.committing`**: 如果当前有其他进程正在执行 `commit()` 操作，则调用 `sleep(&log, &log.lock)` 使当前进程睡眠，等待提交完成。`sleep` 会原子地释放 `log.lock` 并等待。当被唤醒时，它会重新获取锁。
    *   **检查日志空间 (Log Space Check)**：这是一个重要的检查，确保日志有足够的空间容纳当前操作可能产生的所有写操作。
        *   `log.lh.n`: 当前内存日志头中已记录的块数（来自之前已调用 `log_write` 但未提交的操作）。
        *   `log.outstanding`: 已经开始但尚未结束的操作数量。
        *   `(log.outstanding + 1) * MAXOPBLOCKS`: 估计所有未完成的操作（包括当前这个新操作）总共可能写入的最大块数。`MAXOPBLOCKS` (在 `fs.h` 中定义，例如10) 是一个保守的估计，表示单个顶层文件系统操作（如创建一个文件）最多可能修改的块数。
        *   如果 `log.lh.n` 加上这个估计值超过了 `LOGSIZE`（日志数据区的容量），则意味着日志空间可能不足。当前进程也必须 `sleep`，等待其他操作提交并释放日志空间。
    *   **如果条件满足**: `log.outstanding++`（增加未完成操作的计数），释放 `log.lock`，然后 `begin_op` 返回，允许文件系统操作继续。

#### d. `log_write(struct buf *b)` - 记录一个被修改的块

当文件系统层（如 `writei` 写入inode，`dirlink` 写入目录，或对数据块的修改）需要修改一个缓冲区 `b` 的内容，并且这个修改是事务的一部分时，它**不能直接调用 `bwrite(b)`** 将其写回磁盘上的最终位置。相反，它必须调用 `log_write(b)`。

```c
// kernel/log.c
// Called by file system B_DIRTY write to write log record.
// Caller has locked b. Caller has used begin_op.
// log_write(b) says that b is a dirty buffer that needs to be written to disk.
// But instead of writing b directly to its final place on disk,
// log_write() copies b's blockno into log.lh.block[] and then
// holds b in the cache until commit(). log_write() doesn't actually
// write b to the disk log; commit() does that.
void
log_write(struct buf *b)
{
  int i;

  // 调用者必须持有 b->lock (由 bread/bget 获取)
  // 调用者必须已经调用了 begin_op

  acquire(&log.lock);
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction"); // 如果日志头已满或超出日志数据区容量
  if (log.outstanding < 1)
    panic("log_write outside of transaction"); // 必须在 begin_op/end_op 之间

  // 检查块 b 是否已在当前事务的日志头中
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // Block number already logged?
      break;
  }
  log.lh.block[i] = b->blockno; // 记录块号 (如果不在，i==log.lh.n; 如果在，则覆盖旧条目，但块号不变)
  if (i == log.lh.n) { // 如果是新加入的块
    bpin(b->dev, b->blockno); // Pin a buffer in cache so it is not evicted.
                              // (xv6-riscv/x86-book 使用 bpin(b) 来增加 buf 的引用计数)
    log.lh.n++; // 增加日志头中记录的块数
  }
  release(&log.lock);
}
```
1.  **前提条件**：调用 `log_write(b)` 的代码必须已经调用了 `begin_op()`，并且通常持有缓冲区 `b` 的锁 (`b->lock`)。
2.  **获取日志锁 `log.lock`**。
3.  **检查**：
    *   `log.lh.n` 是否已达到 `LOGSIZE` 或 `log.size - 1`（日志数据区的容量）。如果是，则 panic，表示事务太大了，超出了日志容量。
    *   `log.outstanding < 1`：确保 `log_write` 是在 `begin_op`/`end_op` 之间被调用的。
4.  **记录块号**：
    *   遍历内存中的日志头 `log.lh.block[]`，检查块 `b` 的块号 `b->blockno` 是否已经被记录。
    *   如果未被记录，则将 `b->blockno` 添加到 `log.lh.block[]` 数组的末尾（`log.lh.block[log.lh.n]`），并递增 `log.lh.n`。
    *   同时，调用 `bpin(b->dev, b->blockno)` (或在一些版本中是 `bpin(b)`)。`bpin` 的作用是增加缓冲区 `b` 的引用计数 (`b->refcnt`)。这可以防止这个被日志记录的缓冲区在提交之前被 `bget` 回收用于缓存其他磁盘块。在日志提交并安装（`install_trans`）之后，这些块会被 `bunpin`。
    *   如果块号已存在于日志头中（意味着这个块在同一个事务中被多次修改），则不需要再次添加，也不需要再次 `bpin`。
5.  **释放日志锁 `log.lock`**。

**重要**：`log_write(b)` **并不实际将 `b->data` 的内容写入磁盘上的日志区域**。它只是在内存中的日志头里记录下“块 `b` 是脏的，并且是当前事务的一部分”。实际的写盘操作由 `commit()` 负责。缓冲区 `b` 的内容仍然保留在内存的缓冲区缓存中。

#### e. `end_op()` - 结束一个事务

当一个文件系统操作（如一个系统调用的实现）完成了所有对磁盘块的（通过 `log_write` 记录的）修改后，它必须调用 `end_op()`。

```c
// kernel/log.c
void
end_op(void)
{
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding--; // 减少未完成操作的计数
  if(log.committing) // 如果正在提交，则不重复发起
    panic("log.committing");
  if(log.outstanding == 0){ // 如果这是最后一个未完成的操作
    do_commit = 1; // 标记需要进行提交
    log.committing = 1; // 设置正在提交标志
  } else {
    // begin_op() may be waiting for log space,
    // and decrementing log.outstanding has made space.
    wakeup(&log); // 唤醒可能在 begin_op 中等待日志空间的进程
  }
  release(&log.lock);

  if(do_commit){ // 如果需要提交
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit(); // 执行提交操作
    
    acquire(&log.lock);
    log.committing = 0; // 清除正在提交标志
    wakeup(&log); // 唤醒可能在 begin_op 中等待提交完成的进程
    release(&log.lock);
  }
}
```
1.  **获取日志锁 `log.lock`**。
2.  **`log.outstanding--`**: 减少未完成操作的计数。
3.  **检查是否需要提交**:
    *   如果 `log.outstanding == 0`（表示这是系统中最后一个完成的、之前调用过 `begin_op` 的操作），那么就需要执行提交操作了。
        *   设置 `do_commit = 1`。
        *   设置 `log.committing = 1`，以阻止其他进程在提交期间调用 `begin_op` 开始新事务。
    *   如果 `log.outstanding > 0`（表示还有其他文件系统操作正在进行中），则当前 `end_op` 不需要发起提交。但是，由于 `log.outstanding` 减少了，可能为那些在 `begin_op` 中因日志空间不足而睡眠的进程腾出了空间，所以调用 `wakeup(&log)` 来唤醒它们。
4.  **释放日志锁 `log.lock`**。
5.  **如果 `do_commit` 为真**:
    *   调用 `commit()` 函数（见下一节）来执行实际的日志提交过程。**注意**：`commit()` 是在不持有 `log.lock` 的情况下调用的，因为 `commit()` 内部会进行磁盘I/O，可能会导致睡眠，而持有自旋锁时是不能睡眠的。
    *   `commit()` 返回后，重新获取 `log.lock`。
    *   清除 `log.committing = 0` 标志。
    *   调用 `wakeup(&log)` 唤醒那些可能在 `begin_op` 中因为 `log.committing` 为真而睡眠的进程。
    *   释放 `log.lock`。

### 4. 提交过程 (`commit()`)

`commit()` 函数（在 `kernel/log.c` 中）负责将当前事务中所有记录在内存日志头 `log.lh` 中的块的修改持久化到磁盘。它分几个关键步骤执行：

```c
// kernel/log.c
static void
commit()
{
  if (log.lh.n > 0) { // 如果日志头中有记录的块 (即事务非空)
    write_log();      // 步骤1: 将所有被修改的块的内容写入磁盘上的日志数据区
    write_head();     // 步骤2: 将内存中的日志头 (log.lh) 写入磁盘上的日志头块 (提交点)
    install_trans(0); // 步骤3: 将日志数据区的块内容复制到它们在文件系统中的最终位置
    log.lh.n = 0;     // 步骤4: 清空内存中的日志头 (n=0)
    write_head();     // 步骤5: 再次将清空的日志头写入磁盘 (标记日志为空，事务完成)
  }
}
```

1.  **`write_log()` - 将数据写入日志区**:
    *   这个静态函数遍历内存中的日志头 `log.lh.block[]` 中的每一个块号。
    *   对于第 `i` 个块号 `log.lh.block[i]`：
        *   它首先通过 `bread(log.dev, log.lh.block[i])` 获取该块在缓冲区缓存中的最新内容（这个 `buf` 应该已经被 `log_write` pin住了）。
        *   然后，它将这个 `buf->data` 的内容写入到磁盘上日志数据区的第 `i` 个槽位，即磁盘块号为 `log.start + 1 + i` 的位置。这通常通过调用 `bwrite()` 完成（目标是日志数据块）。
        *   完成后，释放 `buf` (`brelse(buf)`)。
    *   **目的**：这一步将所有在当前事务中被修改的数据的副本保存到了磁盘上的日志数据区。

2.  **`write_head()` - 将日志头写入磁盘 (提交点)**：
    *   这个静态函数分配一个缓冲区来格式化磁盘上的日志头块。
    *   它将内存中的 `log.lh`（现在包含了所有已写入日志数据区的块的块号和数量 `n`）的内容复制到这个缓冲区的 `data` 字段。
    *   然后，它调用 `bwrite()` 将这个缓冲区的内容写入到磁盘上日志区域的第一个块（即 `log.start`，这是磁盘日志头块）。
    *   **这是关键的“提交点”**。一旦这个磁盘日志头块被成功写入，就意味着事务在逻辑上已经提交。如果系统在这一点之后、但在 `install_trans` 完成之前崩溃，恢复程序 (`recover_from_log`) 能够读取这个日志头，知道哪些块需要被恢复。

3.  **`install_trans(int recovering)` - 安装事务 (应用到文件系统)**：
    *   这个静态函数负责将日志数据区中的数据块内容复制到它们在文件系统中的最终位置。
    *   它遍历（内存中或从磁盘日志头中读取的）`log.lh.block[]` 中的每一个块号。
    *   对于第 `i` 个块号 `log.lh.block[i]`（这是最终目标块号）：
        *   它首先通过 `bread(log.dev, log.start + 1 + i)` 从磁盘的日志数据区读取第 `i` 个已记录的数据块内容到缓冲区 `logbuf`。
        *   然后，它获取另一个缓冲区 `destbuf`，这次是对应于最终目标位置 `log.lh.block[i]` (`bread(log.dev, log.lh.block[i])`)。
        *   将 `logbuf->data` 的内容用 `memmove` 复制到 `destbuf->data`。
        *   调用 `bwrite(destbuf)` 将修改后的内容写回到文件系统中的最终位置。
        *   释放 `logbuf` 和 `destbuf`。
    *   在 `commit()` 期间调用时 `recovering` 参数为0。如果是恢复期间调用，`recovering` 为1，行为基本相同。
    *   **解开钉住的缓冲区**：在 `install_trans` 完成后（或者在现代xv6中，在每个块被成功安装后），那些之前被 `log_write` 通过 `bpin` 钉住的缓冲区需要被 `bunpin`，即减少它们的引用计数，允许它们被回收。这通常在 `commit` 的最后或 `install_trans` 内部完成。

4.  **清除内存中的日志头 (`log.lh.n = 0`)**: 在事务成功安装到文件系统后，内存中的日志头 `log.lh.n` 被设置为0，表示当前没有活动的日志记录了。

5.  **再次 `write_head()` - 清除磁盘上的日志头**:
    *   再次调用 `write_head()`，但这次它写入的是已经被清零的 `log.lh`（因为 `log.lh.n` 现在是0）。
    *   这会更新磁盘上的日志头块，将其中的 `n` 也设置为0。
    *   **目的**：这标志着磁盘上的日志现在是空的，之前记录的事务已经成功应用到了文件系统。如果系统在这一点之后崩溃，恢复程序在启动时会看到 `n=0` 的日志头，就知道不需要进行任何恢复操作。

### 5. 恢复过程 (`recover_from_log()` 与 `initlog()`)

当 xv6 内核启动时，`fsinit()`（在 `kernel/fs.c` 中）会调用 `initlog(ROOTDEV, &sb)` 来初始化日志系统。

```c
// kernel/log.c
void
initlog(int dev, struct superblock *sb)
{
  // ... (初始化 log 结构体的字段，如 dev, start, size, lock) ...
  // log.start = sb->logstart;
  // log.size = sb->nlog;
  // log.dev = dev;
  // ...
  
  recover_from_log(); // 从日志中恢复（如果需要）
}

static void
recover_from_log(void)
{
  read_head(); // 从磁盘读取日志头到 log.lh
  if(log.lh.n > 0){ // 如果日志头中 n > 0，表示上次有关闭前未完成的事务
    cprintf("recovering fs ...\n");
    install_trans(1); // 安装（重放）这个事务，recovering=1
    log.lh.n = 0;     // 清空内存中的日志头
    write_head();     // 将清空的日志头写回磁盘
    cprintf("recovery complete\n");
  }
}

static void
read_head(void) // 从磁盘日志头块读取日志头到 log.lh
{
  struct buf *buf = bread(log.dev, log.start); // 读取磁盘日志头块
  struct logheader *lh = (struct logheader *) buf->data;
  log.lh.n = lh->n;
  for (int i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}
```

*   **`initlog()`**:
    *   它首先从超级块 `sb` 中获取日志区域的起始位置 `sb.logstart` 和大小 `sb.nlog`，并初始化全局 `log` 结构体的这些字段。
    *   然后，它调用 `recover_from_log()`。

*   **`recover_from_log()`**:
    1.  调用 `read_head()` 从磁盘上的日志头块（`log.start`）读取日志头信息到内存中的 `log.lh`。
    2.  检查 `log.lh.n`。
        *   如果 `log.lh.n == 0`，表示上次系统正常关闭，或者所有事务都已成功完成。不需要恢复。
        *   如果 `log.lh.n > 0`，表示在上次的 `commit()` 过程中，在 `write_head()`（步骤2，提交点）之后但在最终清除磁盘日志头（步骤5）之前发生了崩溃。这意味着日志数据区包含了已提交事务的数据，但这些数据可能没有完全写到文件系统的最终位置。
            *   打印恢复信息。
            *   调用 `install_trans(1)`。这个函数会像正常提交时一样，将日志数据区的内容复制到它们在文件系统中的最终位置。参数 `1` 可能用于指示这是恢复模式下的安装。
            *   将内存中的 `log.lh.n` 设置为 0。
            *   调用 `write_head()` 将这个清零的日志头写回磁盘。这确保了如果再次立即崩溃，不会重复进行恢复。
            *   打印恢复完成信息。

### 6. 缓冲区缓存与日志的交互 (`bpin`, `bunpin`)

*   当 `log_write` 记录一个块时，它会调用 `bpin` (pin buffer)。`bpin` 只是简单地增加缓冲区的引用计数 (`b->refcnt++`)。这确保了只要一个块被记录在日志中（即其块号在 `log.lh.block[]` 中），即使没有其他内核代码路径在使用它，它的 `refcnt` 也至少为1，因此 `bget` 不会回收它。
*   当日志事务被提交并且块内容被安全地安装到其最终位置后（在 `install_trans` 之后，通常在 `commit` 的末尾），需要对这些被 `bpin` 的缓冲区进行 `bunpin` 操作。`bunpin` 会减少缓冲区的引用计数。如果引用计数降为0，该缓冲区就可以被 `brelse` 正常地移到 MRU 链表头部，并最终可能被 `bget` 回收。
    xv6 的 `commit` 函数在 `install_trans` 执行完毕后，通过将 `log.lh.n` 置0来隐式地表示这些块不再受日志的“保护”。实际的 `bunpin` 操作（减少引用计数）通常是在 `brelse` 中完成的，当上层文件系统代码释放对这些块的引用时。日志系统本身通过 `bpin` 确保块在事务处理期间不被重用，而事务完成后，正常的 `brelse` 调用会最终处理引用计数。

xv6 的日志系统虽然简单，但它有效地解决了文件系统在崩溃面前的一致性问题，是xv6能够成为一个可用的、尽管是教学性质的操作系统的关键特性之一。它体现了预写日志的核心思想：先写日志，再写数据。
