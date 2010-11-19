#ifndef __WHITERABBIT_VIC_H
#define __WHITERABBIT_VIC_H


#define FPGA_BASE_VIC 0x70030000
#define WRMCH_VIC_MAX_IRQS 32

typedef void (*wrvic_irq_t)(void *dev_id);

int wrmch_vic_request_irq(int irq, wrvic_irq_t handler, void *dev_id);
void wrmch_vic_free_irq(int irq, void *dev_id);
void wrmch_vic_enable_irq(int irq);
void wrmch_vic_disable_irq(int irq);

#endif
