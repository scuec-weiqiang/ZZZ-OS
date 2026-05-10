#include <fs/blkdev.h>
#include <os/bitops.h>
#include <os/bswap.h>
#include <os/errno.h>
#include <os/err.h>
#include <os/gpio/consumer.h>
#include <os/kmalloc.h>
#include <os/mm.h>
#include <mm/page.h>
#include <os/module.h>
#include <os/of.h>
#include <os/io.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/spinlock.h>
#include <os/string.h>

#define USDHC_SECTOR_SIZE          512U

#define IMX6ULL_CCM_BASE           0x020c4000U
#define IMX6ULL_CCM_SIZE           0x1000U
#define IMX6ULL_CCM_CCGR6          0x80
#define IMX6ULL_CCM_CSCMR1         0x1c
#define IMX6ULL_CCM_CSCDR1         0x24

#define CCM_CCGR6_USDHC1_MASK      GENMASK(3, 2)
#define CCM_CSCMR1_USDHC1_CLK_SEL  BIT(16)
#define CCM_CSCDR1_USDHC1_PODF     GENMASK(13, 11)

#define USDHC_DS_ADDR              0x00
#define USDHC_BLK_ATT              0x04
#define USDHC_CMD_ARG              0x08
#define USDHC_CMD_XFR_TYP          0x0c
#define USDHC_CMD_RSP0             0x10
#define USDHC_CMD_RSP1             0x14
#define USDHC_CMD_RSP2             0x18
#define USDHC_CMD_RSP3             0x1c
#define USDHC_DATA_BUFF_ACC_PORT   0x20
#define USDHC_PRSSTAT              0x24
#define USDHC_PROT_CTRL            0x28
#define USDHC_SYS_CTRL             0x2c
#define USDHC_INT_STATUS           0x30
#define USDHC_INT_STATUS_EN        0x34
#define USDHC_INT_SIGNAL_EN        0x38
#define USDHC_WTMK_LVL             0x44
#define USDHC_MIX_CTRL             0x48

#define USDHC_PRSSTAT_CIHB         BIT(0)
#define USDHC_PRSSTAT_CDIHB        BIT(1)
#define USDHC_PRSSTAT_DLA          BIT(2)
#define USDHC_PRSSTAT_SDSTB        BIT(3)

#define USDHC_PROT_CTRL_DTW_MASK   GENMASK(2, 1)
#define USDHC_PROT_CTRL_DTW_1BIT   0U
#define USDHC_PROT_CTRL_DTW_4BIT   BIT(1)

#define USDHC_SYS_CTRL_IPGEN       BIT(0)
#define USDHC_SYS_CTRL_HCKEN       BIT(1)
#define USDHC_SYS_CTRL_PEREN       BIT(2)
#define USDHC_SYS_CTRL_SDCLKEN     BIT(3)
#define USDHC_SYS_CTRL_DVS_MASK    GENMASK(7, 4)
#define USDHC_SYS_CTRL_SDCLKFS_MASK GENMASK(15, 8)
#define USDHC_SYS_CTRL_DTOCV_MASK  GENMASK(19, 16)
#define USDHC_SYS_CTRL_RSTA        BIT(24)
#define USDHC_SYS_CTRL_INITA       BIT(27)

#define USDHC_INT_CC               BIT(0)
#define USDHC_INT_TC               BIT(1)
#define USDHC_INT_BWR              BIT(4)
#define USDHC_INT_BRR              BIT(5)
#define USDHC_INT_CTOE             BIT(16)
#define USDHC_INT_CCE              BIT(17)
#define USDHC_INT_CEBE             BIT(18)
#define USDHC_INT_CIE              BIT(19)
#define USDHC_INT_DTOE             BIT(20)
#define USDHC_INT_DCE              BIT(21)
#define USDHC_INT_DEBE             BIT(22)
#define USDHC_INT_AC12E            BIT(24)
#define USDHC_INT_DMAE             BIT(28)
#define USDHC_INT_ERROR_MASK       (USDHC_INT_CTOE | USDHC_INT_CCE | \
                                    USDHC_INT_CEBE | USDHC_INT_CIE | \
                                    USDHC_INT_DTOE | USDHC_INT_DCE | \
                                    USDHC_INT_DEBE | USDHC_INT_AC12E | \
                                    USDHC_INT_DMAE)

