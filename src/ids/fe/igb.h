/*_
 * Copyright (c) 2016-2017 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _IGB_H
#define _IGB_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <mki/driver.h>
#include "pci.h"
#include "common.h"

#define IGB_I219LM      0x156f
#define IGB_I211        0x1539

#define IGB_MMIO_SIZE   0x13000


#define IGB_REG_CTRL        0x0000
#define IGB_REG_STATUS      0x0004
#define IGB_REG_MDIC        0x0020

#define IGB_REG_RCTL        0x0100

#define IGB_REG_TCTL        0x0400
#define IGB_REG_TCTL_EXT    0x0404
#define IGB_REG_TIPG        0x0410
#define IGB_REG_RETX_CTL    0x041c


#define IGB_REG_EICS        0x1520
#define IGB_REG_EIMS        0x1524
#define IGB_REG_EIMC        0x1528
#define IGB_REG_EICR        0x1580

#define IGB_REG_RXPBSIZE    0x2404
#define IGB_REG_TXPBSIZE    0x3404
#define IGB_REG_DTXMXSZSQ   0x3540
#define IGB_REG_DTXMXPKTSZ  0x355c
#define IGB_REG_DTXCTL      0x3590
#define IGB_REG_DTXTCPFLGL  0x359c
#define IGB_REG_DTXTCPFLGH  0x35a0
#define IGB_REG_DTXBCTL     0x35a4

#define IGB_REG_MTA(n)      (0x5200 + 4 * (n))

#define IGB_REG_MRQC        0x5818

#define IGB_REG_SWSM        0x5b50

#define IGB_REG_RDBAL(n)    (0xc000 + 0x40 * (n))
#define IGB_REG_RDBAH(n)    (0xc004 + 0x40 * (n))
#define IGB_REG_RDLEN(n)    (0xc008 + 0x40 * (n))
#define IGB_REG_SRRCTL(n)   (0xc00c + 0x40 * (n))
#define IGB_REG_RDH(n)      (0xc010 + 0x40 * (n))
#define IGB_REG_RDT(n)      (0xc018 + 0x40 * (n))
#define IGB_REG_RXDCTL(n)   (0xc028 + 0x40 * (n))

#define IGB_REG_RXCTL       0x5000
#define IGB_REG_RLPML       0x5004
#define IGB_REG_RFCTL       0x5008
#define IGB_REG_RAL(n)      (0x5400 + 8 * (n))
#define IGB_REG_RAH(n)      (0x5404 + 8 * (n))

#define IGB_REG_TDBAL(n)    (0xe000 + 0x40 * (n))
#define IGB_REG_TDBAH(n)    (0xe004 + 0x40 * (n))
#define IGB_REG_TDLEN(n)    (0xe008 + 0x40 * (n))
#define IGB_REG_TDH(n)      (0xe010 + 0x40 * (n))
#define IGB_REG_TXCTL(n)    (0xe014 + 0x40 * (n))
#define IGB_REG_TDT(n)      (0xe018 + 0x40 * (n))
#define IGB_REG_TXDCTL(n)   (0xe028 + 0x40 * (n))
#define IGB_REG_TDWBAL(n)   (0xe038 + 0x40 * (n))
#define IGB_REG_TDWBAH(n)   (0xe03c + 0x40 * (n))


#define IGB_CTRL_FD         0x1
#define IGB_CTRL_GIO_MASTER_DISABLE     0x4
#define IGB_CTRL_SLU        0x40
#define IGB_CTRL_ILOS       0x80
#define IGB_CTRL_FRCDPLX    0x1000
#define IGB_CTRL_RST        0x04000000
#define IGB_CTRL_VME        0x40000000
#define IGB_CTRL_PHY_RST    0x80000000

#define IGB_STATUS_GIO_MASTER_ENABLE    (1 << 19)

#define IGB_TCTL_EN         (1 << 1)
#define IGB_TCTL_PSP        (1 << 2)
#define IGB_TCTL_SWXOFF     (1 << 22)
#define IGB_TCTL_RTLC       (1 << 24)

#define IGB_RCTL_RXEN       0x2
#define IGB_RCTL_SBP        0x4
#define IGB_RCTL_UPE        0x8
#define IGB_RCTL_MPE        0x10
#define IGB_RCTL_LPE        0x20
#define IGB_RCTL_BAM        0x8000
#define IGB_RCTL_BSIZE_2048 0x0
#define IGB_RCTL_BSIZE_1024 (0x1 << 16)
#define IGB_RCTL_BSIZE_512  (0x2 << 16)
#define IGB_RCTL_BSIZE_256  (0x3 << 16)
#define IGB_RCTL_VFE        (1 << 18)
#define IGB_RCTL_CFIEN      (1 << 19)
#define IGB_RCTL_CFI        (1 << 20)
#define IGB_RCTL_PSP        (1 << 21)
#define IGB_RCTL_DPF        (1 << 22)
#define IGB_RCTL_PMCF       (1 << 23)
#define IGB_RCTL_SECRC      (1 << 26)

#define IGB_SRRCTL_BSIZEPACKET_8K       (0x8 << 0)
#define IGB_SRRCTL_BSIZEPACKET_10K      (0xa << 0)
#define IGB_SRRCTL_BSIZEHEADER_256      (0x4 << 8)
#define IGB_SRRCTL_DESCTYPE_ADVANCED    (0x1 << 25)
#define IGB_SRRCTL_TIMESTAMP            (1 << 30)
#define IGB_SRRCTL_DROP_EN              (1UL << 31)

#define IGB_SWSM_SMBI       0x1
#define IGB_SWSM_SWESMBI    0x2

#define IGB_RXDCTL_ENABLE   (1 << 25)

#define IGB_TXDCTL_ENABLE   (1 << 25)

/*
 * Receive descriptor
 */
