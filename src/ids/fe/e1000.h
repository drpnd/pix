/*_
 * Copyright (c) 2016 Hirochika Asai <asai@jar.jp>
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

#ifndef _E1000_H
#define _E1000_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <mki/driver.h>
#include "pci.h"
#include "common.h"

#define E1000_NRXQ              1
#define E1000_NTXQ              1

#define E1000_MMIO_SIZE         0x6000

/* MMIO registers */
#define E1000_REG_CTRL          0x00
#define E1000_REG_STATUS        0x08
#define E1000_REG_EEC           0x10
#define E1000_REG_CTRL_EXT      0x18
#define E1000_REG_EERD          0x14
#define E1000_REG_MDIC          0x20
#define E1000_REG_ICR           0x00c0
#define E1000_REG_ITR           0x00c4
#define E1000_REG_ICS           0x00c8
#define E1000_REG_IMS           0x00d0
#define E1000_REG_IMC           0x00d8
#define E1000_REG_RCTL          0x0100
#define E1000_REG_RDBAL         0x2800
#define E1000_REG_RDBAH         0x2804
#define E1000_REG_RDLEN         0x2808
#define E1000_REG_RDH           0x2810  /* head */
#define E1000_REG_RDT           0x2818  /* tail */
#define E1000_REG_RXDCTL        0x2828
#define E1000_REG_TCTL          0x0400  /* transmit control */
#define E1000_REG_TDBAL         0x3800
#define E1000_REG_TDBAH         0x3804
#define E1000_REG_TDLEN         0x3808
#define E1000_REG_TDH           0x3810  /* head */
#define E1000_REG_TDT           0x3818  /* tail */
#define E1000_REG_MTA(n)        (0x5200 + (n) * 4)  /* x128 */
#define E1000_REG_TXDCTL        0x03828
#define E1000_REG_RAL           0x5400
#define E1000_REG_RAH           0x5404

#define E1000_CTRL_FD           1       /* Full duplex */
#define E1000_CTRL_LRST         (1<<3)  /* Link reset */
#define E1000_CTRL_ASDE         (1<<5)  /* Auto speed detection enable */
#define E1000_CTRL_SLU          (1<<6)  /* Set linkup */
#define E1000_CTRL_PHY_RST      (1<<31)  /* PHY reset */

#define E1000_CTRL_RST          (1<<26)
#define E1000_CTRL_VME          (1<<30)

#define E1000_CTRL_EXT_LINK_MODE_MASK (3<<22)
#define E1000_CTRL_EXT_EE_RST   (1<<13)

#define E1000_RCTL_EN           (1<<1)
#define E1000_RCTL_SBP          (1<<2)
#define E1000_RCTL_UPE          (1<<3)  /* Unicast promiscuous */
#define E1000_RCTL_MPE          (1<<4)  /* Multicast promiscuous */
#define E1000_RCTL_LPE          (1<<5)  /* Long packet reception */
#define E1000_RCTL_BAM          (1<<15) /* Broadcast accept mode */
#define E1000_RCTL_BSEX         (1<<25) /* Buffer size extension */
#define E1000_RCTL_SECRC        (1<<26) /* Strip ethernet CRC from incoming packet */

#define E1000_RCTL_BSIZE_8192   ((2<<16) | E1000_RCTL_BSEX)
#define E1000_RCTL_BSIZE_SHIFT  16

#define E1000_TCTL_EN           (1<<1)
#define E1000_TCTL_PSP          (1<<3)  /* pad short packets */
#define E1000_TCTL_MULR         (1<<28)

#define E1000_TXDCTL_GRAN_CL    0
#define E1000_TXDCTL_GRAN_DESC  (1<<24)
#define E1000_TXDCTL_PTHRESH_SHIFT 0
#define E1000_TXDCTL_HTHRESH_SHIFT 8
#define E1000_TXDCTL_WTHRESH_SHIFT 16
#define E1000_TXDCTL_LTHRESH_SHIFT 25

#define E1000_82543GC           0x1004
#define E1000_PRO1000MT         0x100e  /* Intel Pro 1000/MT */
#define E1000_82545EM           0x100f
#define E1000_82541PI           0x107c
#define E1000_82573L            0x109a
#define E1000_82567LM           0x10f5
#define E1000_82577LM           0x10ea
#define E1000_82579LM           0x1502



