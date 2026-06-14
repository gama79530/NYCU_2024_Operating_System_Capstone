# Project Agent Instructions

## Project Mission

This repository implements the NYCU Operating System Capstone 2024 labs from
Lab 0 through Lab 8. The target is a small bare-metal operating system for
Raspberry Pi 3, developed and tested with both QEMU and real hardware.

## Hardware Context

- Target board: Raspberry Pi 3.
- CPU architecture: ARMv8-A / AArch64.
- Execution model: bare-metal kernel code, without a host operating system.
- Early boot starts from the Raspberry Pi firmware loading `kernel8.img`.
- The kernel is expected to manage its own linker layout, stack setup, BSS
  clearing, exception levels, interrupt state, memory mappings, and device
  access.
- Current development assumes single-core kernel execution. Keep secondary
  cores idle unless a lab or user request explicitly introduces multicore work.
- Treat low-level kernel data accesses as requiring 8-byte alignment by default.
  Avoid casting unaligned addresses to typed pointers. Use byte-wise access or
  explicit packing/parsing when handling unaligned data formats.
- Validate with QEMU when possible, but remember QEMU behavior can differ from
  real Raspberry Pi 3 hardware.

## Hardware-Driven Software Constraints

- Hardware registers and peripheral memory must be accessed through explicit
  memory-mapped I/O operations. Avoid assuming normal memory semantics for
  device registers.
- Do not assume standard-library, heap, filesystem, process, or OS services are
  available in kernel code unless this project has implemented them.
- Code that crosses the C and assembly boundary must preserve the required
  AArch64 calling convention and stack alignment.

## Commit Message Format

Use Conventional Commits with a concise imperative summary:

```text
<type>(<scope>): <summary>
```

Allowed types:

- `feat`: add a user-visible lab/kernel capability
- `fix`: correct broken behavior
- `refactor`: restructure code without changing behavior
- `test`: add or update tests
- `build`: change build scripts, linker flow, or toolchain setup
- `docs`: update documentation only
- `chore`: maintenance that does not affect runtime behavior

Allowed scopes are `lab0`, `lab1`, `lab2`, `lab3`, `lab4`, `lab5`, `lab6`,
`lab7`, `lab8`, and `uploader`.

Subject rules:

- Capitalize the summary.
- Use imperative mood: "Add", "Fix", "Parse", "Initialize".
- Do not end the summary with a period.
- Keep the summary itself around 50 characters, excluding `<type>(<scope>):`.
- Keep the full header readable; prefer staying under 72 characters when it
  does not make the summary worse.

Examples:

```text
feat(lab1): Add mini UART initialization
fix(lab3): Clear core timer interrupt status
build(lab0): Add kernel image build rule
```

Use a commit body when the change needs context. Separate it from the header
with a blank line and wrap it around 72 characters. Explain what changed and
why; let the diff explain how.