struct igb_rx_desc_read {
    uint64_t pkt_addr;          /* Bit 0: A0 */
    volatile uint64_t hdr_addr; /* Bit 0: DD */
} __attribute__ ((packed));
struct igb_rx_desc_wb {
    uint32_t info0;
    uint32_t info1;
    uint32_t staterr;
    uint16_t length;
    uint16_t vlan;
} __attribute__ ((packed));
union igb_rx_desc {
    struct igb_rx_desc_read read;
    struct igb_rx_desc_wb wb;
} __attribute__ ((packed));

/*
 * Transmit descriptor
 */
struct igb_tx_desc_ctx {
    uint32_t vlan_maclen_iplen;
    uint32_t launchtime;        /* 25-bits in 32 ns */
    uint64_t other;
} __attribute__ ((packed));
struct igb_tx_desc_data {
    uint64_t pkt_addr;
    uint16_t length;
    uint8_t dtyp_mac;
    uint8_t dcmd;
    uint32_t paylen_popts_idx_sta;
} __attribute__ ((packed));
struct igb_tx_desc_wb {
    uint64_t timestamp;
    uint32_t rsv0;
    uint8_t sta;
    uint8_t rsv1[3];
};
union igb_tx_desc {
    struct igb_tx_desc_ctx ctx;
    struct igb_tx_desc_data data;
    struct igb_tx_desc_wb wb;
} __attribute__ ((packed));

/*
 * Rx ring buffer
 */
struct igb_rx_ring {
    union igb_rx_desc *descs;
    void **bufs;
    uint16_t tail;
    uint16_t head;
    uint16_t soft_head;
    uint16_t len;
    /* Queue information */
    uint16_t idx;               /* Queue index */
    void *mmio;                 /* MMIO */
};

/*
 * Tx ring buffer
 */
struct igb_tx_ring {
    union igb_tx_desc *descs;
    void **bufs;
    uint16_t tail;
    uint16_t head;
    uint16_t soft_head;
    uint16_t len;
    /* Write-back */
    uint32_t *tdwba;
    /* Queue information */
    uint16_t idx;               /* Queue index */
    void *mmio;                 /* MMIO */
};

/*
 * igb device
 */
struct igb_device {
    void *mmio;
    uint8_t macaddr[6];
    uint16_t device_id;
};

/*
 * Prototype declarations
 */
static __inline__ int igb_read_mac_address(struct igb_device *);

/*
 * Check if the device is igb
 */
