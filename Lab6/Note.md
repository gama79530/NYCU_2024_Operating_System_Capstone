# Lab 6 Note

We will work through the Lab 1-5 development process again. The main difference
is that the MMU and virtual memory will be part of the design from the beginning.

Further details will be added as we progress.

## Implementation Roadmap: Labs 1-6

Start from a minimal system and gradually complete all basic and advanced exercises
in Labs 1-6, including both Video Player sections. Introduce MMU support early
instead of rebuilding the MMU-less process model and replacing it later.

Prefer one exercise per step. Combine closely related work where it produces a
coherent checkpoint, and split larger exercises into separately verifiable parts.
Earlier implementations remain references for code that can be reused after review.

- `LabN-BM` means Basic Exercise M in Lab N.
- `LabN-AM` means Advanced Exercise M in Lab N.
- `(partial)` identifies a subset; the later `(complete)` step finishes that exercise.
- `Lab5-Video` and `Lab6-Video` label the unnumbered Video Player sections.
- All steps are planned; the checkpoints below are not completed verification results.

### Steps 1-7: Minimal Kernel and Kernel Virtual Memory

| Step | Implementation | Exercise coverage | Completion checkpoint |
| --- | --- | --- | --- |
| 1 | Basic boot, BSS clearing, stack setup, EL2-to-EL1 transition, and a minimal exception framework | **Lab1-B1** + **Lab3-B1 (partial)** | Reach EL1 with an initialized stack and exception vectors |
| 2 | Polling Mini UART and basic diagnostic output | **Lab1-B2** | Print and receive characters before enabling the MMU |
| 3 | TCR, MAIR, bootstrap page tables, and identity mapping | **Lab6-B1 (partial)** | Enable the MMU while UART output continues to work |
| 4 | High kernel addresses and finer mappings for Normal RAM and Device MMIO | **Lab6-B1 (complete)** | Execute at high kernel VAs with working stack, vectors, and UART |
| 5 | Minimal shell and full-width address output | **Lab1-B3** | Run help and hello commands |
| 6 | Mailbox communication with explicitly converted, aligned buffers | **Lab1-B4** | Read board revision and ARM memory information |
| 7 | Watchdog-based reboot command | **Lab1-A1** | Reboot real hardware; record QEMU limitations separately |

Pull the EL2-to-EL1 setup and minimal exception diagnostics forward from Lab3-B1
because they are needed for EL1 MMU bring-up. Complete user-mode exception handling
at Step 18. Bootstrap page tables use reserved, page-aligned storage so these early
steps do not depend on an allocator that has not been implemented yet.

### Steps 8-12: Loading and Resource Discovery

| Step | Implementation | Exercise coverage | Completion checkpoint |
| --- | --- | --- | --- |
| 8 | UART kernel loading and an explicit bootloader-to-kernel handoff | **Lab2-B1** | Transfer and start the MMU-aware kernel at its physical load address |
| 9 | Bootloader self-relocation | **Lab2-A1** | Relocate the bootloader before loading an overlapping kernel image |
| 10 | DTB traversal and property lookup | **Lab2-A2** | Preserve the DTB handoff address and discover RAM/initramfs physical ranges |
| 11 | CPIO archive parsing through mapped kernel pointers | **Lab2-B2** | Find files and print their contents |
| 12 | Simple startup allocation with explicit address/alignment conventions | **Lab2-B3** | Allocate usable kernel memory without depending on buddy |

The bootloader can remain MMU-off. Its physical relocation is separate from the
kernel's virtual mappings. Read firmware structures through mapped pointers while
keeping the physical address values decoded from their properties distinct.

### Steps 13-17: Memory Allocation

| Step | Implementation | Exercise coverage | Completion checkpoint |
| --- | --- | --- | --- |
| 13 | Buddy allocation over a known unused RAM region | **Lab4-B1** | Allocate contiguous frames, split blocks, free, and coalesce |
| 14 | Efficient frame lookup and allocation/free operations | **Lab4-A1** | Meet the required complexity while keeping physical indices distinct from kernel pointers |
| 15 | Physical-memory reservation API | **Lab4-A2** | Keep reserved ranges out of allocatable memory |
| 16 | Startup bookkeeping, dynamic frame metadata, and usable-RAM handoff to buddy | **Lab4-A3** | Preserve the kernel, bootstrap tables, stacks, DTB, initramfs, startup allocations, and spin tables |
| 17 | Dynamic allocation for small objects and larger requests | **Lab4-B2** | Allocate/free through kernel VAs and release backing frames correctly |

