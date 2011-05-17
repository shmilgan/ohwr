#ifndef __WHITERABBIT_RTU_IRQ_H
#define __WHITERABBIT_RTU_IRQ_H

#define __WR_IOC_MAGIC      '4'

#define WR_RTU_IRQWAIT      _IO(__WR_IOC_MAGIC, 4)
#define WR_RTU_IRQENA	    _IO(__WR_IOC_MAGIC, 5)

#endif /*__WHITERABBIT_RTU_IRQ_H*/