static __inline__ int
igb_is_igb(uint16_t vendor_id, uint16_t device_id)
{
    if ( 0x8086 != vendor_id ) {
        return 0;
    }
    switch ( device_id ) {
    case IGB_I211:
    case IGB_I219LM:
        return 1;
    default:
        return 0;
    }
}

/*
 * Initialize igb device
 */
static __inline__ struct igb_device *
igb_init(uint16_t device_id, uint16_t bus, uint16_t slot, uint16_t func)
{
    struct igb_device *dev;
    uint64_t pmmio;
    uint32_t m32;

    /* Allocate an igb device data structure */
    dev = malloc(sizeof(struct igb_device));
    if ( NULL == dev ) {
        return NULL;
    }

    /* Read MMIO */
    pmmio = pci_read_mmio(bus, slot, func);
    dev->mmio = driver_mmap((void *)pmmio, IGB_MMIO_SIZE);
    if ( NULL == dev->mmio ) {
        /* Error */
        free(dev);
        return NULL;
    }

    /* Initialize the PCI configuration space */
    m32 = pci_read_config(bus, slot, func, 0x4);
    pci_write_config(bus, slot, func, 0x04, m32 | 0x7);

    /* Get the device MAC address */
    igb_read_mac_address(dev);

    return dev;
}

/*
 * The number of supported Tx queues
 */
static __inline__ int
igb_max_tx_queues(struct igb_device *dev)
{
    (void)dev;
    return 4;
}

/*
 * Get the device MAC address
 */
static __inline__ int
igb_read_mac_address(struct igb_device *dev)
{
    uint32_t m32;

    /* Read MAC address */
    m32 = rd32(dev->mmio, IGB_REG_RAL(0));
    dev->macaddr[0] = m32 & 0xff;
    dev->macaddr[1] = (m32 >> 8) & 0xff;
    dev->macaddr[2] = (m32 >> 16) & 0xff;
    dev->macaddr[3] = (m32 >> 24) & 0xff;
    m32 = rd32(dev->mmio, IGB_REG_RAH(0));
    dev->macaddr[4] = m32 & 0xff;
    dev->macaddr[5] = (m32 >> 8) & 0xff;

    return 0;
}

/*
 * Initialize the hardware
 */
static __inline__ int
igb_init_hw(struct igb_device *dev)
{
    ssize_t i;
    uint32_t m32;

    /* Initialization sequence: S4.5.3 */

    /* Disable interrupts */
    wr32(dev->mmio, IGB_REG_EIMC, 0x7fffffff);
    /* Clear any pending interrupts */
    rd32(dev->mmio, IGB_REG_EICR);

    /* Issue global reset and perform general configuration */
    wr32(dev->mmio, IGB_REG_CTRL,
         rd32(dev->mmio, IGB_REG_CTRL) | IGB_CTRL_RST);
    for ( i = 0; i < 128; i++ ) {
        /* Sleep 1 ms */
        busywait(1000);
        m32 = rd32(dev->mmio, IGB_REG_CTRL);
        if ( !(m32 & IGB_CTRL_RST) ) {
            break;
        }
    }
    if ( m32 & IGB_CTRL_RST ) {
        printf("Error on reset %p %d\n", dev->mmio, m32);
        return -1;
    }

    /* Disable GIO master */
    wr32(dev->mmio, IGB_REG_CTRL,
         rd32(dev->mmio, IGB_REG_CTRL) | IGB_CTRL_GIO_MASTER_DISABLE);
    for ( i = 0; i < 128; i++ ) {
        /* Sleep 1 ms */
        busywait(1000);
        m32 = rd32(dev->mmio, IGB_REG_STATUS);
        if ( !(m32 & IGB_STATUS_GIO_MASTER_ENABLE) ) {
            break;
        }
    }
    if ( m32 & IGB_STATUS_GIO_MASTER_ENABLE ) {
        printf("Error on disabling PCIe master %x\n", m32);
        //return -1;
    }

    /* Issue a global reset (a.k.a. software reset) */
    wr32(dev->mmio, IGB_REG_CTRL,
         rd32(dev->mmio, IGB_REG_CTRL) | IGB_CTRL_RST);
    for ( i = 0; i < 128; i++ ) {
        /* Sleep 1 ms */
        busywait(1000);
        m32 = rd32(dev->mmio, IGB_REG_CTRL);
        if ( !(m32 & IGB_CTRL_RST) ) {
            break;
        }
    }
    if ( m32 & IGB_CTRL_RST ) {
        printf("Error on reset %p %d\n", dev->mmio, m32);
        return -1;
    }

    /* Disable interrupts again */
    wr32(dev->mmio, IGB_REG_EIMC, 0x7fffffff);

    /* Setup the PHY and the link */

    /* Initialize all statistical counters; Reset when Memory Access Enable bit
       of the PCIe */

    /* Initialize receive */
    /* Initialize transmit */
    /* Enable interrupts */

    return 0;
}