#define USDHC_WTMK_RD_WML_MASK     GENMASK(7, 0)
#define USDHC_WTMK_WR_WML_MASK     GENMASK(23, 16)

#define USDHC_MIX_CTRL_DMAEN       BIT(0)
#define USDHC_MIX_CTRL_BCEN        BIT(1)
#define USDHC_MIX_CTRL_AC12EN      BIT(2)
#define USDHC_MIX_CTRL_DTDSEL      BIT(4)
#define USDHC_MIX_CTRL_MSBSEL      BIT(5)

#define USDHC_CMD_RSPTYP_NONE      (0U << 16)
#define USDHC_CMD_RSPTYP_136       (1U << 16)
#define USDHC_CMD_RSPTYP_48        (2U << 16)
#define USDHC_CMD_RSPTYP_48_BUSY   (3U << 16)
#define USDHC_CMD_CCCEN            BIT(19)
#define USDHC_CMD_CICEN            BIT(20)
#define USDHC_CMD_DPSEL            BIT(21)
#define USDHC_CMD_CMDINX_SHIFT     24

#define SD_CMD_GO_IDLE_STATE       0
#define SD_CMD_ALL_SEND_CID        2
#define SD_CMD_SEND_RELATIVE_ADDR  3
#define SD_CMD_SELECT_CARD         7
#define SD_CMD_SEND_IF_COND        8
#define SD_CMD_SEND_CSD            9
#define SD_CMD_SET_BLOCKLEN        16
#define SD_CMD_READ_SINGLE_BLOCK   17
#define SD_CMD_WRITE_SINGLE_BLOCK  24
#define SD_CMD_APP_CMD             55
#define SD_ACMD_SET_BUS_WIDTH      6
#define SD_ACMD_SD_SEND_OP_COND    41

#define SD_OCR_BUSY                BIT(31)
#define SD_OCR_HCS                 BIT(30)
#define SD_OCR_VOLTAGE_WINDOW      0x00ff8000U

#define USDHC_PIO_WORDS            (USDHC_SECTOR_SIZE / sizeof(u32))
#define USDHC_MAX_POLLS            1000000
#define USDHC_ACMD41_RETRIES       1000
#define USDHC_INIT_CLOCK_HZ        400000U
#define USDHC_TRANSFER_CLOCK_HZ    25000000U

enum usdhc_resp_type {
    USDHC_RESP_NONE = 0,
    USDHC_RESP_R1,
    USDHC_RESP_R1B,
    USDHC_RESP_R2,
    USDHC_RESP_R3,
    USDHC_RESP_R6,
    USDHC_RESP_R7,
};

struct usdhc_cmd {
    u32 opcode;
    u32 arg;
    enum usdhc_resp_type resp_type;
    u32 resp[4];
};

struct usdhc_data {
    void *buf;
    u32 blksz;
    u32 blocks;
    bool write;
};

struct imx6ull_usdhc {
    spinlock_t lock;
    void *base;
    void *ccm_base;
    struct gpio_desc *cd_gpio;
    struct request_queue queue;
    struct gendisk disk;
    u32 src_clk_hz;
    sector_t capacity;
    u16 rca;
    u8 bus_width;
    bool high_capacity;
};

static struct block_device_operations imx6ull_usdhc_bdops = {
    .submit_bio = NULL,
};

static inline u32 usdhc_readl(struct imx6ull_usdhc *host, u32 reg)
{
    return readl(host->base + reg);
}

static inline void usdhc_writel(struct imx6ull_usdhc *host, u32 reg, u32 val)
{
    writel(val, host->base + reg);
}

static int usdhc_wait_reg(struct imx6ull_usdhc *host, u32 reg, u32 mask, u32 expect)
{
    int timeout = USDHC_MAX_POLLS;

    while (timeout-- > 0) {
        if ((usdhc_readl(host, reg) & mask) == expect) {
            return 0;
        }
    }

    return -ETIMEDOUT;
}

