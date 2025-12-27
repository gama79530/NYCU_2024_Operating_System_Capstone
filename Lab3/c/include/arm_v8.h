#ifndef LAB3_C_ARM_V8_H
#define LAB3_C_ARM_V8_H

/*  D8.5.2 CNTHCTL_EL2, Counter-timer Hypervisor Control register
    Page D8-2171 of AArch64-Reference-Manual */
#define EL1PCEN \
    (1 << 1)  // EL1PCEN[1]: Controls whether the physical timer registers are accessible from
              // Non-secure EL1 and EL0 modes
#define EL1PCTEN \
    (1)  // EL1PCTEN[0]: Controls whether the physical counter, CNTPCT_EL0, is accessible from
         // Non-secure EL1 and EL0 modes
#define CNTHCTL_EL2 (EL1PCEN | EL1PCTEN)


/*  D8.5.18 CNTVOFF_EL2, Counter-timer Virtual Offset register
    Page D8-2193 of AArch64-Reference-Manual */
#define CNTVOFF_EL2 \
    0  // CNTVCT_EL0 (virtual timer) = CNTPCT_EL0 (physical timer) + CNTVOFF_EL2 (offset)


/*  D8.2.78 SCR_EL3, Secure Configuration Register
    Page D8-2022 of AArch64-Reference-Manual */
#define NS_EL3 1                     // NS, bit [0]: Non-secure bit.
#define RES1_EL3 (1 << 5 || 1 << 4)  // RES1, bits [5:4]: Reserved
#define SMD_EL3 (1 << 7)             // SMD, bit [7]: SMC Disable.
#define HCE_EL3 \
    (1 << 8)  // HCE, bit [8]: Hypervisor Call enable. This bit enables use of the HVC instruction
              // from Non-secure EL1 modes
#define RW_EL3 (1 << 10)  // RW, bit [10]: Register width control for lower exception levels.
#define SCR_EL3 (NS_EL3 | RES1_EL3 | SMD_EL3 | HCE_EL3 | RW_EL3)  // 0x5b1


/*  C4.3.19 SPSR_EL3, Saved Program Status Register (EL3)
    Page C4-291 of AArch64-Reference-Manual */
#define MODE_EL3 (1 << 3 | 1 << 0)  // EL2h
#define F_EL3 (1 << 6)              // F, bit [6]: FIQ mask bit.
#define I_EL3 (1 << 7)              // I, bit [7]: IRQ mask bit.
#define A_EL3 (1 << 8)              // A, bit [8]: SError (System Error) mask bit.
#define D_EL3 (1 << 9)              // D, bit [9]: Process state D mask.
#define SPSR_EL3 (MODE_EL3 | F_EL3 | I_EL3 | A_EL3 | D_EL3)  // 0x3c9


/*  D8.2.32 HCR_EL2, Hypervisor Configuration Register
    Page D8-1923 of AArch64-Reference-Manual */
#define RW_EL2 (1 << 31)   // RW, bit [31]: Register Width control for lower exception levels
#define SWIO_EL2 (1 << 1)  // SWIO, bit [1]: Set/Way Invalidation Override.
#define HCR_EL2 (RW_EL2 | SWIO_EL2)  // TGE, bit [31]: Traps General Exceptions to EL2


/*  C4.3.18 SPSR_EL2, Saved Program Status Register (EL2)
    Page C4-286 of AArch64-Reference-Manual */
#define MODE_EL2 (1 << 2 | 1 << 0)  // EL1h
#define F_EL2 (1 << 6)              // F, bit [6]: FIQ mask bit.
#define I_EL2 (1 << 7)              // I, bit [7]: IRQ mask bit.
#define A_EL2 (1 << 8)              // A, bit [8]: SError (System Error) mask bit.
#define D_EL2 (1 << 9)              // D, bit [9]: Process state D mask.
#define SPSR_EL2 (MODE_EL2 | F_EL2 | I_EL2 | A_EL2 | D_EL2)  // 0x3c5


/*  D8.2.79 SCTLR_EL1, System Control Register (EL1)
    Page D8-2025 of AArch64-Reference-Manual */
#define M_EL1 0         // M, bit [0]: MMU enable for EL1 and EL0 stage 1 address translation.
#define C_EL1 (0 << 2)  // C, bit [2]: Cache enable.
#define RES1_EL1                                       \
    (1 << 11 | 1 << 20 | 1 << 22 | 1 << 23 | 1 << 28 | \
     1 << 29)              // RES1, Bit [11], Bit [20], Bits [23:22]
#define I_EL1 (0 << 12)    // I, bit [12]: Instruction cache enable.
#define E0E_EL1 (0 << 24)  // E0E, bit [24]: Endianness of explicit data accesses at EL0.
#define EE_EL1 (0 << 25)   // EE, bit [25]: Exception Endianness.
#define SCTLR_EL1 (M_EL1 | C_EL1 | RES1_EL1 | I_EL1 | E0E_EL1 | EE_EL1)
#endif