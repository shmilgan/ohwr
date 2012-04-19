#ifndef PIO_H
#define PIO_H

#include <hw/pio.h>

int shw_pio_mmap_init();
void shw_pio_toggle_pin(pio_pin_t* pin, uint32_t udelay);
void shw_pio_configure_all();
void shw_pio_configure(const pio_pin_t *pin);

#endif //PIO_H