static int usdhc_wait_irq(struct imx6ull_usdhc *host, u32 mask)
{
    int timeout = USDHC_MAX_POLLS;

    while (timeout-- > 0) {
        u32 status = usdhc_readl(host, USDHC_INT_STATUS);

        if (status & USDHC_INT_ERROR_MASK) {
            usdhc_writel(host, USDHC_INT_STATUS, status & USDHC_INT_ERROR_MASK);
            if (status & (USDHC_INT_CTOE | USDHC_INT_DTOE)) {
                return -ETIMEDOUT;
            }
            return -EIO;
        }

        if (status & mask) {
            usdhc_writel(host, USDHC_INT_STATUS, status & mask);
            return 0;
        }
    }

    return -ETIMEDOUT;
}

static void usdhc_enable_root_clock(struct imx6ull_usdhc *host)
{
    u32 val = readl(host->ccm_base + IMX6ULL_CCM_CCGR6);

    val &= ~CCM_CCGR6_USDHC1_MASK;
    val |= FIELD_PREP(CCM_CCGR6_USDHC1_MASK, 0x3);
    writel(val, host->ccm_base + IMX6ULL_CCM_CCGR6);
}

static u32 usdhc_get_src_clock(struct imx6ull_usdhc *host)
{
    u32 cscmr1 = readl(host->ccm_base + IMX6ULL_CCM_CSCMR1);
    u32 cscdr1 = readl(host->ccm_base + IMX6ULL_CCM_CSCDR1);
    u32 parent_hz = (cscmr1 & CCM_CSCMR1_USDHC1_CLK_SEL) ? 352000000U : 396000000U;
    u32 podf = FIELD_GET(CCM_CSCDR1_USDHC1_PODF, cscdr1) + 1U;

    return parent_hz / podf;
}

static int usdhc_hw_reset(struct imx6ull_usdhc *host)
{
    u32 val = usdhc_readl(host, USDHC_SYS_CTRL);

    val |= USDHC_SYS_CTRL_RSTA;
    usdhc_writel(host, USDHC_SYS_CTRL, val);

    return usdhc_wait_reg(host, USDHC_SYS_CTRL, USDHC_SYS_CTRL_RSTA, 0);
}

static int usdhc_set_clock(struct imx6ull_usdhc *host, u32 target_hz)
{
    u32 pre_div = 1;
    u32 div = 1;
    u32 actual_hz;
    u32 sys;

    if (!host->src_clk_hz || !target_hz) {
        return -EINVAL;
    }

    while (pre_div < 256 && (host->src_clk_hz / (pre_div * 16U)) > target_hz) {
        pre_div <<= 1;
    }

    while (div < 16 && (host->src_clk_hz / (pre_div * div)) > target_hz) {
        div++;
    }

    actual_hz = host->src_clk_hz / (pre_div * div);
    sys = usdhc_readl(host, USDHC_SYS_CTRL);
    sys &= ~(USDHC_SYS_CTRL_SDCLKEN | USDHC_SYS_CTRL_DVS_MASK |
             USDHC_SYS_CTRL_SDCLKFS_MASK | USDHC_SYS_CTRL_DTOCV_MASK);
    sys |= USDHC_SYS_CTRL_IPGEN | USDHC_SYS_CTRL_HCKEN | USDHC_SYS_CTRL_PEREN;
    sys |= FIELD_PREP(USDHC_SYS_CTRL_DVS_MASK, div - 1U);
    sys |= FIELD_PREP(USDHC_SYS_CTRL_SDCLKFS_MASK, pre_div >> 1);
    sys |= FIELD_PREP(USDHC_SYS_CTRL_DTOCV_MASK, 0x0e);
    usdhc_writel(host, USDHC_SYS_CTRL, sys);

    sys |= USDHC_SYS_CTRL_SDCLKEN;
    usdhc_writel(host, USDHC_SYS_CTRL, sys);

    if (usdhc_wait_reg(host, USDHC_PRSSTAT, USDHC_PRSSTAT_SDSTB, USDHC_PRSSTAT_SDSTB) < 0) {
        return -ETIMEDOUT;
    }

    printk("imx6ull-usdhc: card clock set to %u Hz (target %u Hz)\n",
           actual_hz, target_hz);
    return 0;
}