/*
 * Setup Rx port
 */
static __inline__ int
igb_setup_rx(struct igb_device *dev)
{
    ssize_t i;

    /* S4.5.9 */

    /* Setup multicast array table */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, IGB_REG_MTA(i), 0);
    }

    /* Program Rx packet buffer size */
    /* Set cfg_ts_en */
    wr32(dev->mmio, IGB_REG_RXPBSIZE,
         rd32(dev->mmio, IGB_REG_RXPBSIZE) | (1UL << 31));

    /* Clear multi queue */
    wr32(dev->mmio, IGB_REG_MRQC, 0);

    /* CRC strip */
    wr32(dev->mmio, IGB_REG_RCTL,
         rd32(dev->mmio, IGB_REG_RCTL) | IGB_RCTL_SBP | IGB_RCTL_UPE
         | IGB_RCTL_MPE | IGB_RCTL_LPE | IGB_RCTL_BAM | IGB_RCTL_SECRC);

    return 0;
}

/*
 * Enable Rx
 */
static __inline__ int
igb_enable_rx(struct igb_device *dev)
{
    /* Enable RX */
    wr32(dev->mmio, IGB_REG_RCTL,
         rd32(dev->mmio, IGB_REG_RCTL) | IGB_RCTL_RXEN);

    return 0;
}

/*
 * Disable Rx
 */
static __inline__ int
igb_disable_rx(struct igb_device *dev)
{
    /* Disable RX */
    wr32(dev->mmio, IGB_REG_RCTL,
         rd32(dev->mmio, IGB_REG_RCTL) & ~IGB_RCTL_RXEN);

    return 0;
}


/*
 * Setup Tx
 */
static __inline__ int
igb_setup_tx(struct igb_device *dev)
{
    /* S4.5.10 */

    /* Program TCTL */

    /* Program TXPBSIZE */
    wr32(dev->mmio, IGB_REG_TXPBSIZE, 0x04000014);

    return 0;
}

/*
 * Enable Tx
 */
static __inline__ int
igb_enable_tx(struct igb_device *dev)
{
    /* Enable Tx */
    wr32(dev->mmio, IGB_REG_TCTL,
         rd32(dev->mmio, IGB_REG_TCTL) | IGB_TCTL_EN);

    return 0;
}

/*
 * Disable Tx
 */
static __inline__ int
igb_disable_tx(struct igb_device *dev)
{
    /* Disable Tx */
    wr32(dev->mmio, IGB_REG_TCTL,
         rd32(dev->mmio, IGB_REG_TCTL) & ~IGB_TCTL_EN);

    return 0;
}

/*
 * Setup Rx ring
 */
