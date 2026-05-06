/**
 * @FilePath     : /ZZZ-OS/arch/arm/irq/gic.h
 * @Description  :
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 18:05:14
 * @LastEditTime : 2026-03-22 18:32:04
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
 */
#ifndef __CORTEX_CA7_H
#define __CORTEX_CA7_H
/***************************************************************
Copyright © zuozhongkai Co., Ltd. 1998-2019. All rights reserved.
文件名	: 	 core_ca7.h
作者	   : 左忠凯
版本	   : V1.0
描述	   : Cortex-A7内核通用文件。
其他	   : 本文件主要实现了对GIC操作函数
论坛 	   : www.wtmembed.com
日志	   : 初版V1.0 2019/1/4 左忠凯创建
***************************************************************/
#include <os/string.h>
#include <os/types.h>
#include <os/compiler.h>

#define FORCEDINLINE __attribute__((always_inline))
#define __ASM __asm     /* GNU C语言内嵌汇编关键字 */
#define __INLINE inline /* GNU内联关键字 */
#define __STATIC_INLINE static inline

#define __IM volatile const /* 只读 */
#define __OM volatile       /* 只写 */
#define __IOM volatile      /* 读写 */
#define __STRINGIFY(x) #x

/* C语言实现MCR指令 */
#define __MCR(coproc, opcode_1, src, CRn, CRm, opcode_2)                         \
    __ASM volatile("MCR " __STRINGIFY(p##coproc) ", " __STRINGIFY(opcode_1) ", " \
                                                                            "%0, " __STRINGIFY(c##CRn) ", " __STRINGIFY(c##CRm) ", " __STRINGIFY(opcode_2) : : "r"(src))

/* C语言实现MRC指令 */
#define __MRC(coproc, opcode_1, CRn, CRm, opcode_2)                                                                                                                            \
    ({                                                                                                                                                                         \
        u32 __dst;                                                                                                                                                        \
        __ASM volatile("MRC " __STRINGIFY(p##coproc) ", " __STRINGIFY(opcode_1) ", "                                                                                           \
                                                                                "%0, " __STRINGIFY(c##CRn) ", " __STRINGIFY(c##CRm) ", " __STRINGIFY(opcode_2) : "=r"(__dst)); \
        __dst;                                                                                                                                                                 \
    })

/* 其他一些C语言内嵌汇编 */
static __always_inline void __set_APSR(u32 apsr) {
    __ASM volatile("MSR apsr, %0" : : "r"(apsr) : "cc");
}

static __always_inline u32 __get_CPSR(void) {
    u32 result;

    __ASM volatile("MRS %0, cpsr" : "=r"(result));
    return (result);
}

static __always_inline void __set_CPSR(u32 cpsr) {
    __ASM volatile("MSR cpsr, %0" : : "r"(cpsr) : "cc");
}

static __always_inline u32 __get_FPEXC(void) {
    u32 result;

    __ASM volatile("VMRS %0, fpexc" : "=r"(result));
    return result;
}

static __always_inline void __set_FPEXC(u32 fpexc) {
    __ASM volatile("VMSR fpexc, %0" : : "r"(fpexc));
}

typedef enum IRQn {
    /* Auxiliary constants */
    NotAvail_IRQn = -128, /**< Not available device specific interrupt */

    /* Core interrupts */
    Software0_IRQn = 0,           /**< Cortex-A7 Software Generated Interrupt 0 */
    Software1_IRQn = 1,           /**< Cortex-A7 Software Generated Interrupt 1 */
    Software2_IRQn = 2,           /**< Cortex-A7 Software Generated Interrupt 2 */
    Software3_IRQn = 3,           /**< Cortex-A7 Software Generated Interrupt 3 */
    Software4_IRQn = 4,           /**< Cortex-A7 Software Generated Interrupt 4 */
    Software5_IRQn = 5,           /**< Cortex-A7 Software Generated Interrupt 5 */
    Software6_IRQn = 6,           /**< Cortex-A7 Software Generated Interrupt 6 */
    Software7_IRQn = 7,           /**< Cortex-A7 Software Generated Interrupt 7 */
    Software8_IRQn = 8,           /**< Cortex-A7 Software Generated Interrupt 8 */
    Software9_IRQn = 9,           /**< Cortex-A7 Software Generated Interrupt 9 */
    Software10_IRQn = 10,         /**< Cortex-A7 Software Generated Interrupt 10 */
    Software11_IRQn = 11,         /**< Cortex-A7 Software Generated Interrupt 11 */
    Software12_IRQn = 12,         /**< Cortex-A7 Software Generated Interrupt 12 */
    Software13_IRQn = 13,         /**< Cortex-A7 Software Generated Interrupt 13 */
    Software14_IRQn = 14,         /**< Cortex-A7 Software Generated Interrupt 14 */
    Software15_IRQn = 15,         /**< Cortex-A7 Software Generated Interrupt 15 */
    VirtualMaintenance_IRQn = 25, /**< Cortex-A7 Virtual Maintenance Interrupt */
    HypervisorTimer_IRQn = 26,    /**< Cortex-A7 Hypervisor Timer Interrupt */
    VirtualTimer_IRQn = 27,       /**< Cortex-A7 Virtual Timer Interrupt */
    LegacyFastInt_IRQn = 28,      /**< Cortex-A7 Legacy nFIQ signal Interrupt */
    SecurePhyTimer_IRQn = 29,     /**< Cortex-A7 Secure Physical Timer Interrupt */
    NonSecurePhyTimer_IRQn = 30,  /**< Cortex-A7 Non-secure Physical Timer Interrupt */
    LegacyIRQ_IRQn = 31,          /**< Cortex-A7 Legacy nIRQ Interrupt */

    /* Device specific interrupts */
    IOMUXC_IRQn = 32,                /**< General Purpose Register 1 from IOMUXC. Used to notify cores on exception condition while boot. */
    DAP_IRQn = 33,                   /**< Debug Access Port interrupt request. */
    SDMA_IRQn = 34,                  /**< SDMA interrupt request from all channels. */
    TSC_IRQn = 35,                   /**< TSC interrupt. */
    SNVS_IRQn = 36,                  /**< Logic OR of SNVS_LP and SNVS_HP interrupts. */
    LCDIF_IRQn = 37,                 /**< LCDIF sync interrupt. */
    RNGB_IRQn = 38,                  /**< RNGB interrupt. */
    CSI_IRQn = 39,                   /**< CMOS Sensor Interface interrupt request. */
    PXP_IRQ0_IRQn = 40,              /**< PXP interrupt pxp_irq_0. */
    SCTR_IRQ0_IRQn = 41,             /**< SCTR compare interrupt ipi_int[0]. */
    SCTR_IRQ1_IRQn = 42,             /**< SCTR compare interrupt ipi_int[1]. */
    WDOG3_IRQn = 43,                 /**< WDOG3 timer reset interrupt request. */
    Reserved44_IRQn = 44,            /**< Reserved */
    APBH_IRQn = 45,                  /**< DMA Logical OR of APBH DMA channels 0-3 completion and error interrupts. */
    WEIM_IRQn = 46,                  /**< WEIM interrupt request. */
    RAWNAND_BCH_IRQn = 47,           /**< BCH operation complete interrupt. */
    RAWNAND_GPMI_IRQn = 48,          /**< GPMI operation timeout error interrupt. */
    UART6_IRQn = 49,                 /**< UART6 interrupt request. */
    PXP_IRQ1_IRQn = 50,              /**< PXP interrupt pxp_irq_1. */
    SNVS_Consolidated_IRQn = 51,     /**< SNVS consolidated interrupt. */
    SNVS_Security_IRQn = 52,         /**< SNVS security interrupt. */
    CSU_IRQn = 53,                   /**< CSU interrupt request 1. Indicates to the processor that one or more alarm inputs were asserted. */
    USDHC1_IRQn = 54,                /**< USDHC1 (Enhanced SDHC) interrupt request. */
    USDHC2_IRQn = 55,                /**< USDHC2 (Enhanced SDHC) interrupt request. */
    SAI3_RX_IRQn = 56,               /**< SAI3 interrupt ipi_int_sai_rx. */
    SAI3_TX_IRQn = 57,               /**< SAI3 interrupt ipi_int_sai_tx. */
    UART1_IRQn = 58,                 /**< UART1 interrupt request. */
    UART2_IRQn = 59,                 /**< UART2 interrupt request. */
    UART3_IRQn = 60,                 /**< UART3 interrupt request. */
    UART4_IRQn = 61,                 /**< UART4 interrupt request. */
    UART5_IRQn = 62,                 /**< UART5 interrupt request. */
    eCSPI1_IRQn = 63,                /**< eCSPI1 interrupt request. */
    eCSPI2_IRQn = 64,                /**< eCSPI2 interrupt request. */
    eCSPI3_IRQn = 65,                /**< eCSPI3 interrupt request. */
    eCSPI4_IRQn = 66,                /**< eCSPI4 interrupt request. */
    I2C4_IRQn = 67,                  /**< I2C4 interrupt request. */
    I2C1_IRQn = 68,                  /**< I2C1 interrupt request. */
    I2C2_IRQn = 69,                  /**< I2C2 interrupt request. */
    I2C3_IRQn = 70,                  /**< I2C3 interrupt request. */
    UART7_IRQn = 71,                 /**< UART-7 ORed interrupt. */
    UART8_IRQn = 72,                 /**< UART-8 ORed interrupt. */
    Reserved73_IRQn = 73,            /**< Reserved */
    USB_OTG2_IRQn = 74,              /**< USBO2 USB OTG2 */
    USB_OTG1_IRQn = 75,              /**< USBO2 USB OTG1 */
    USB_PHY1_IRQn = 76,              /**< UTMI0 interrupt request. */
    USB_PHY2_IRQn = 77,              /**< UTMI1 interrupt request. */
    DCP_IRQ_IRQn = 78,               /**< DCP interrupt request dcp_irq. */
    DCP_VMI_IRQ_IRQn = 79,           /**< DCP interrupt request dcp_vmi_irq. */
    DCP_SEC_IRQ_IRQn = 80,           /**< DCP interrupt request secure_irq. */
    TEMPMON_IRQn = 81,               /**< Temperature Monitor Temperature Sensor (temperature greater than threshold) interrupt request. */
    ASRC_IRQn = 82,                  /**< ASRC interrupt request. */
    ESAI_IRQn = 83,                  /**< ESAI interrupt request. */
    SPDIF_IRQn = 84,                 /**< SPDIF interrupt. */
    Reserved85_IRQn = 85,            /**< Reserved */
    PMU_IRQ1_IRQn = 86,              /**< Brown-out event on either the 1.1, 2.5 or 3.0 regulators. */
    GPT1_IRQn = 87,                  /**< Logical OR of GPT1 rollover interrupt line, input capture 1 and 2 lines, output compare 1, 2, and 3 interrupt lines. */
    EPIT1_IRQn = 88,                 /**< EPIT1 output compare interrupt. */
    EPIT2_IRQn = 89,                 /**< EPIT2 output compare interrupt. */
    GPIO1_INT7_IRQn = 90,            /**< INT7 interrupt request. */
    GPIO1_INT6_IRQn = 91,            /**< INT6 interrupt request. */
    GPIO1_INT5_IRQn = 92,            /**< INT5 interrupt request. */
    GPIO1_INT4_IRQn = 93,            /**< INT4 interrupt request. */
    GPIO1_INT3_IRQn = 94,            /**< INT3 interrupt request. */
    GPIO1_INT2_IRQn = 95,            /**< INT2 interrupt request. */
    GPIO1_INT1_IRQn = 96,            /**< INT1 interrupt request. */
    GPIO1_INT0_IRQn = 97,            /**< INT0 interrupt request. */
    GPIO1_Combined_0_15_IRQn = 98,   /**< Combined interrupt indication for GPIO1 signals 0 - 15. */
    GPIO1_Combined_16_31_IRQn = 99,  /**< Combined interrupt indication for GPIO1 signals 16 - 31. */
    GPIO2_Combined_0_15_IRQn = 100,  /**< Combined interrupt indication for GPIO2 signals 0 - 15. */
    GPIO2_Combined_16_31_IRQn = 101, /**< Combined interrupt indication for GPIO2 signals 16 - 31. */
    GPIO3_Combined_0_15_IRQn = 102,  /**< Combined interrupt indication for GPIO3 signals 0 - 15. */
    GPIO3_Combined_16_31_IRQn = 103, /**< Combined interrupt indication for GPIO3 signals 16 - 31. */
    GPIO4_Combined_0_15_IRQn = 104,  /**< Combined interrupt indication for GPIO4 signals 0 - 15. */
    GPIO4_Combined_16_31_IRQn = 105, /**< Combined interrupt indication for GPIO4 signals 16 - 31. */
    GPIO5_Combined_0_15_IRQn = 106,  /**< Combined interrupt indication for GPIO5 signals 0 - 15. */
    GPIO5_Combined_16_31_IRQn = 107, /**< Combined interrupt indication for GPIO5 signals 16 - 31. */
    Reserved108_IRQn = 108,          /**< Reserved */
    Reserved109_IRQn = 109,          /**< Reserved */
    Reserved110_IRQn = 110,          /**< Reserved */
    Reserved111_IRQn = 111,          /**< Reserved */
    WDOG1_IRQn = 112,                /**< WDOG1 timer reset interrupt request. */
    WDOG2_IRQn = 113,                /**< WDOG2 timer reset interrupt request. */
    KPP_IRQn = 114,                  /**< Key Pad interrupt request. */
    PWM1_IRQn = 115,                 /**< hasRegInstance(`PWM1`)?`Cumulative interrupt line for PWM1. Logical OR of rollover, compare, and FIFO waterlevel crossing interrupts.`:`Reserved`) */
    PWM2_IRQn = 116,                 /**< hasRegInstance(`PWM2`)?`Cumulative interrupt line for PWM2. Logical OR of rollover, compare, and FIFO waterlevel crossing interrupts.`:`Reserved`) */
    PWM3_IRQn = 117,                 /**< hasRegInstance(`PWM3`)?`Cumulative interrupt line for PWM3. Logical OR of rollover, compare, and FIFO waterlevel crossing interrupts.`:`Reserved`) */
    PWM4_IRQn = 118,                 /**< hasRegInstance(`PWM4`)?`Cumulative interrupt line for PWM4. Logical OR of rollover, compare, and FIFO waterlevel crossing interrupts.`:`Reserved`) */
    CCM_IRQ1_IRQn = 119,             /**< CCM interrupt request ipi_int_1. */
    CCM_IRQ2_IRQn = 120,             /**< CCM interrupt request ipi_int_2. */
    GPC_IRQn = 121,                  /**< GPC interrupt request 1. */
    Reserved122_IRQn = 122,          /**< Reserved */
    SRC_IRQn = 123,                  /**< SRC interrupt request src_ipi_int_1. */
    Reserved124_IRQn = 124,          /**< Reserved */
    Reserved125_IRQn = 125,          /**< Reserved */
    CPU_PerformanceUnit_IRQn = 126,  /**< Performance Unit interrupt ~ipi_pmu_irq_b. */
    CPU_CTI_Trigger_IRQn = 127,      /**< CTI trigger outputs interrupt ~ipi_cti_irq_b. */
    SRC_Combined_IRQn = 128,         /**< Combined CPU wdog interrupts (4x) out of SRC. */
    SAI1_IRQn = 129,                 /**< SAI1 interrupt request. */
    SAI2_IRQn = 130,                 /**< SAI2 interrupt request. */
    Reserved131_IRQn = 131,          /**< Reserved */
    ADC1_IRQn = 132,                 /**< ADC1 interrupt request. */
    ADC_5HC_IRQn = 133,              /**< ADC_5HC interrupt request. */
    Reserved134_IRQn = 134,          /**< Reserved */
    Reserved135_IRQn = 135,          /**< Reserved */
    SJC_IRQn = 136,                  /**< SJC interrupt from General Purpose register. */
    CAAM_Job_Ring0_IRQn = 137,       /**< CAAM job ring 0 interrupt ipi_caam_irq0. */
    CAAM_Job_Ring1_IRQn = 138,       /**< CAAM job ring 1 interrupt ipi_caam_irq1. */
    QSPI_IRQn = 139,                 /**< QSPI1 interrupt request ipi_int_ored. */
    TZASC_IRQn = 140,                /**< TZASC (PL380) interrupt request. */
    GPT2_IRQn = 141,                 /**< Logical OR of GPT2 rollover interrupt line, input capture 1 and 2 lines, output compare 1, 2 and 3 interrupt lines. */
    CAN1_IRQn = 142,                 /**< Combined interrupt of ini_int_busoff,ini_int_error,ipi_int_mbor,ipi_int_txwarning and ipi_int_waken */
    CAN2_IRQn = 143,                 /**< Combined interrupt of ini_int_busoff,ini_int_error,ipi_int_mbor,ipi_int_txwarning and ipi_int_waken */
    Reserved144_IRQn = 144,          /**< Reserved */
    Reserved145_IRQn = 145,          /**< Reserved */
    PWM5_IRQn = 146,                 /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO Waterlevel crossing interrupt line */
    PWM6_IRQn = 147,                 /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO Waterlevel crossing interrupt line */
    PWM7_IRQn = 148,                 /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO Waterlevel crossing interrupt line */
    PWM8_IRQn = 149,                 /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO Waterlevel crossing interrupt line */
    ENET1_IRQn = 150,                /**< ENET1 interrupt */
    ENET1_1588_IRQn = 151,           /**< ENET1 1588 Timer interrupt [synchronous] request. */
    ENET2_IRQn = 152,                /**< ENET2 interrupt */
    ENET2_1588_IRQn = 153,           /**< MAC 0 1588 Timer interrupt [synchronous] request. */
    Reserved154_IRQn = 154,          /**< Reserved */
    Reserved155_IRQn = 155,          /**< Reserved */
    Reserved156_IRQn = 156,          /**< Reserved */
    Reserved157_IRQn = 157,          /**< Reserved */
    Reserved158_IRQn = 158,          /**< Reserved */
    PMU_IRQ2_IRQn = 159              /**< Brown-out event on either core, gpu or soc regulators. */
} IRQn_Type;

/*******************************************************************************
 *        		一些内核寄存器定义和抽象
  定义如下几个内核寄存器:
  - CPSR
  - CP15
 ******************************************************************************/

/* CPSR寄存器
 * 参考资料：ARM Cortex-A(armV7)编程手册V4.0.pdf P46
 */
typedef union {
    struct
    {
        u32 M : 5;          /*!< bit:  0.. 4  Mode field */
        u32 T : 1;          /*!< bit:      5  Thumb execution state bit */
        u32 F : 1;          /*!< bit:      6  FIQ mask bit */
        u32 I : 1;          /*!< bit:      7  IRQ mask bit */
        u32 A : 1;          /*!< bit:      8  Asynchronous abort mask bit */
        u32 E : 1;          /*!< bit:      9  Endianness execution state bit */
        u32 IT1 : 6;        /*!< bit: 10..15  If-Then execution state bits 2-7 */
        u32 GE : 4;         /*!< bit: 16..19  Greater than or Equal flags */
        u32 _reserved0 : 4; /*!< bit: 20..23  Reserved */
        u32 J : 1;          /*!< bit:     24  Jazelle bit */
        u32 IT0 : 2;        /*!< bit: 25..26  If-Then execution state bits 0-1 */
        u32 Q : 1;          /*!< bit:     27  Saturation condition flag */
        u32 V : 1;          /*!< bit:     28  Overflow condition code flag */
        u32 C : 1;          /*!< bit:     29  Carry condition code flag */
        u32 Z : 1;          /*!< bit:     30  Zero condition code flag */
        u32 N : 1;          /*!< bit:     31  Negative condition code flag */
    } b;                         /*!< Structure used for bit  access */
    u32 w;                  /*!< Type      used for word access */
} CPSR_Type;

/* CP15的SCTLR寄存器
 * 参考资料：Cortex-A7 Technical ReferenceManua.pdf P105
 */
typedef union {
    struct
    {
        u32 M : 1;          /*!< bit:     0  MMU enable */
        u32 A : 1;          /*!< bit:     1  Alignment check enable */
        u32 C : 1;          /*!< bit:     2  Cache enable */
        u32 _reserved0 : 2; /*!< bit: 3.. 4  Reserved */
        u32 CP15BEN : 1;    /*!< bit:     5  CP15 barrier enable */
        u32 _reserved1 : 1; /*!< bit:     6  Reserved */
        u32 B : 1;          /*!< bit:     7  Endianness model */
        u32 _reserved2 : 2; /*!< bit: 8.. 9  Reserved */
        u32 SW : 1;         /*!< bit:    10  SWP and SWPB enable */
        u32 Z : 1;          /*!< bit:    11  Branch prediction enable */
        u32 I : 1;          /*!< bit:    12  Instruction cache enable */
        u32 V : 1;          /*!< bit:    13  Vectors bit */
        u32 RR : 1;         /*!< bit:    14  Round Robin select */
        u32 _reserved3 : 2; /*!< bit:15..16  Reserved */
        u32 HA : 1;         /*!< bit:    17  Hardware Access flag enable */
        u32 _reserved4 : 1; /*!< bit:    18  Reserved */
        u32 WXN : 1;        /*!< bit:    19  Write permission implies XN */
        u32 UWXN : 1;       /*!< bit:    20  Unprivileged write permission implies PL1 XN */
        u32 FI : 1;         /*!< bit:    21  Fast interrupts configuration enable */
        u32 U : 1;          /*!< bit:    22  Alignment model */
        u32 _reserved5 : 1; /*!< bit:    23  Reserved */
        u32 VE : 1;         /*!< bit:    24  Interrupt Vectors Enable */
        u32 EE : 1;         /*!< bit:    25  Exception Endianness */
        u32 _reserved6 : 1; /*!< bit:    26  Reserved */
        u32 NMFI : 1;       /*!< bit:    27  Non-maskable FIQ (NMFI) support */
        u32 TRE : 1;        /*!< bit:    28  TEX remap enable. */
        u32 AFE : 1;        /*!< bit:    29  Access flag enable */
        u32 TE : 1;         /*!< bit:    30  Thumb Exception enable */
        u32 _reserved7 : 1; /*!< bit:    31  Reserved */
    } b;                         /*!< Structure used for bit  access */
    u32 w;                  /*!< Type      used for word access */
} SCTLR_Type;

#define __GIC_PRIO_BITS 5 /**< Number of Bits used for Priority Levels */

/* CP15 寄存器SCTLR各个位定义 */
#define SCTLR_TE_Pos 30U                   /*!< SCTLR: TE Position */
#define SCTLR_TE_Msk (1UL << SCTLR_TE_Pos) /*!< SCTLR: TE Mask */

#define SCTLR_AFE_Pos 29U                    /*!< SCTLR: AFE Position */
#define SCTLR_AFE_Msk (1UL << SCTLR_AFE_Pos) /*!< SCTLR: AFE Mask */

#define SCTLR_TRE_Pos 28U                    /*!< SCTLR: TRE Position */
#define SCTLR_TRE_Msk (1UL << SCTLR_TRE_Pos) /*!< SCTLR: TRE Mask */

#define SCTLR_NMFI_Pos 27U                     /*!< SCTLR: NMFI Position */
#define SCTLR_NMFI_Msk (1UL << SCTLR_NMFI_Pos) /*!< SCTLR: NMFI Mask */

#define SCTLR_EE_Pos 25U                   /*!< SCTLR: EE Position */
#define SCTLR_EE_Msk (1UL << SCTLR_EE_Pos) /*!< SCTLR: EE Mask */

#define SCTLR_VE_Pos 24U                   /*!< SCTLR: VE Position */
#define SCTLR_VE_Msk (1UL << SCTLR_VE_Pos) /*!< SCTLR: VE Mask */

#define SCTLR_U_Pos 22U                  /*!< SCTLR: U Position */
#define SCTLR_U_Msk (1UL << SCTLR_U_Pos) /*!< SCTLR: U Mask */

#define SCTLR_FI_Pos 21U                   /*!< SCTLR: FI Position */
#define SCTLR_FI_Msk (1UL << SCTLR_FI_Pos) /*!< SCTLR: FI Mask */

#define SCTLR_UWXN_Pos 20U                     /*!< SCTLR: UWXN Position */
#define SCTLR_UWXN_Msk (1UL << SCTLR_UWXN_Pos) /*!< SCTLR: UWXN Mask */

#define SCTLR_WXN_Pos 19U                    /*!< SCTLR: WXN Position */
#define SCTLR_WXN_Msk (1UL << SCTLR_WXN_Pos) /*!< SCTLR: WXN Mask */

#define SCTLR_HA_Pos 17U                   /*!< SCTLR: HA Position */
#define SCTLR_HA_Msk (1UL << SCTLR_HA_Pos) /*!< SCTLR: HA Mask */

#define SCTLR_RR_Pos 14U                   /*!< SCTLR: RR Position */
#define SCTLR_RR_Msk (1UL << SCTLR_RR_Pos) /*!< SCTLR: RR Mask */

#define SCTLR_V_Pos 13U                  /*!< SCTLR: V Position */
#define SCTLR_V_Msk (1UL << SCTLR_V_Pos) /*!< SCTLR: V Mask */

#define SCTLR_I_Pos 12U                  /*!< SCTLR: I Position */
#define SCTLR_I_Msk (1UL << SCTLR_I_Pos) /*!< SCTLR: I Mask */

#define SCTLR_Z_Pos 11U                  /*!< SCTLR: Z Position */
#define SCTLR_Z_Msk (1UL << SCTLR_Z_Pos) /*!< SCTLR: Z Mask */

#define SCTLR_SW_Pos 10U                   /*!< SCTLR: SW Position */
#define SCTLR_SW_Msk (1UL << SCTLR_SW_Pos) /*!< SCTLR: SW Mask */

#define SCTLR_B_Pos 7U                   /*!< SCTLR: B Position */
#define SCTLR_B_Msk (1UL << SCTLR_B_Pos) /*!< SCTLR: B Mask */

#define SCTLR_CP15BEN_Pos 5U                         /*!< SCTLR: CP15BEN Position */
#define SCTLR_CP15BEN_Msk (1UL << SCTLR_CP15BEN_Pos) /*!< SCTLR: CP15BEN Mask */

#define SCTLR_C_Pos 2U                   /*!< SCTLR: C Position */
#define SCTLR_C_Msk (1UL << SCTLR_C_Pos) /*!< SCTLR: C Mask */

#define SCTLR_A_Pos 1U                   /*!< SCTLR: A Position */
#define SCTLR_A_Msk (1UL << SCTLR_A_Pos) /*!< SCTLR: A Mask */

#define SCTLR_M_Pos 0U                   /*!< SCTLR: M Position */
#define SCTLR_M_Msk (1UL << SCTLR_M_Pos) /*!< SCTLR: M Mask */

/* CP15的ACTLR寄存器
 * 参考资料:Cortex-A7 Technical ReferenceManua.pdf P113
 */
typedef union {
    struct
    {
        u32 _reserved0 : 6;  /*!< bit: 0.. 5  Reserved */
        u32 SMP : 1;         /*!< bit:     6  Enables coherent requests to the processor */
        u32 _reserved1 : 3;  /*!< bit: 7.. 9  Reserved */
        u32 DODMBS : 1;      /*!< bit:    10  Disable optimized data memory barrier behavior */
        u32 L2RADIS : 1;     /*!< bit:    11  L2 Data Cache read-allocate mode disable */
        u32 L1RADIS : 1;     /*!< bit:    12  L1 Data Cache read-allocate mode disable */
        u32 L1PCTL : 2;      /*!< bit:13..14  L1 Data prefetch control */
        u32 DDVM : 1;        /*!< bit:    15  Disable Distributed Virtual Memory (DVM) transactions */
        u32 _reserved3 : 12; /*!< bit:16..27  Reserved */
        u32 DDI : 1;         /*!< bit:    28  Disable dual issue */
        u32 _reserved7 : 3;  /*!< bit:29..31  Reserved */
    } b;                          /*!< Structure used for bit  access */
    u32 w;                   /*!< Type      used for word access */
} ACTLR_Type;

#define ACTLR_DDI_Pos 28U                    /*!< ACTLR: DDI Position */
#define ACTLR_DDI_Msk (1UL << ACTLR_DDI_Pos) /*!< ACTLR: DDI Mask */

#define ACTLR_DDVM_Pos 15U                     /*!< ACTLR: DDVM Position */
#define ACTLR_DDVM_Msk (1UL << ACTLR_DDVM_Pos) /*!< ACTLR: DDVM Mask */

#define ACTLR_L1PCTL_Pos 13U                       /*!< ACTLR: L1PCTL Position */
#define ACTLR_L1PCTL_Msk (3UL << ACTLR_L1PCTL_Pos) /*!< ACTLR: L1PCTL Mask */

#define ACTLR_L1RADIS_Pos 12U                        /*!< ACTLR: L1RADIS Position */
#define ACTLR_L1RADIS_Msk (1UL << ACTLR_L1RADIS_Pos) /*!< ACTLR: L1RADIS Mask */

#define ACTLR_L2RADIS_Pos 11U                        /*!< ACTLR: L2RADIS Position */
#define ACTLR_L2RADIS_Msk (1UL << ACTLR_L2RADIS_Pos) /*!< ACTLR: L2RADIS Mask */

#define ACTLR_DODMBS_Pos 10U                       /*!< ACTLR: DODMBS Position */
#define ACTLR_DODMBS_Msk (1UL << ACTLR_DODMBS_Pos) /*!< ACTLR: DODMBS Mask */

#define ACTLR_SMP_Pos 6U                     /*!< ACTLR: SMP Position */
#define ACTLR_SMP_Msk (1UL << ACTLR_SMP_Pos) /*!< ACTLR: SMP Mask */

/* CP15的CPACR寄存器
 * 参考资料：Cortex-A7 Technical ReferenceManua.pdf P115
 */
typedef union {
    struct
    {
        u32 _reserved0 : 20; /*!< bit: 0..19  Reserved */
        u32 cp10 : 2;        /*!< bit:20..21  Access rights for coprocessor 10 */
        u32 cp11 : 2;        /*!< bit:22..23  Access rights for coprocessor 11 */
        u32 _reserved1 : 6;  /*!< bit:24..29  Reserved */
        u32 D32DIS : 1;      /*!< bit:    30  Disable use of registers D16-D31 of the VFP register file */
        u32 ASEDIS : 1;      /*!< bit:    31  Disable Advanced SIMD Functionality */
    } b;                          /*!< Structure used for bit  access */
    u32 w;                   /*!< Type      used for word access */
} CPACR_Type;

#define CPACR_ASEDIS_Pos 31U                       /*!< CPACR: ASEDIS Position */
#define CPACR_ASEDIS_Msk (1UL << CPACR_ASEDIS_Pos) /*!< CPACR: ASEDIS Mask */

#define CPACR_D32DIS_Pos 30U                       /*!< CPACR: D32DIS Position */
#define CPACR_D32DIS_Msk (1UL << CPACR_D32DIS_Pos) /*!< CPACR: D32DIS Mask */

#define CPACR_cp11_Pos 22U                     /*!< CPACR: cp11 Position */
#define CPACR_cp11_Msk (3UL << CPACR_cp11_Pos) /*!< CPACR: cp11 Mask */

#define CPACR_cp10_Pos 20U                     /*!< CPACR: cp10 Position */
#define CPACR_cp10_Msk (3UL << CPACR_cp10_Pos) /*!< CPACR: cp10 Mask */

/* CP15的DFSR寄存器
 * 参考资料：Cortex-A7 Technical ReferenceManua.pdf P128
 */
typedef union {
    struct
    {
        u32 FS0 : 4;         /*!< bit: 0.. 3  Fault Status bits bit 0-3 */
        u32 Domain : 4;      /*!< bit: 4.. 7  Fault on which domain */
        u32 _reserved0 : 2;  /*!< bit: 8.. 9  Reserved */
        u32 FS1 : 1;         /*!< bit:    10  Fault Status bits bit 4 */
        u32 WnR : 1;         /*!< bit:    11  Write not Read bit */
        u32 ExT : 1;         /*!< bit:    12  External abort type */
        u32 CM : 1;          /*!< bit:    13  Cache maintenance fault */
        u32 _reserved1 : 18; /*!< bit:14..31  Reserved */
    } b;                          /*!< Structure used for bit  access */
    u32 w;                   /*!< Type      used for word access */
} DFSR_Type;

#define DFSR_CM_Pos 13U                  /*!< DFSR: CM Position */
#define DFSR_CM_Msk (1UL << DFSR_CM_Pos) /*!< DFSR: CM Mask */

#define DFSR_Ext_Pos 12U                   /*!< DFSR: Ext Position */
#define DFSR_Ext_Msk (1UL << DFSR_Ext_Pos) /*!< DFSR: Ext Mask */

#define DFSR_WnR_Pos 11U                   /*!< DFSR: WnR Position */
#define DFSR_WnR_Msk (1UL << DFSR_WnR_Pos) /*!< DFSR: WnR Mask */

#define DFSR_FS1_Pos 10U                   /*!< DFSR: FS1 Position */
#define DFSR_FS1_Msk (1UL << DFSR_FS1_Pos) /*!< DFSR: FS1 Mask */

#define DFSR_Domain_Pos 4U                         /*!< DFSR: Domain Position */
#define DFSR_Domain_Msk (0xFUL << DFSR_Domain_Pos) /*!< DFSR: Domain Mask */

#define DFSR_FS0_Pos 0U                      /*!< DFSR: FS0 Position */
#define DFSR_FS0_Msk (0xFUL << DFSR_FS0_Pos) /*!< DFSR: FS0 Mask */

/* CP15的IFSR寄存器
 * 参考资料：Cortex-A7 Technical ReferenceManua.pdf P131
 */
typedef union {
    struct
    {
        u32 FS0 : 4;         /*!< bit: 0.. 3  Fault Status bits bit 0-3 */
        u32 _reserved0 : 6;  /*!< bit: 4.. 9  Reserved */
        u32 FS1 : 1;         /*!< bit:    10  Fault Status bits bit 4 */
        u32 _reserved1 : 1;  /*!< bit:    11  Reserved */
        u32 ExT : 1;         /*!< bit:    12  External abort type */
        u32 _reserved2 : 19; /*!< bit:13..31  Reserved */
    } b;                          /*!< Structure used for bit  access */
    u32 w;                   /*!< Type      used for word access */
} IFSR_Type;

#define IFSR_ExT_Pos 12U                   /*!< IFSR: ExT Position */
#define IFSR_ExT_Msk (1UL << IFSR_ExT_Pos) /*!< IFSR: ExT Mask */

#define IFSR_FS1_Pos 10U                   /*!< IFSR: FS1 Position */
#define IFSR_FS1_Msk (1UL << IFSR_FS1_Pos) /*!< IFSR: FS1 Mask */

#define IFSR_FS0_Pos 0U                      /*!< IFSR: FS0 Position */
#define IFSR_FS0_Msk (0xFUL << IFSR_FS0_Pos) /*!< IFSR: FS0 Mask */

/* CP15的ISR寄存器
 * 参考资料：ARM ArchitectureReference Manual ARMv7-A and ARMv7-R edition.pdf P1640
 */
typedef union {
    struct
    {
        u32 _reserved0 : 6;  /*!< bit: 0.. 5  Reserved */
        u32 F : 1;           /*!< bit:     6  FIQ pending bit */
        u32 I : 1;           /*!< bit:     7  IRQ pending bit */
        u32 A : 1;           /*!< bit:     8  External abort pending bit */
        u32 _reserved1 : 23; /*!< bit:14..31  Reserved */
    } b;                          /*!< Structure used for bit  access */
    u32 w;                   /*!< Type      used for word access */
} ISR_Type;

#define ISR_A_Pos 13U                /*!< ISR: A Position */
#define ISR_A_Msk (1UL << ISR_A_Pos) /*!< ISR: A Mask */

#define ISR_I_Pos 12U                /*!< ISR: I Position */
#define ISR_I_Msk (1UL << ISR_I_Pos) /*!< ISR: I Mask */

#define ISR_F_Pos 11U                /*!< ISR: F Position */
#define ISR_F_Msk (1UL << ISR_F_Pos) /*!< ISR: F Mask */

/* Mask and shift a bit field value for use in a register bit range. */
#define _VAL2FLD(field, value) ((value << field##_Pos) & field##_Msk)

/* Mask and shift a register value to extract a bit filed value. */
#define _FLD2VAL(field, value) ((value & field##_Msk) >> field##_Pos)

/*******************************************************************************
 *       			CP15 访问函数
 ******************************************************************************/

FORCEDINLINE __STATIC_INLINE u32 __get_SCTLR(void) {
    return __MRC(15, 0, 1, 0, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_SCTLR(u32 sctlr) {
    __MCR(15, 0, sctlr, 1, 0, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_ACTLR(void) {
    return __MRC(15, 0, 1, 0, 1);
}

FORCEDINLINE __STATIC_INLINE void __set_ACTLR(u32 actlr) {
    __MCR(15, 0, actlr, 1, 0, 1);
}

FORCEDINLINE __STATIC_INLINE u32 __get_CPACR(void) {
    return __MRC(15, 0, 1, 0, 2);
}

FORCEDINLINE __STATIC_INLINE void __set_CPACR(u32 cpacr) {
    __MCR(15, 0, cpacr, 1, 0, 2);
}

FORCEDINLINE __STATIC_INLINE u32 __get_TTBR0(void) {
    return __MRC(15, 0, 2, 0, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_TTBR0(u32 ttbr0) {
    __MCR(15, 0, ttbr0, 2, 0, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_TTBR1(void) {
    return __MRC(15, 0, 2, 0, 1);
}

FORCEDINLINE __STATIC_INLINE void __set_TTBR1(u32 ttbr1) {
    __MCR(15, 0, ttbr1, 2, 0, 1);
}

FORCEDINLINE __STATIC_INLINE u32 __get_TTBCR(void) {
    return __MRC(15, 0, 2, 0, 2);
}

FORCEDINLINE __STATIC_INLINE void __set_TTBCR(u32 ttbcr) {
    __MCR(15, 0, ttbcr, 2, 0, 2);
}

FORCEDINLINE __STATIC_INLINE u32 __get_DACR(void) {
    return __MRC(15, 0, 3, 0, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_DACR(u32 dacr) {
    __MCR(15, 0, dacr, 3, 0, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_DFSR(void) {
    return __MRC(15, 0, 5, 0, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_DFSR(u32 dfsr) {
    __MCR(15, 0, dfsr, 5, 0, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_IFSR(void) {
    return __MRC(15, 0, 5, 0, 1);
}

FORCEDINLINE __STATIC_INLINE void __set_IFSR(u32 ifsr) {
    __MCR(15, 0, ifsr, 5, 0, 1);
}

FORCEDINLINE __STATIC_INLINE u32 __get_DFAR(void) {
    return __MRC(15, 0, 6, 0, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_DFAR(u32 dfar) {
    __MCR(15, 0, dfar, 6, 0, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_IFAR(void) {
    return __MRC(15, 0, 6, 0, 2);
}

FORCEDINLINE __STATIC_INLINE void __set_IFAR(u32 ifar) {
    __MCR(15, 0, ifar, 6, 0, 2);
}

FORCEDINLINE __STATIC_INLINE u32 __get_VBAR(void) {
    return __MRC(15, 0, 12, 0, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_VBAR(u32 vbar) {
    __MCR(15, 0, vbar, 12, 0, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_ISR(void) {
    return __MRC(15, 0, 12, 1, 0);
}

FORCEDINLINE __STATIC_INLINE void __set_ISR(u32 isr) {
    __MCR(15, 0, isr, 12, 1, 0);
}

FORCEDINLINE __STATIC_INLINE u32 __get_CONTEXTIDR(void) {
    return __MRC(15, 0, 13, 0, 1);
}

FORCEDINLINE __STATIC_INLINE void __set_CONTEXTIDR(u32 contextidr) {
    __MCR(15, 0, contextidr, 13, 0, 1);
}

FORCEDINLINE __STATIC_INLINE u32 __get_CBAR(void) {
    return __MRC(15, 4, 15, 0, 0);
}

/*******************************************************************************
 *                 GIC相关内容
 *有关GIC的内容，参考：ARM Generic Interrupt Controller(ARM GIC控制器)V2.0.pdf
 ******************************************************************************/

/*
 * GIC寄存器描述结构体，
 * GIC分为分发器端和CPU接口端
 */
typedef struct {
    u32 RESERVED0[1024];
    __IOM u32 D_CTLR; /*!< Offset: 0x1000 (R/W) Distributor Control Register */
    __IM u32 D_TYPER; /*!< Offset: 0x1004 (R/ )  Interrupt Controller Type Register */
    __IM u32 D_IIDR;  /*!< Offset: 0x1008 (R/ )  Distributor Implementer Identification Register */
    u32 RESERVED1[29];
    __IOM u32 D_IGROUPR[16]; /*!< Offset: 0x1080 - 0x0BC (R/W) Interrupt Group Registers */
    u32 RESERVED2[16];
    __IOM u32 D_ISENABLER[16]; /*!< Offset: 0x1100 - 0x13C (R/W) Interrupt Set-Enable Registers */
    u32 RESERVED3[16];
    __IOM u32 D_ICENABLER[16]; /*!< Offset: 0x1180 - 0x1BC (R/W) Interrupt Clear-Enable Registers */
    u32 RESERVED4[16];
    __IOM u32 D_ISPENDR[16]; /*!< Offset: 0x1200 - 0x23C (R/W) Interrupt Set-Pending Registers */
    u32 RESERVED5[16];
    __IOM u32 D_ICPENDR[16]; /*!< Offset: 0x1280 - 0x2BC (R/W) Interrupt Clear-Pending Registers */
    u32 RESERVED6[16];
    __IOM u32 D_ISACTIVER[16]; /*!< Offset: 0x1300 - 0x33C (R/W) Interrupt Set-Active Registers */
    u32 RESERVED7[16];
    __IOM u32 D_ICACTIVER[16]; /*!< Offset: 0x1380 - 0x3BC (R/W) Interrupt Clear-Active Registers */
    u32 RESERVED8[16];
    __IOM u8 D_IPRIORITYR[512]; /*!< Offset: 0x1400 - 0x5FC (R/W) Interrupt Priority Registers */
    u32 RESERVED9[128];
    __IOM u8 D_ITARGETSR[512]; /*!< Offset: 0x1800 - 0x9FC (R/W) Interrupt Targets Registers */
    u32 RESERVED10[128];
    __IOM u32 D_ICFGR[32]; /*!< Offset: 0x1C00 - 0xC7C (R/W) Interrupt configuration registers */
    u32 RESERVED11[32];
    __IM u32 D_PPISR;     /*!< Offset: 0x1D00 (R/ ) Private Peripheral Interrupt Status Register */
    __IM u32 D_SPISR[15]; /*!< Offset: 0x1D04 - 0xD3C (R/ ) Shared Peripheral Interrupt Status Registers */
    u32 RESERVED12[112];
    __OM u32 D_SGIR; /*!< Offset: 0x1F00 ( /W) Software Generated Interrupt Register */
    u32 RESERVED13[3];
    __IOM u8 D_CPENDSGIR[16]; /*!< Offset: 0x1F10 - 0xF1C (R/W) SGI Clear-Pending Registers */
    __IOM u8 D_SPENDSGIR[16]; /*!< Offset: 0x1F20 - 0xF2C (R/W) SGI Set-Pending Registers */
    u32 RESERVED14[40];
    __IM u32 D_PIDR4; /*!< Offset: 0x1FD0 (R/ ) Peripheral ID4 Register */
    __IM u32 D_PIDR5; /*!< Offset: 0x1FD4 (R/ ) Peripheral ID5 Register */
    __IM u32 D_PIDR6; /*!< Offset: 0x1FD8 (R/ ) Peripheral ID6 Register */
    __IM u32 D_PIDR7; /*!< Offset: 0x1FDC (R/ ) Peripheral ID7 Register */
    __IM u32 D_PIDR0; /*!< Offset: 0x1FE0 (R/ ) Peripheral ID0 Register */
    __IM u32 D_PIDR1; /*!< Offset: 0x1FE4 (R/ ) Peripheral ID1 Register */
    __IM u32 D_PIDR2; /*!< Offset: 0x1FE8 (R/ ) Peripheral ID2 Register */
    __IM u32 D_PIDR3; /*!< Offset: 0x1FEC (R/ ) Peripheral ID3 Register */
    __IM u32 D_CIDR0; /*!< Offset: 0x1FF0 (R/ ) Component ID0 Register */
    __IM u32 D_CIDR1; /*!< Offset: 0x1FF4 (R/ ) Component ID1 Register */
    __IM u32 D_CIDR2; /*!< Offset: 0x1FF8 (R/ ) Component ID2 Register */
    __IM u32 D_CIDR3; /*!< Offset: 0x1FFC (R/ ) Component ID3 Register */

    __IOM u32 C_CTLR;  /*!< Offset: 0x2000 (R/W) CPU Interface Control Register */
    __IOM u32 C_PMR;   /*!< Offset: 0x2004 (R/W) Interrupt Priority Mask Register */
    __IOM u32 C_BPR;   /*!< Offset: 0x2008 (R/W) Binary Point Register */
    __IM u32 C_IAR;    /*!< Offset: 0x200C (R/ ) Interrupt Acknowledge Register */
    __OM u32 C_EOIR;   /*!< Offset: 0x2010 ( /W) End Of Interrupt Register */
    __IM u32 C_RPR;    /*!< Offset: 0x2014 (R/ ) Running Priority Register */
    __IM u32 C_HPPIR;  /*!< Offset: 0x2018 (R/ ) Highest Priority Pending Interrupt Register */
    __IOM u32 C_ABPR;  /*!< Offset: 0x201C (R/W) Aliased Binary Point Register */
    __IM u32 C_AIAR;   /*!< Offset: 0x2020 (R/ ) Aliased Interrupt Acknowledge Register */
    __OM u32 C_AEOIR;  /*!< Offset: 0x2024 ( /W) Aliased End Of Interrupt Register */
    __IM u32 C_AHPPIR; /*!< Offset: 0x2028 (R/ ) Aliased Highest Priority Pending Interrupt Register */
    u32 RESERVED15[41];
    __IOM u32 C_APR0; /*!< Offset: 0x20D0 (R/W) Active Priority Register */
    u32 RESERVED16[3];
    __IOM u32 C_NSAPR0; /*!< Offset: 0x20E0 (R/W) Non-secure Active Priority Register */
    u32 RESERVED17[6];
    __IM u32 C_IIDR; /*!< Offset: 0x20FC (R/ ) CPU Interface Identification Register */
    u32 RESERVED18[960];
    __OM u32 C_DIR; /*!< Offset: 0x3000 ( /W) Deactivate Interrupt Register */
} GIC_Type;


struct gic_dist {
        u32 RESERVED0[1024];
    __IOM u32 D_CTLR; /*!< Offset: 0x1000 (R/W) Distributor Control Register */
    __IM u32 D_TYPER; /*!< Offset: 0x1004 (R/ )  Interrupt Controller Type Register */
    __IM u32 D_IIDR;  /*!< Offset: 0x1008 (R/ )  Distributor Implementer Identification Register */
    u32 RESERVED1[29];
    __IOM u32 D_IGROUPR[16]; /*!< Offset: 0x1080 - 0x0BC (R/W) Interrupt Group Registers */
    u32 RESERVED2[16];
    __IOM u32 D_ISENABLER[16]; /*!< Offset: 0x1100 - 0x13C (R/W) Interrupt Set-Enable Registers */
    u32 RESERVED3[16];
    __IOM u32 D_ICENABLER[16]; /*!< Offset: 0x1180 - 0x1BC (R/W) Interrupt Clear-Enable Registers */
    u32 RESERVED4[16];
    __IOM u32 D_ISPENDR[16]; /*!< Offset: 0x1200 - 0x23C (R/W) Interrupt Set-Pending Registers */
    u32 RESERVED5[16];
    __IOM u32 D_ICPENDR[16]; /*!< Offset: 0x1280 - 0x2BC (R/W) Interrupt Clear-Pending Registers */
    u32 RESERVED6[16];
    __IOM u32 D_ISACTIVER[16]; /*!< Offset: 0x1300 - 0x33C (R/W) Interrupt Set-Active Registers */
    u32 RESERVED7[16];
    __IOM u32 D_ICACTIVER[16]; /*!< Offset: 0x1380 - 0x3BC (R/W) Interrupt Clear-Active Registers */
    u32 RESERVED8[16];
    __IOM u8 D_IPRIORITYR[512]; /*!< Offset: 0x1400 - 0x5FC (R/W) Interrupt Priority Registers */
    u32 RESERVED9[128];
    __IOM u8 D_ITARGETSR[512]; /*!< Offset: 0x1800 - 0x9FC (R/W) Interrupt Targets Registers */
    u32 RESERVED10[128];
    __IOM u32 D_ICFGR[32]; /*!< Offset: 0x1C00 - 0xC7C (R/W) Interrupt configuration registers */
    u32 RESERVED11[32];
    __IM u32 D_PPISR;     /*!< Offset: 0x1D00 (R/ ) Private Peripheral Interrupt Status Register */
    __IM u32 D_SPISR[15]; /*!< Offset: 0x1D04 - 0xD3C (R/ ) Shared Peripheral Interrupt Status Registers */
    u32 RESERVED12[112];
    __OM u32 D_SGIR; /*!< Offset: 0x1F00 ( /W) Software Generated Interrupt Register */
    u32 RESERVED13[3];
    __IOM u8 D_CPENDSGIR[16]; /*!< Offset: 0x1F10 - 0xF1C (R/W) SGI Clear-Pending Registers */
    __IOM u8 D_SPENDSGIR[16]; /*!< Offset: 0x1F20 - 0xF2C (R/W) SGI Set-Pending Registers */
    u32 RESERVED14[40];
    __IM u32 D_PIDR4; /*!< Offset: 0x1FD0 (R/ ) Peripheral ID4 Register */
    __IM u32 D_PIDR5; /*!< Offset: 0x1FD4 (R/ ) Peripheral ID5 Register */
    __IM u32 D_PIDR6; /*!< Offset: 0x1FD8 (R/ ) Peripheral ID6 Register */
    __IM u32 D_PIDR7; /*!< Offset: 0x1FDC (R/ ) Peripheral ID7 Register */
    __IM u32 D_PIDR0; /*!< Offset: 0x1FE0 (R/ ) Peripheral ID0 Register */
    __IM u32 D_PIDR1; /*!< Offset: 0x1FE4 (R/ ) Peripheral ID1 Register */
    __IM u32 D_PIDR2; /*!< Offset: 0x1FE8 (R/ ) Peripheral ID2 Register */
    __IM u32 D_PIDR3; /*!< Offset: 0x1FEC (R/ ) Peripheral ID3 Register */
    __IM u32 D_CIDR0; /*!< Offset: 0x1FF0 (R/ ) Component ID0 Register */
    __IM u32 D_CIDR1; /*!< Offset: 0x1FF4 (R/ ) Component ID1 Register */
    __IM u32 D_CIDR2; /*!< Offset: 0x1FF8 (R/ ) Component ID2 Register */
    __IM u32 D_CIDR3; /*!< Offset: 0x1FFC (R/ ) Component ID3 Register */
};

struct gic_cpu {
    __IOM u32 C_CTLR;  /*!< Offset: 0x2000 (R/W) CPU Interface Control Register */
    __IOM u32 C_PMR;   /*!< Offset: 0x2004 (R/W) Interrupt Priority Mask Register */
    __IOM u32 C_BPR;   /*!< Offset: 0x2008 (R/W) Binary Point Register */
    __IM u32 C_IAR;    /*!< Offset: 0x200C (R/ ) Interrupt Acknowledge Register */
    __OM u32 C_EOIR;   /*!< Offset: 0x2010 ( /W) End Of Interrupt Register */
    __IM u32 C_RPR;    /*!< Offset: 0x2014 (R/ ) Running Priority Register */
    __IM u32 C_HPPIR;  /*!< Offset: 0x2018 (R/ ) Highest Priority Pending Interrupt Register */
    __IOM u32 C_ABPR;  /*!< Offset: 0x201C (R/W) Aliased Binary Point Register */
    __IM u32 C_AIAR;   /*!< Offset: 0x2020 (R/ ) Aliased Interrupt Acknowledge Register */
    __OM u32 C_AEOIR;  /*!< Offset: 0x2024 ( /W) Aliased End Of Interrupt Register */
    __IM u32 C_AHPPIR; /*!< Offset: 0x2028 (R/ ) Aliased Highest Priority Pending Interrupt Register */
    u32 RESERVED15[41];
    __IOM u32 C_APR0; /*!< Offset: 0x20D0 (R/W) Active Priority Register */
    u32 RESERVED16[3];
    __IOM u32 C_NSAPR0; /*!< Offset: 0x20E0 (R/W) Non-secure Active Priority Register */
    u32 RESERVED17[6];
    __IM u32 C_IIDR; /*!< Offset: 0x20FC (R/ ) CPU Interface Identification Register */
    u32 RESERVED18[960];
    __OM u32 C_DIR; /*!< Offset: 0x3000 ( /W) Deactivate Interrupt Register */
};



/*
 * GIC初始化
 * 为了简单使用GIC的group0
 */
FORCEDINLINE __STATIC_INLINE void GIC_Init(void *gicBase) {
    u32 i;
    u32 irqRegs;
    GIC_Type *gic = (GIC_Type *)gicBase;

    irqRegs = (gic->D_TYPER & 0x1FUL) + 1;

    /* On POR, all SPI is in group 0, level-sensitive and using 1-N model */

    /* Disable all PPI, SGI and SPI */
    for (i = 0; i < irqRegs; i++)
        gic->D_ICENABLER[i] = 0xFFFFFFFFUL;

    /* Make all interrupts have higher priority */
    gic->C_PMR = (0xFFUL << (8 - __GIC_PRIO_BITS)) & 0xFFUL;

    /* No subpriority, all priority level allows preemption */
    gic->C_BPR = 7 - __GIC_PRIO_BITS;

    /* Enable group0 distribution */
    gic->D_CTLR = 1UL;

    /* Enable group0 signaling */
    gic->C_CTLR = 1UL;
}

/*
 * 使能指定的中断
 */
FORCEDINLINE __STATIC_INLINE void GIC_EnableIRQ(IRQn_Type IRQn) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    gic->D_ISENABLER[((u32)(s32)IRQn) >> 5] = (u32)(1UL << (((u32)(s32)IRQn) & 0x1FUL));
}

/*
 * 关闭指定的中断
 */

FORCEDINLINE __STATIC_INLINE void GIC_DisableIRQ(IRQn_Type IRQn) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    gic->D_ICENABLER[((u32)(s32)IRQn) >> 5] = (u32)(1UL << (((u32)(s32)IRQn) & 0x1FUL));
}

/*
 * 返回中断号
 */
FORCEDINLINE __STATIC_INLINE u32 GIC_AcknowledgeIRQ(void) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    return gic->C_IAR & 0x1FFFUL;
}

/*
 * 向EOIR写入发送中断的中断号来释放中断
 */
FORCEDINLINE __STATIC_INLINE void GIC_DeactivateIRQ(u32 value) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    gic->C_EOIR = value;
}

/*
 * 获取运行优先级
 */
FORCEDINLINE __STATIC_INLINE u32 GIC_GetRunningPriority(void) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    return gic->C_RPR & 0xFFUL;
}

/*
 * 设置组优先级
 */
FORCEDINLINE __STATIC_INLINE void GIC_SetPriorityGrouping(u32 PriorityGroup) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    gic->C_BPR = PriorityGroup & 0x7UL;
}

/*
 * 获取组优先级
 */
FORCEDINLINE __STATIC_INLINE u32 GIC_GetPriorityGrouping(void) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);

    return gic->C_BPR & 0x7UL;
}

/*
 * 设置优先级
 */
FORCEDINLINE __STATIC_INLINE void GIC_SetPriority(IRQn_Type IRQn, u32 priority) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    gic->D_IPRIORITYR[((u32)(s32)IRQn)] = (u8)((priority << (8UL - __GIC_PRIO_BITS)) & (u32)0xFFUL);
}

/*
 * 获取优先级
 */
FORCEDINLINE __STATIC_INLINE u32 GIC_GetPriority(IRQn_Type IRQn) {
    GIC_Type *gic = (GIC_Type *)(__get_CBAR() & 0xFFFF0000UL);
    return (((u32)gic->D_IPRIORITYR[((u32)(s32)IRQn)] >> (8UL - __GIC_PRIO_BITS)));
}

#endif
