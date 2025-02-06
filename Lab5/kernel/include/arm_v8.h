#ifndef __ARM_V8_H__
#define __ARM_V8_H__

/***************************************************************************
 * D8.2.79  SCTLR_EL1, System Control Register (EL1)
 * Page 2025 of AArch64-Reference-Manual 
 ***************************************************************************/

#define SCTLR_EE_LITTLE_ENDIAN  (0 << 25)
#define SCTLR_EE_BIG_ENDIAN     (1 << 25)
#define SCTLR_E0E_LITTLE_ENDIAN (0 << 24)
#define SCTLR_E0E_BIG_ENDIAN    (1 << 24)
#define SCTLR_I_CACHE_DISABLED  (0 << 12)
#define SCTLR_I_CACHE_ENABLED   (1 << 12)
#define SCTLR_D_CACHE_DISABLED  (0 << 2)
#define SCTLR_D_CACHE_ENABLED   (1 << 2)
#define SCTLR_MMU_DISABLED      (0 << 0)
#define SCTLR_MMU_ENABLED       (1 << 0)

#define SCTLR_VALUE             \
    (SCTLR_EE_LITTLE_ENDIAN | SCTLR_E0E_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | \
     SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

/***************************************************************************
 * D8.2.32 HCR_EL2, Hypervisor Configuration Register
 * Page 1923 of AArch64-Reference-Manual
 ***************************************************************************/

#define HCR_RW_AARCH32          (0 << 31) 
#define HCR_RW_AARCH64          (1 << 31)

#define HCR_VALUE               (HCR_RW_AARCH64)

/***************************************************************************
 * D8.2.78  SCR_EL3, Secure Configuration Register
 * Page 2022 of AArch64-Reference-Manual
 ***************************************************************************/

#define SCR_RW_AARCH32          (0 << 10)
#define SCR_RW_AARCH64          (1 << 10)
#define SCR_NS_SECURE           (0 << 0)
#define SCR_NS_NON_SECURE       (1 << 0)

#define SCR_VALUE               (SCR_RW_AARCH64 | SCR_NS_NON_SECURE)

/***************************************************************************
 * D1.6.4   Saved Program Status Registers (SPSRs)
 * Page 1417 of AArch64-Reference-Manual
 ***************************************************************************/

#define SPSR_MASK_D             (1 << 9)
#define SPSR_MASK_A             (1 << 8)
#define SPSR_MASK_I             (1 << 7)
#define SPSR_MASK_F             (1 << 6)
#define SPSR_MASK_ALL           (SPSR_MASK_D | SPSR_MASK_A | SPSR_MASK_I | SPSR_MASK_F)
#define SPSR_MODE_EL0t          0x00000000
#define SPSR_MODE_EL1t          0x00000004
#define SPSR_MODE_EL1h          0x00000005
#define SPSR_MODE_EL2t          0x00000008
#define SPSR_MODE_EL2h          0x00000009
#define SPSR_MODE_EL3t          0x0000000c
#define SPSR_MODE_EL3h          0x0000000d

#define SPSR_VALUE              (SPSR_MASK_ALL | SPSR_MODE_EL1h)

/***************************************************************************
 * D8.2.24  ESR_EL1, Exception Syndrome Register (EL1)
 * Page 1899 of AArch64-Reference-Manual
 ***************************************************************************/

#define ESR_ELx_EC_SHIFT        26
#define ESR_ELx_EC_SVC64        0x15

#endif