static __inline__ int
igb_setup_rx_ring(struct igb_device *dev, struct igb_rx_ring *rxring, int idx,
                  void *m, uint64_t v2poff, uint16_t qlen)
{
    union igb_rx_desc *rxdesc;
    ssize_t i;
    uint32_t m32;
    uint64_t m64;

    /* Check the queue index first */
    if ( 0 != idx ) {
        return -1;
    }

    /* Copy MMIO base address */
    rxring->mmio = dev->mmio;

    /* Use queue #0 */
    rxring->idx = idx;

    rxring->tail = 0;
    rxring->head = 0;
    rxring->soft_head = 0;

    /* up to 64 K minus 8 */
    rxring->len = qlen;

    /* Allocate for descriptors */
    rxring->descs = m;
    m += sizeof(union igb_rx_desc) * qlen;
    rxring->bufs = m;

    for ( i = 0; i < rxring->len; i++ ) {
        rxdesc = &rxring->descs[i];
        rxdesc->read.pkt_addr = 0;
        rxdesc->read.hdr_addr = 0;
    }

    m64 = (uint64_t)rxring->descs + v2poff;
    wr32(rxring->mmio, IGB_REG_RDBAL(rxring->idx), m64 & 0xffffffffULL);
    wr32(rxring->mmio, IGB_REG_RDBAH(rxring->idx), m64 >> 32);
    wr32(rxring->mmio, IGB_REG_RDLEN(rxring->idx),
         rxring->len * sizeof(union igb_rx_desc));

    wr32(rxring->mmio, IGB_REG_SRRCTL(rxring->idx),
         IGB_SRRCTL_BSIZEPACKET_10K | IGB_SRRCTL_BSIZEHEADER_256
         | IGB_SRRCTL_DESCTYPE_ADVANCED | IGB_SRRCTL_TIMESTAMP
         | IGB_SRRCTL_DROP_EN);

    /* Enable this queue */
    wr32(rxring->mmio, IGB_REG_RXDCTL(rxring->idx), IGB_RXDCTL_ENABLE);
    for ( i = 0; i < 10; i++ ) {
        busywait(1);
        m32 = rd32(rxring->mmio, IGB_REG_RXDCTL(rxring->idx));
        if ( m32 & IGB_RXDCTL_ENABLE ) {
            break;
        }
    }
    if ( !(m32 & IGB_RXDCTL_ENABLE) ) {
        printf("Error on enabling an RX queue.\n");
    }

    wr32(rxring->mmio, IGB_REG_RDH(rxring->idx), 0);
    wr32(rxring->mmio, IGB_REG_RDT(rxring->idx), 0);

    return 0;
}

static __inline__ int
igb_rx_refill(struct igb_rx_ring *rxring, void *pkt, void *hdr)
{
    union igb_rx_desc *rxdesc;
    uint16_t new_tail;

    new_tail = rxring->tail + 1 < rxring->len ? rxring->tail + 1 : 0;
    if ( new_tail == rxring->head ) {
        /* Buffer is full */
        return 0;
    }
    rxdesc = &rxring->descs[rxring->tail];
    rxdesc->read.pkt_addr = (uint64_t)pkt;
    rxdesc->read.hdr_addr = 0;
    rxring->bufs[rxring->tail] = hdr;
    rxring->tail = new_tail;

    return 1;
}

static __inline__ void
igb_rx_commit(struct igb_rx_ring *rxring)
{
    __sync_synchronize();
    wr32(rxring->mmio, IGB_REG_RDT(rxring->idx), rxring->tail);
}

static __inline__ int
igb_rx_dequeue(struct igb_rx_ring *rxring, void **hdr)
{
    uint16_t head;
    int len;

    if ( rxring->head == rxring->soft_head ) {
        /* Update the head */
        rxring->head = rd32(rxring->mmio, IGB_REG_RDH(rxring->idx));
        if ( rxring->head == rxring->soft_head ) {
            return -1;
        }
    }
    head = rxring->soft_head + 1 < rxring->len ? rxring->soft_head + 1 : 0;
    *hdr = rxring->bufs[rxring->soft_head];
    len = rxring->descs[rxring->soft_head].wb.length;
    rxring->soft_head = head;

    return len;
}

/*
 * Setup Tx ring
 */