static int usdhc_send_init_clocks(struct imx6ull_usdhc *host)
{
    u32 sys = usdhc_readl(host, USDHC_SYS_CTRL);

    sys |= USDHC_SYS_CTRL_INITA;
    usdhc_writel(host, USDHC_SYS_CTRL, sys);

    return usdhc_wait_reg(host, USDHC_SYS_CTRL, USDHC_SYS_CTRL_INITA, 0);
}

static void usdhc_read_response(struct imx6ull_usdhc *host, struct usdhc_cmd *cmd)
{
    memset(cmd->resp, 0, sizeof(cmd->resp));

    if (cmd->resp_type == USDHC_RESP_R2) {
        u32 rsp0 = usdhc_readl(host, USDHC_CMD_RSP0);
        u32 rsp1 = usdhc_readl(host, USDHC_CMD_RSP1);
        u32 rsp2 = usdhc_readl(host, USDHC_CMD_RSP2);
        u32 rsp3 = usdhc_readl(host, USDHC_CMD_RSP3);

        cmd->resp[0] = (rsp3 << 8) | (rsp2 >> 24);
        cmd->resp[1] = (rsp2 << 8) | (rsp1 >> 24);
        cmd->resp[2] = (rsp1 << 8) | (rsp0 >> 24);
        cmd->resp[3] = (rsp0 << 8);
        return;
    }

    if (cmd->resp_type != USDHC_RESP_NONE) {
        cmd->resp[0] = usdhc_readl(host, USDHC_CMD_RSP0);
    }
}

static int usdhc_wait_ready(struct imx6ull_usdhc *host, bool data_present)
{
    u32 mask = USDHC_PRSSTAT_CIHB;

    if (data_present) {
        mask |= USDHC_PRSSTAT_CDIHB;
    }

    return usdhc_wait_reg(host, USDHC_PRSSTAT, mask, 0);
}

static int usdhc_send_cmd(struct imx6ull_usdhc *host, struct usdhc_cmd *cmd,
                          struct usdhc_data *data)
{
    u32 xfertyp = cmd->opcode << USDHC_CMD_CMDINX_SHIFT;
    u32 mix = 0;
    int ret;

    ret = usdhc_wait_ready(host, data != NULL);
    if (ret < 0) {
        return ret;
    }

    switch (cmd->resp_type) {
    case USDHC_RESP_NONE:
        xfertyp |= USDHC_CMD_RSPTYP_NONE;
        break;
    case USDHC_RESP_R1:
    case USDHC_RESP_R6:
    case USDHC_RESP_R7:
        xfertyp |= USDHC_CMD_RSPTYP_48 | USDHC_CMD_CCCEN | USDHC_CMD_CICEN;
        break;
    case USDHC_RESP_R1B:
        xfertyp |= USDHC_CMD_RSPTYP_48_BUSY | USDHC_CMD_CCCEN | USDHC_CMD_CICEN;
        break;
    case USDHC_RESP_R2:
        xfertyp |= USDHC_CMD_RSPTYP_136 | USDHC_CMD_CCCEN;
        break;
    case USDHC_RESP_R3:
        xfertyp |= USDHC_CMD_RSPTYP_48;
        break;
    default:
        return -EINVAL;
    }

    if (data) {
        u32 blk_att = (data->blocks << 16) | data->blksz;

        xfertyp |= USDHC_CMD_DPSEL;
        if (!data->write) {
            mix |= USDHC_MIX_CTRL_DTDSEL;
        }
        if (data->blocks > 1) {
            mix |= USDHC_MIX_CTRL_BCEN | USDHC_MIX_CTRL_MSBSEL;
        }

        usdhc_writel(host, USDHC_BLK_ATT, blk_att);
    }

    usdhc_writel(host, USDHC_MIX_CTRL, mix);
    usdhc_writel(host, USDHC_INT_STATUS, UINT32_MAX);
    usdhc_writel(host, USDHC_CMD_ARG, cmd->arg);
    usdhc_writel(host, USDHC_CMD_XFR_TYP, xfertyp);