/*
 * Receive descriptor
 */
struct e1000_rx_desc {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__ ((packed));

/*
 * Transmit descriptor
 */
struct e1000_tx_desc {
    volatile uint64_t address;
    uint32_t length:20;
    uint32_t dtyp:4;
    uint32_t dcmd:8;
    volatile uint8_t sta:4;
    uint8_t rsv:4;
    uint8_t popts;
    uint16_t special;
} __attribute__ ((packed));

/*
 * Rx ring buffer
 */
struct e1000_rx_ring {
    /* Descriptors */
    struct e1000_rx_desc *descs;
    /* Array of pointers to packet buffers */
    void **bufs;
    uint16_t tail;
    uint16_t head;
    uint16_t soft_head;
    uint16_t len;
    /* Queue information */
    void *mmio;                 /* MMIO */
};

/*
 * Tx ring buffer
 */
struct e1000_tx_ring {
    /* Descriptors */
    struct e1000_tx_desc *descs;
    /* Packet buffers to be collected */
    void **bufs;
    uint16_t soft_head;
    uint16_t head;
    uint16_t tail;
    uint16_t len;
    /* Queue information */
    void *mmio;                 /* MMIO */
};

/*
 * e1000 device
 */
struct e1000_device {
    void *mmio;
    uint8_t macaddr[6];
    uint16_t device_id;
    int pirq;
};

/*
 * Prototype declarations
 */
static __inline__ int e1000_is_e1000(uint16_t, uint16_t);
static __inline__ int e1000_read_mac_address(struct e1000_device *);

/*
 * Check if the device is e1000
 */
static __inline__ int
e1000_is_e1000(uint16_t vendor_id, uint16_t device_id)
{
    /* Must be Intel */
    if ( 0x8086 != vendor_id ) {
        return 0;
    }
    switch ( device_id ) {
    case E1000_82543GC:
    case E1000_PRO1000MT:
    case E1000_82545EM:
    case E1000_82541PI:
    case E1000_82573L:
    case E1000_82567LM:
    case E1000_82577LM:
    case E1000_82579LM:
        return 1;
    default:
        return 0;
    }

    return 0;
};

/*
 * Initialize e1000 device
 */
static __inline__ struct e1000_device *
e1000_init(uint16_t device_id, uint16_t bus, uint16_t slot, uint16_t func)
{
    struct e1000_device *dev;
    uint64_t pmmio;
    uint32_t m32;

    /* Allocate an ixgbe device data structure */
    dev = malloc(sizeof(struct e1000_device));
    if ( NULL == dev ) {
        return NULL;
    }
    dev->device_id = device_id;

    /* Read MMIO */
    pmmio = pci_read_mmio(bus, slot, func);
    dev->mmio = driver_mmap((void *)pmmio, E1000_MMIO_SIZE);
    if ( NULL == dev->mmio ) {
        /* Error */
        free(dev);
        return NULL;
    }

    /* Initialize the PCI configuration space */
    m32 = pci_read_config(bus, slot, func, 0x4);
    pci_write_config(bus, slot, func, 0x4, m32 | 0x7);

    /* Get the device MAC address */
    e1000_read_mac_address(dev);

    return dev;
}

/*
 * The number of supported Tx queues
 */
static __inline__ int
e1000_max_tx_queues(struct e1000_device *dev)
{
    (void)dev;
    return 1;
}

/*
 * Read from EEPROM
 */
static __inline__ uint16_t
e1000_eeprom_read_8254x(void *mmio, uint8_t addr)
{
    uint32_t data;

    /* Start */
    wr32(mmio, E1000_REG_EERD, ((uint32_t)addr << 8) | 1);

    /* Until it's done */
    while ( !((data = rd32(mmio, E1000_REG_EERD)) & (1 << 4)) ) {
        /* pause */
    }

    return (uint16_t)((data >> 16) & 0xffff);
}
static __inline__ uint16_t
e1000_eeprom_read(void *mmio, uint8_t addr)
{
    uint16_t data;
    uint32_t tmp;

    /* Start */
    wr32(mmio, E1000_REG_EERD, ((uint32_t)addr << 2) | 1);

    /* Until it's done */
    while ( !((tmp = rd32(mmio, E1000_REG_EERD)) & (1 << 1)) ) {
        /* pause */
    }
    data = (uint16_t)((tmp >> 16) & 0xffff);

    return data;
}

/*
 * Get the device MAC address
 */
static __inline__ int
e1000_read_mac_address(struct e1000_device *dev)
{
    uint16_t m16;
    uint32_t m32;

    switch ( dev->device_id ) {
    case E1000_PRO1000MT:
    case E1000_82545EM:
    case E1000_82543GC:
        /* Read MAC address */
        m16 = e1000_eeprom_read_8254x(dev->mmio, 0);
        dev->macaddr[0] = m16 & 0xff;
        dev->macaddr[1] = (m16 >> 8) & 0xff;
        m16 = e1000_eeprom_read_8254x(dev->mmio, 1);
        dev->macaddr[2] = m16 & 0xff;
        dev->macaddr[3] = (m16 >> 8) & 0xff;
        m16 = e1000_eeprom_read_8254x(dev->mmio, 2);
        dev->macaddr[4] = m16 & 0xff;
        dev->macaddr[5] = (m16 >> 8) & 0xff;
        break;

    case E1000_82541PI:
    case E1000_82573L:
        /* Read MAC address */
        m16 = e1000_eeprom_read(dev->mmio, 0);
        dev->macaddr[0] = m16 & 0xff;
        dev->macaddr[1] = (m16 >> 8) & 0xff;
        m16 = e1000_eeprom_read(dev->mmio, 1);
        dev->macaddr[2] = m16 & 0xff;
        dev->macaddr[3] = (m16 >> 8) & 0xff;
        m16 = e1000_eeprom_read(dev->mmio, 2);
        dev->macaddr[4] = m16 & 0xff;
        dev->macaddr[5] = (m16 >> 8) & 0xff;
        break;

    case E1000_82567LM:
    case E1000_82577LM:
    case E1000_82579LM:
        /* Read MAC address */
        m32 = rd32(dev->mmio, E1000_REG_RAL);
        dev->macaddr[0] = m32 & 0xff;
        dev->macaddr[1] = (m32 >> 8) & 0xff;
        dev->macaddr[2] = (m32 >> 16) & 0xff;
        dev->macaddr[3] = (m32 >> 24) & 0xff;
        m32 = rd32(dev->mmio, E1000_REG_RAH);
        dev->macaddr[4] = m32 & 0xff;
        dev->macaddr[5] = (m32 >> 8) & 0xff;
        break;
    }

    return 0;
}

/*
 * Initialize the hardware
 */
static __inline__ int
e1000_init_hw(struct e1000_device *dev)
{
    ssize_t i;

    /* Initialize */

    /* Disable interrupts */
    wr32(dev->mmio, E1000_REG_IMC, 0xffffffff);

    /* Reset the device */
    wr32(dev->mmio, E1000_REG_CTRL,
         rd32(dev->mmio, E1000_REG_CTRL) | E1000_CTRL_RST);

    /* Wait 100 us */
    busywait(100);

    /* Disable interrupts again */
    wr32(dev->mmio, E1000_REG_IMC, 0xffffffff);

    /* Set link up */
    wr32(dev->mmio, E1000_REG_CTRL,
         rd32(dev->mmio, E1000_REG_CTRL) | E1000_CTRL_SLU);

    /* Set link mode to direct copper interface */
    wr32(dev->mmio, E1000_REG_CTRL_EXT,
         rd32(dev->mmio, E1000_REG_CTRL_EXT) & ~E1000_CTRL_EXT_LINK_MODE_MASK);

    /* Link up and enable 802.1Q VLAN */
    wr32(dev->mmio, E1000_REG_CTRL,
         rd32(dev->mmio, E1000_REG_CTRL) | E1000_CTRL_SLU | E1000_CTRL_VME);

    /* Initialize multicast array table */
    for ( i = 0; i < 128; i++ ) {
        wr32(dev->mmio, E1000_REG_MTA(i), 0);
    }

    /* Set the interrupt throttling rate (bit [15:0] in 256 ns) */
    wr32(dev->mmio, E1000_REG_ITR, 4); /* ~1 microsecond */

    /* Enable interrupt (REG_IMS <- 0x1f6dc, then read REG_ICR ) */
    wr32(dev->mmio, E1000_REG_IMS, 0x1f6dc);
    /* Clear pending interrupts */
    (void)rd32(dev->mmio, E1000_REG_ICR);

    return 0;
}

/*
 * Setup Rx port
 */
static __inline__ int
e1000_setup_rx(struct e1000_device *dev)
{
    (void)dev;
    /* Nothing to do */
    return 0;
}

/*
 * Setup Rx ring
 */
static __inline__ int
e1000_setup_rx_ring(struct e1000_device *dev, struct e1000_rx_ring *rxring,
                    int idx, void *m, uint64_t v2poff, uint16_t qlen)
{
    struct e1000_rx_desc *rxdesc;
    uint64_t m64;
    ssize_t i;

    /* Check the queue index first */
    if ( 0 != idx ) {
        return -1;
    }

    rxring->mmio = dev->mmio;

    rxring->tail = 0;
    rxring->head = 0;
    rxring->soft_head = 0;
    rxring->len = qlen;

    /* Allocate for descriptors */
    rxring->descs = m;
    m += sizeof(struct e1000_rx_desc) * qlen;
    rxring->bufs = m;

    for ( i = 0; i < rxring->len; i++ ) {
        rxdesc = &rxring->descs[i];
        rxdesc->address = 0;
        rxdesc->length = 0;
        rxdesc->checksum = 0;
        rxdesc->status = 0;
        rxdesc->errors = 0;
        rxdesc->special = 0;
        rxring->bufs[i] = NULL;
    }

    m64 = (uint64_t)rxring->descs + v2poff;
    wr32(rxring->mmio, E1000_REG_RDBAH, m64 >> 32);
    wr32(rxring->mmio, E1000_REG_RDBAL, m64 & 0xffffffff);
    wr32(rxring->mmio, E1000_REG_RDLEN,
         rxring->len * sizeof(struct e1000_rx_desc));
    wr32(rxring->mmio, E1000_REG_RDH, 0);
    wr32(rxring->mmio, E1000_REG_RDT, 0);
    wr32(rxring->mmio, E1000_REG_RCTL,
         E1000_RCTL_SBP | E1000_RCTL_UPE
         | E1000_RCTL_MPE | E1000_RCTL_LPE | E1000_RCTL_BAM
         | E1000_RCTL_BSIZE_8192 | E1000_RCTL_SECRC);
    wr32(rxring->mmio, E1000_REG_RXDCTL, (1 << 24));

    /* Enable this ring */
    wr32(rxring->mmio, E1000_REG_RCTL,
         rd32(rxring->mmio, E1000_REG_RCTL) | E1000_RCTL_EN);

    return 0;
}

static __inline__ int
e1000_rx_refill(struct e1000_rx_ring *rxring, void *pkt, void *hdr)
{
    struct e1000_rx_desc *rxdesc;
    uint16_t new_tail;

    new_tail = rxring->tail + 1 < rxring->len ? rxring->tail + 1 : 0;
    if ( new_tail == rxring->head ) {
        /* Buffer is full */
        return 0;
    }
    rxdesc = &rxring->descs[rxring->tail];
    rxdesc->address = (uint64_t)pkt;
    rxdesc->length = 0;
    rxdesc->checksum = 0;
    rxdesc->status = 0;
    rxdesc->errors = 0;
    rxdesc->special = 0;
    rxring->bufs[rxring->tail] = hdr;
    rxring->tail = new_tail;

    return 1;
}

static __inline__ void
e1000_rx_commit(struct e1000_rx_ring *rxring)
{
    wr32(rxring->mmio, E1000_REG_RDT, rxring->tail);
}

static __inline__ int
e1000_rx_dequeue(struct e1000_rx_ring *rxring, void **hdr)
{
    uint16_t head;
    int len;

    if ( rxring->head == rxring->soft_head ) {
        /* Update the head */
        rxring->head = rd32(rxring->mmio, E1000_REG_RDH);
        if ( rxring->head == rxring->soft_head ) {
            return -1;
        }
    }
    head = rxring->soft_head + 1 < rxring->len ? rxring->soft_head + 1 : 0;
    *hdr = rxring->bufs[rxring->soft_head];
    len = rxring->descs[rxring->soft_head].length;
    rxring->soft_head = head;

    return len;
}

/*
 * Setup Tx port
 */
static __inline__ int
e1000_setup_tx(struct e1000_device *dev)
{
    (void)dev;
    /* Nothing to do */
    return 0;
}

/*
 * Setup Tx ring
 */
static __inline__ int
e1000_setup_tx_ring(struct e1000_device *dev, struct e1000_tx_ring *txring,
                    int idx, void *m, uint64_t v2poff, uint16_t qlen)
{
    struct e1000_tx_desc *txdesc;
    uint64_t m64;
    ssize_t i;

    /* Check the queue index first */
    if ( 0 != idx ) {
        return -1;
    }

    txring->mmio = dev->mmio;

    txring->soft_head = 0;
    txring->head = 0;
    txring->tail = 0;
    txring->len = qlen;

    /* Allocate for descriptors */
    txring->descs = m;
    m += sizeof(struct e1000_tx_desc) * qlen;
    txring->bufs = m;

    for ( i = 0; i < txring->len; i++ ) {
        txdesc = &txring->descs[i];
        txdesc->address = 0;
        txdesc->length = 0;
        txdesc->dtyp = 0;
        txdesc->dcmd = 0;
        txdesc->sta = 0;
        txdesc->rsv = 0;
        txdesc->popts = 0;
        txdesc->special = 0;
    }

    m64 = (uint64_t)txring->descs + v2poff;
    wr32(txring->mmio, E1000_REG_TDBAH, m64 >> 32);
    wr32(txring->mmio, E1000_REG_TDBAL, m64 & 0xffffffff);
    wr32(txring->mmio, E1000_REG_TDLEN,
         txring->len * sizeof(struct e1000_tx_desc));
    wr32(txring->mmio, E1000_REG_TDH, 0);
    wr32(txring->mmio, E1000_REG_TDT, 0);
    wr32(txring->mmio, E1000_REG_TCTL,
         E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_MULR);

    return 0;
}

static __inline__ int
e1000_tx_enqueue(struct e1000_tx_ring *txring, void *pkt, void *hdr,
                 size_t length)
{
    struct e1000_tx_desc *txdesc;
    uint16_t new_tail;

    new_tail = txring->tail + 1 < txring->len ? txring->tail + 1 : 0;
    if ( new_tail == txring->soft_head ) {
        /* Buffer is full */
        return 0;
    }
    txdesc = &txring->descs[txring->tail];
    txdesc->address = (uint64_t)pkt;
    txdesc->length = length;
    txdesc->dcmd = (0 << 5) | (1 << 3) | (1 << 1) | 1;;
    txdesc->dtyp = 0;
    txdesc->sta = 0;
    txdesc->rsv = 0;
    txdesc->popts = 0;
    txdesc->special = 0;
    txring->bufs[txring->tail] = hdr;
    txring->tail = new_tail;

    return 1;
}

static __inline__ void
e1000_tx_commit(struct e1000_tx_ring *txring)
{
    wr32(txring->mmio, E1000_REG_TDT, txring->tail);
}


static __inline__ int
e1000_calc_rx_ring_memsize(struct e1000_rx_ring *rx, uint16_t qlen)
{
    (void)rx;
    return (sizeof(struct e1000_rx_desc) + sizeof(void *)) * qlen;
}
static __inline__ int
e1000_calc_tx_ring_memsize(struct e1000_tx_ring *tx, uint16_t qlen)
{
    (void)tx;
    return (sizeof(struct e1000_tx_desc) + sizeof(void *)) * qlen;
}


static __inline__ int
e1000_collect_buffer(struct e1000_tx_ring *txring, void **hdr)
{
    txring->head = rd32(txring->mmio, E1000_REG_TDH);
    if ( txring->soft_head == txring->head ) {
        return 0;
    }

    *hdr = txring->bufs[txring->soft_head];

    txring->soft_head
        = txring->soft_head + 1 < txring->len ? txring->soft_head + 1 : 0;

    return 1;
}

#endif /* _E1000_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