static __inline__ int
igb_setup_tx_ring(struct igb_device *dev, struct igb_tx_ring *txring, int idx,
                  void *m, uint64_t v2poff, uint16_t qlen)
{
    union igb_tx_desc *txdesc;
    ssize_t i;
    uint32_t m32;
    uint64_t m64;

    /* Check the queue index first */
    if ( idx >= 4 ) {
        return -1;
    }

    txring->mmio = dev->mmio;

    txring->idx = idx;

    txring->tail = 0;
    txring->head = 0;
    txring->soft_head = 0;
    txring->len = qlen;

    /* Allocate for descriptors */
    txring->descs = m;
    m += sizeof(union igb_tx_desc) * qlen;
    txring->bufs = m;
    m += sizeof(void *) * qlen;
    txring->tdwba = m;

    for ( i = 0; i < txring->len; i++ ) {
        txdesc = &txring->descs[i];
        txdesc->data.pkt_addr = 0;
        txdesc->data.length = 0;
        txdesc->data.dtyp_mac = 0;
        txdesc->data.dcmd = 0;
        txdesc->data.paylen_popts_idx_sta = 0;
    }

    m64 = (uint64_t)txring->descs + v2poff;
    wr32(txring->mmio, IGB_REG_TDBAH(txring->idx), m64 >> 32);
    wr32(txring->mmio, IGB_REG_TDBAL(txring->idx), m64 & 0xffffffffUL);
    wr32(txring->mmio, IGB_REG_TDLEN(txring->idx),
         txring->len * sizeof(union igb_tx_desc));
    wr32(txring->mmio, IGB_REG_TDH(txring->idx), 0);
    wr32(txring->mmio, IGB_REG_TDT(txring->idx), 0);

    /* Write-back */
    m64 = (uint64_t)txring->tdwba + v2poff;
    wr32(txring->mmio, IGB_REG_TDWBAH(txring->idx), m64 >> 32);
    wr32(txring->mmio, IGB_REG_TDWBAL(txring->idx), (m64 & 0xfffffffc) | 1);
    *(txring->tdwba) = 0;

    wr32(txring->mmio, IGB_REG_TXDCTL(txring->idx), IGB_TXDCTL_ENABLE
         | (16 << 16) /* WTHRESH */
         | (0 << 8) /* HTHRESH */
         | (0 << 0) /* PTHRESH */);
    for ( i = 0; i < 10; i++ ) {
        busywait(1);
        m32 = rd32(txring->mmio, IGB_REG_TXDCTL(txring->idx));
        if ( m32 & IGB_TXDCTL_ENABLE ) {
            break;
        }
    }
    if ( !(m32 & IGB_TXDCTL_ENABLE) ) {
        printf("Error on enabling a TX queue. (Q=%d)\n", txring->idx);
    }

    return 0;
}

static __inline__ int
igb_tx_enqueue(struct igb_tx_ring *txring, void *pkt, void *hdr, size_t length)
{
    union igb_tx_desc *txdesc;
    uint16_t new_tail;

    new_tail = txring->tail + 1 < txring->len ? txring->tail + 1 : 0;
    if ( new_tail == txring->soft_head ) {
        /* Buffer is full */
        return 0;
    }
    txdesc = &txring->descs[txring->tail];
    txdesc->data.pkt_addr = (uint64_t)pkt;
    txdesc->data.length = length;
    txdesc->data.dtyp_mac = (3 << 4);
    txdesc->data.dcmd = (1 << 5) | (1 << 3) | (1 << 1) | 1; /* (1<<3): WB */
    txdesc->data.paylen_popts_idx_sta = ((uint64_t)length << 14);
    txring->bufs[txring->tail] = hdr;
    txring->tail = new_tail;

    return 1;
}

static __inline__ void
igb_tx_commit(struct igb_tx_ring *txring)
{
    __sync_synchronize();
    wr32(txring->mmio, IGB_REG_TDT(txring->idx), txring->tail);
}

static __inline__ int
igb_calc_rx_ring_memsize(struct igb_rx_ring *rx, uint16_t qlen)
{
    (void)rx;
    return (sizeof(union igb_rx_desc) + sizeof(void *)) * qlen;
}
static __inline__ int
igb_calc_tx_ring_memsize(struct igb_tx_ring *tx, uint16_t qlen)
{
    (void)tx;
    return (sizeof(union igb_tx_desc) + sizeof(void *)) * qlen + 128;
}

static __inline__ int
igb_collect_buffer(struct igb_tx_ring *txring, void **hdr)
{
    txring->head = *txring->tdwba;
    if ( txring->soft_head == txring->head ) {
        return 0;
    }

    *hdr = txring->bufs[txring->soft_head];

    txring->soft_head
        = txring->soft_head + 1 < txring->len ? txring->soft_head + 1 : 0;

    return 1;
}

#endif /* _IGB_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