    ret = usdhc_wait_irq(host, USDHC_INT_CC);
    if (ret < 0) {
        return ret;
    }

    usdhc_read_response(host, cmd);

    if (cmd->resp_type == USDHC_RESP_R1B) {
        ret = usdhc_wait_reg(host, USDHC_PRSSTAT,
                             USDHC_PRSSTAT_CDIHB | USDHC_PRSSTAT_DLA, 0);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

static int usdhc_pio_read_block(struct imx6ull_usdhc *host, void *buf)
{
    u32 *dst = buf;
    int ret;
    int i;

    ret = usdhc_wait_irq(host, USDHC_INT_BRR);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < USDHC_PIO_WORDS; i++) {
        dst[i] = usdhc_readl(host, USDHC_DATA_BUFF_ACC_PORT);
    }

    return usdhc_wait_irq(host, USDHC_INT_TC);
}

static int usdhc_pio_write_block(struct imx6ull_usdhc *host, const void *buf)
{
    const u32 *src = buf;
    int ret;
    int i;

    ret = usdhc_wait_irq(host, USDHC_INT_BWR);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < USDHC_PIO_WORDS; i++) {
        usdhc_writel(host, USDHC_DATA_BUFF_ACC_PORT, src[i]);
    }

    return usdhc_wait_irq(host, USDHC_INT_TC);
}

static u32 sd_resp_get_bits(const u32 *resp, int start, int size)
{
    int off = 3 - (start / 32);
    int shift = start & 31;
    u32 mask;
    u32 value;

    if (size == 32) {
        mask = UINT32_MAX;
    } else {
        mask = (1U << size) - 1U;
    }

    value = resp[off] >> shift;
    if (size + shift > 32) {
        value |= resp[off - 1] << (32 - shift);
    }

    return value & mask;
}

static sector_t sd_decode_capacity(const u32 *csd)
{
    u32 csd_structure = sd_resp_get_bits(csd, 126, 2);

    if (csd_structure == 1) {
        u32 c_size = sd_resp_get_bits(csd, 48, 22);
        return (sector_t)(c_size + 1U) * 1024U;
    }

    if (csd_structure == 0) {
        u32 read_bl_len = sd_resp_get_bits(csd, 80, 4);
        u32 c_size = sd_resp_get_bits(csd, 62, 12);
        u32 c_size_mult = sd_resp_get_bits(csd, 47, 3);
        u32 block_len = 1U << read_bl_len;
        u32 blocknr = (c_size + 1U) << (c_size_mult + 2U);
        u64 capacity_bytes = (u64)blocknr * block_len;

        return (sector_t)(capacity_bytes / USDHC_SECTOR_SIZE);
    }

    return 0;
}

static int sd_send_app_cmd(struct imx6ull_usdhc *host)
{
    struct usdhc_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_APP_CMD;
    cmd.arg = (u32)host->rca << 16;
    cmd.resp_type = USDHC_RESP_R1;

    return usdhc_send_cmd(host, &cmd, NULL);
}

static int sd_send_acmd(struct imx6ull_usdhc *host, u32 opcode, u32 arg,
                        enum usdhc_resp_type resp_type, struct usdhc_cmd *cmd)
{
    int ret = sd_send_app_cmd(host);

    if (ret < 0) {
        return ret;
    }

    memset(cmd, 0, sizeof(*cmd));
    cmd->opcode = opcode;
    cmd->arg = arg;
    cmd->resp_type = resp_type;

    return usdhc_send_cmd(host, cmd, NULL);
}

static int sd_set_bus_width(struct imx6ull_usdhc *host, u8 width)
{
    struct usdhc_cmd cmd;
    u32 prot_ctrl;
    int ret;

    if (width == 1) {
        prot_ctrl = usdhc_readl(host, USDHC_PROT_CTRL);
        prot_ctrl &= ~USDHC_PROT_CTRL_DTW_MASK;
        usdhc_writel(host, USDHC_PROT_CTRL, prot_ctrl);
        host->bus_width = 1;
        return 0;
    }

    if (width != 4) {
        return -EINVAL;
    }

    ret = sd_send_acmd(host, SD_ACMD_SET_BUS_WIDTH, 2, USDHC_RESP_R1, &cmd);
    if (ret < 0) {
        return ret;
    }

    prot_ctrl = usdhc_readl(host, USDHC_PROT_CTRL);
    prot_ctrl &= ~USDHC_PROT_CTRL_DTW_MASK;
    prot_ctrl |= USDHC_PROT_CTRL_DTW_4BIT;
    usdhc_writel(host, USDHC_PROT_CTRL, prot_ctrl);
    host->bus_width = 4;

    return 0;
}