Complete allocation before introducing allocator-backed timer queues, deferred
tasks, and process objects. Frame ownership should allow later sharing, but COW
reference-counting behavior is implemented at Step 31.

### Steps 18-22: Exceptions and Interrupts

| Step | Implementation | Exercise coverage | Completion checkpoint |
| --- | --- | --- | --- |
| 18 | User code/stack mappings, EL0 entry, SVC handling, and full context save/restore | **Lab3-B1 (complete)** + **Lab6-B2 (partial)** | Run a small EL0 program, enter the mapped EL1 handler, and resume correctly |
| 19 | Core timer interrupt and rearming | **Lab3-B2** | Interrupt EL0 execution, report elapsed time, and rearm for two seconds later |
| 20 | UART RX/TX interrupts and asynchronous buffers | **Lab3-B3** | Operate UART without continuous polling |
| 21 | One-shot timer multiplexing and nonblocking timeout commands | **Lab3-A1** | Execute multiple timeouts in the expected order |
| 22 | Deferred tasks, nested interrupts, and priority-based task preemption | **Lab3-A2** | Allow higher-priority work to interrupt lower-priority processing without corrupting context |

Step 18 needs one user execution context, not a complete process system. Establish
fault diagnostics here, but defer demand allocation until regions and process
lifecycle handling exist. Keep vectors, kernel stacks, callbacks, and their data
mapped throughout interrupt and exception handling.

### Steps 23-28: Threads, Processes, and Integration

| Step | Implementation | Exercise coverage | Completion checkpoint |
| --- | --- | --- | --- |
| 23 | Kernel threads, cooperative round-robin switching, idle thread, and zombie reclamation | **Lab5-B1** | Run multiple yielding threads and reclaim their resources after completion |
| 24 | Timer-driven kernel preemption and critical-section protection | **Lab5-B2 (partial)** | Share the CPU between threads that never voluntarily yield |
| 25 | Per-process address spaces, user scheduling/preemption, and getpid/UART syscalls | **Lab5-B2 (partial)** + **Lab6-B2 (partial)** | Run separate processes at the same user VAs with independent backing memory |
| 26 | Exec, eager-copy fork, exit, kill, user mailbox calls, and complete cleanup | **Lab5-B2 (complete)** + **Lab6-B2 (complete)** | Preserve process isolation and correctly reclaim page tables and backing memory |
| 27 | Signal registration/delivery, user handler stacks, and return trampolines | **Lab5-A1** | Execute registered handlers in EL0 and restore interrupted context |
| 28 | Video Player, display mappings, timer access, and interactive process operations | **Lab5-Video** + **Lab6-Video** | Run the Lab 6 test program with video playback and a responsive user shell |

Implement address-space switching alongside user scheduling. Keep kernel stacks and
metadata accessible through shared kernel mappings. Fork preserves user VAs and
copies private writable memory; it does not scan and relocate stack values.

Reserve syscall number 10 for mmap when designing the syscall ABI, and choose a
nonconflicting internal sigreturn number. Complete signals before the full Video
Player integration checkpoint.

At Step 28, all Lab 1-5 functionality and the basic Lab 6 VM integration are covered.
The roadmap continues through the remaining Lab 6 advanced exercises below.

### Steps 29-31: Advanced Virtual Memory

| Step | Implementation | Exercise coverage | Completion checkpoint |
| --- | --- | --- | --- |
| 29 | Anonymous regions, protection, and the mmap syscall | **Lab6-A1** | Handle address selection, alignment, overlap, and access protection |
| 30 | Fault-driven allocation and invalid-access termination | **Lab6-A2** | Allocate backing pages on demand and terminate the process on illegal access |
| 31 | Copy-on-write fork and shared-frame reference counting | **Lab6-A3** | Resolve writes to shared pages correctly; repeat process, mailbox, and Video Player checks |

These are part of the full Lab 1-6 route. The final checkpoint requires all earlier
capabilities to continue working with demand paging and COW enabled.

## Roadmap Sources

- [Lab 1 spec](../Lab1/spec.pdf)
- [Lab 2 spec](../Lab2/spec.pdf)
- [Lab 3 spec](../Lab3/spec.pdf)
- [Lab 4 spec](../Lab4/spec.pdf)
- [Lab 5 spec](../Lab5/spec.pdf)
- [Lab 6 spec](spec.pdf)
