#ifndef __WHITERABBIT_MRP_ATTR_H
#define __WHITERABBIT_MRP_ATTR_H

#include <stdint.h>

#include "mrp.h"

int mrp_attr_cmp(const struct mrp_attr *attr, uint8_t type, uint8_t len,
    const void *value);
int mrp_attr_subsequent(struct mrp_application *app, void *firstval,
    void *secondval, int type, int len);
struct mrp_attr *mrp_attr_lookup(struct mrp_attr *head,
    uint8_t type, uint8_t len, const void *value);
void mrp_attr_insert(struct mrp_attr **root, struct mrp_attr *attr);
struct mrp_attr *mrp_attr_create(uint8_t type, uint8_t len, const void *value);
void mrp_attr_destroy(struct mrp_attr **root, struct mrp_attr *attr);
void mrp_attr_destroy_all(struct mrp_attr **head);


#endif /*__WHITERABBIT_MRP_ATTR_H*/