static int sd_card_init(struct imx6ull_usdhc *host)
{
    struct usdhc_cmd cmd;
    u32 ocr_arg = SD_OCR_VOLTAGE_WINDOW | SD_OCR_HCS;
    bool sd_v2 = true;
    int ret;
    int retry;

    ret = usdhc_send_init_clocks(host);
    if (ret < 0) {
        return ret;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.resp_type = USDHC_RESP_NONE;
    ret = usdhc_send_cmd(host, &cmd, NULL);
    if (ret < 0) {
        return ret;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_SEND_IF_COND;
    cmd.arg = 0x1aa;
    cmd.resp_type = USDHC_RESP_R7;
    ret = usdhc_send_cmd(host, &cmd, NULL);
    if (ret < 0) {
        sd_v2 = false;
        ocr_arg &= ~SD_OCR_HCS;
    } else if ((cmd.resp[0] & 0xfffU) != 0x1aaU) {
        return -EIO;
    }

    host->rca = 0;
    for (retry = 0; retry < USDHC_ACMD41_RETRIES; retry++) {
        ret = sd_send_acmd(host, SD_ACMD_SD_SEND_OP_COND, ocr_arg, USDHC_RESP_R3, &cmd);
        if (ret < 0) {
            return ret;
        }
        if (cmd.resp[0] & SD_OCR_BUSY) {
            host->high_capacity = (cmd.resp[0] & SD_OCR_HCS) != 0;
            break;
        }
    }

    if (retry == USDHC_ACMD41_RETRIES) {
        return -ETIMEDOUT;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_ALL_SEND_CID;
    cmd.arg = 0;
    cmd.resp_type = USDHC_RESP_R2;
    ret = usdhc_send_cmd(host, &cmd, NULL);
    if (ret < 0) {
        return ret;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_SEND_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.resp_type = USDHC_RESP_R6;
    ret = usdhc_send_cmd(host, &cmd, NULL);
    if (ret < 0) {
        return ret;
    }
    host->rca = (u16)(cmd.resp[0] >> 16);

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_SEND_CSD;
    cmd.arg = (u32)host->rca << 16;
    cmd.resp_type = USDHC_RESP_R2;
    ret = usdhc_send_cmd(host, &cmd, NULL);
    if (ret < 0) {
        return ret;
    }

    host->capacity = sd_decode_capacity(cmd.resp);
    if (!host->capacity) {
        return -EIO;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = SD_CMD_SELECT_CARD;
    cmd.arg = (u32)host->rca << 16;
    cmd.resp_type = USDHC_RESP_R1B;
    ret = usdhc_send_cmd(host, &cmd, NULL);
    if (ret < 0) {
        return ret;
    }

    if (!host->high_capacity) {
        memset(&cmd, 0, sizeof(cmd));
        cmd.opcode = SD_CMD_SET_BLOCKLEN;
        cmd.arg = USDHC_SECTOR_SIZE;
        cmd.resp_type = USDHC_RESP_R1;
        ret = usdhc_send_cmd(host, &cmd, NULL);
        if (ret < 0) {
            return ret;
        }
    }

    ret = sd_set_bus_width(host, host->bus_width);
    if (ret < 0 && host->bus_width != 1) {
        return ret;
    }

    printk("imx6ull-usdhc: SD card ready (%s, %u sectors, RCA=0x%x, spec=%s)\n",
           host->high_capacity ? "SDHC/SDXC" : "SDSC",
           host->capacity, host->rca, sd_v2 ? "v2+" : "legacy");
    return 0;
}

static int imx6ull_usdhc_rw_block(struct imx6ull_usdhc *host, sector_t sector, void *buf,
                                  bool write)
{
    struct usdhc_cmd cmd;
    struct usdhc_data data;
    u32 addr = host->high_capacity ? sector : sector * USDHC_SECTOR_SIZE;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    memset(&data, 0, sizeof(data));

    cmd.opcode = write ? SD_CMD_WRITE_SINGLE_BLOCK : SD_CMD_READ_SINGLE_BLOCK;
    cmd.arg = addr;
    cmd.resp_type = USDHC_RESP_R1;

    data.buf = buf;
    data.blksz = USDHC_SECTOR_SIZE;
    data.blocks = 1;
    data.write = write;

    ret = usdhc_send_cmd(host, &cmd, &data);
    if (ret < 0) {
        return ret;
    }

    if (write) {
        return usdhc_pio_write_block(host, buf);
    }

    return usdhc_pio_read_block(host, buf);
}

static bool imx6ull_usdhc_card_present(struct imx6ull_usdhc *host)
{
    if (!host->cd_gpio) {
        return true;
    }

    return gpiod_get_value(host->cd_gpio) != 0;
}

static int imx6ull_usdhc_submit_bio(struct blkdev *bdev, struct bio *bio)
{
    struct imx6ull_usdhc *host;
    sector_t sector;
    int ret = 0;

    if (!bdev || !bio || !bdev->bd_disk) {
        return -EINVAL;
    }

    host = bdev->bd_disk->private_data;
    if (!host) {
        return -ENODEV;
    }

    if (!imx6ull_usdhc_card_present(host)) {
        return -ENOMEDIUM;
    }

    sector = bdev_sector_offset(bdev, bio->bi_sector);
    spin_lock(&host->lock);

    for (int i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *bvec = &bio->bi_io_vec[i];
        u8 *base;
        u32 done = 0;

        if (!bvec->page) {
            ret = -EINVAL;
            break;
        }

        if (bvec->len == 0) {
            continue;
        }

        base = (u8 *)page_address(bvec->page) + bvec->offset;
        while (done < bvec->len) {
            ret = imx6ull_usdhc_rw_block(host, sector, base + done,
                                         bio->op == REQ_OP_WRITE);
            if (ret < 0) {
                break;
            }
            done += USDHC_SECTOR_SIZE;
            sector++;
        }

        if (ret < 0) {
            break;
        }
    }

    spin_unlock(&host->lock);
    return ret;
}

static int imx6ull_usdhc_register_disk(struct imx6ull_usdhc *host)
{
    dev_t devnr;
    int ret;

    memset(&host->queue, 0, sizeof(host->queue));
    spin_lock_init(&host->queue.lock);
    host->queue.fops = &imx6ull_usdhc_bdops;
    host->queue.logical_block_size = USDHC_SECTOR_SIZE;
    host->queue.max_hw_sectors = 1;
    host->queue.queuedata = host;

    memset(&host->disk, 0, sizeof(host->disk));
    strcpy(host->disk.disk_name, "usdhc1");
    host->disk.capacity = host->capacity;
    host->disk.logical_block_size = USDHC_SECTOR_SIZE;
    host->disk.queue = &host->queue;
    host->disk.private_data = host;

    ret = alloc_blkdev_region(&devnr, 1);
    if (ret < 0) {
        return ret;
    }

    return blkdev_register("usdhc1", devnr, &host->disk, NULL);
}

static int imx6ull_usdhc_parse_dt(struct platform_device *pdev, struct imx6ull_usdhc *host)
{
    u32 *bus_width = NULL;

    host->bus_width = 1;
    bus_width = of_read_u32_array(pdev->dev.of_node, "bus-width", 1);
    if (bus_width) {
        if (bus_width[0] == 1 || bus_width[0] == 4) {
            host->bus_width = (u8)bus_width[0];
        }
        kfree(bus_width);
    }

    host->cd_gpio = gpiod_get(&pdev->dev, "cd", GPIOD_IN);
    if (IS_ERR(host->cd_gpio)) {
        host->cd_gpio = NULL;
    }

    return 0;
}

static int imx6ull_usdhc_probe(struct platform_device *pdev)
{
    struct imx6ull_usdhc *host;
    int ret;

    host = kzalloc(sizeof(*host));
    if (!host) {
        return -ENOMEM;
    }

    host->base = (void *)platform_ioremap_resource(pdev, 0);
    if (!host->base) {
        ret = -ENODEV;
        goto err_free_host;
    }

    host->ccm_base = ioremap(IMX6ULL_CCM_BASE, IMX6ULL_CCM_SIZE);
    if (!host->ccm_base) {
        ret = -ENOMEM;
        goto err_unmap_base;
    }

    spin_lock_init(&host->lock);
    platform_set_drvdata(pdev, host);

    ret = imx6ull_usdhc_parse_dt(pdev, host);
    if (ret < 0) {
        goto err_unmap_ccm;
    }

    if (!imx6ull_usdhc_card_present(host)) {
        ret = -ENOMEDIUM;
        goto err_put_cd;
    }

    usdhc_enable_root_clock(host);
    host->src_clk_hz = usdhc_get_src_clock(host);

    ret = usdhc_hw_reset(host);
    if (ret < 0) {
        goto err_put_cd;
    }

    usdhc_writel(host, USDHC_INT_STATUS_EN, UINT32_MAX);
    usdhc_writel(host, USDHC_INT_SIGNAL_EN, 0);
    usdhc_writel(host, USDHC_WTMK_LVL,
                 FIELD_PREP(USDHC_WTMK_RD_WML_MASK, USDHC_PIO_WORDS) |
                 FIELD_PREP(USDHC_WTMK_WR_WML_MASK, USDHC_PIO_WORDS));
    usdhc_writel(host, USDHC_DS_ADDR, 0);

    ret = usdhc_set_clock(host, USDHC_INIT_CLOCK_HZ);
    if (ret < 0) {
        goto err_put_cd;
    }

    ret = sd_card_init(host);
    if (ret < 0) {
        goto err_put_cd;
    }

    ret = usdhc_set_clock(host, USDHC_TRANSFER_CLOCK_HZ);
    if (ret < 0) {
        goto err_put_cd;
    }

    ret = imx6ull_usdhc_register_disk(host);
    if (ret < 0) {
        goto err_put_cd;
    }

    printk("imx6ull-usdhc: registered /dev/usdhc1 (%u sectors)\n", host->capacity);
    return 0;

err_put_cd:
    if (host->cd_gpio) {
        gpiod_put(host->cd_gpio);
        host->cd_gpio = NULL;
    }
err_unmap_ccm:
    iounmap((virt_addr_t)host->ccm_base, IMX6ULL_CCM_SIZE);
err_unmap_base:
    iounmap((virt_addr_t)host->base, 0x4000);
err_free_host:
    kfree(host);
    return ret;
}

static int imx6ull_usdhc_remove(struct platform_device *pdev)
{
    struct imx6ull_usdhc *host = platform_get_drvdata(pdev);

    if (!host) {
        return 0;
    }

    if (host->cd_gpio) {
        gpiod_put(host->cd_gpio);
    }
    if (host->ccm_base) {
        iounmap((virt_addr_t)host->ccm_base, IMX6ULL_CCM_SIZE);
    }
    if (host->base) {
        iounmap((virt_addr_t)host->base, 0x4000);
    }
    kfree(host);
    return 0;
}

static const struct of_device_id imx6ull_usdhc_dt_ids[] = {
    { .compatible = "fsl,imx6ul-usdhc", },
    { .compatible = "fsl,imx6sx-usdhc", },
    {},
};

static struct platform_driver imx6ull_usdhc_driver = {
    .probe = imx6ull_usdhc_probe,
    .remove = imx6ull_usdhc_remove,
    .driver = {
        .name = "imx6ull-usdhc",
        .of_match_table = imx6ull_usdhc_dt_ids,
    },
};

static int imx6ull_usdhc_driver_init(void)
{
    imx6ull_usdhc_bdops.submit_bio = imx6ull_usdhc_submit_bio;
    return platform_driver_register(&imx6ull_usdhc_driver);
}

device_initcall(imx6ull_usdhc_driver_init);
