#include "malloc.h"

#include "mrp_attr.h"

/* Inserts a before b */
static void insert_before(struct mrp_attr *b, struct mrp_attr *a)
{
    a->prev = b->prev;
    b->prev->next = a;

    a->next = b;
    b->prev = a;
}

int mrp_attr_cmp(const struct mrp_attr *attr, uint8_t type, uint8_t len,
    const void *value)
{
    if (attr->type != type)
        return attr->type - type;
    if (attr->len != len)
        return attr->len - len;
    return memcmp(attr->value, value, len);
}

struct mrp_attr *mrp_attr_lookup(struct mrp_attr *head,
    uint8_t type,  uint8_t len, const void *value)
{
    struct mrp_attr *node = head;

    if (!node)
        return NULL;

    do {
        if (mrp_attr_cmp(node, type, len, value) == 0)
            return node;
        node = node->next;
    } while (node != head);
    return NULL;
}

/* Inserts attribute in the ordered list pointed by head */
void mrp_attr_insert(struct mrp_attr **head, struct mrp_attr *attr)
{
    struct mrp_attr *node = *head;

    if (node) {
        do {
            if (mrp_attr_cmp(node, attr->type, attr->len, attr->value) > 0)
                break;
            else
                node = node->next;
        } while (node != *head);
        insert_before(node, attr);
    }
    if (node == *head)
        *head = attr;
}

struct mrp_attr *mrp_attr_create(uint8_t type, uint8_t len, const void *value)
{
    struct mrp_attr *attr;

    attr = (struct mrp_attr*)malloc(sizeof(struct mrp_attr) + len);
    if (!attr)
        return attr;
    attr->app_state = MRP_APPLICANT_VO;
    attr->reg_state = MRP_REGISTRAR_MT;
    attr->type = type;
    attr->len  = len;
    memcpy(attr->value, value, len);
    attr->next = attr->prev = attr;
    return attr;
}

void mrp_attr_destroy(struct mrp_attr **head, struct mrp_attr *attr)
{
    if (attr->prev)
        attr->prev->next = attr->next;
    if (attr->next)
        attr->next->prev = attr->prev;
    if (*head == attr)
        *head = attr->next;
    free(attr);
}

void mrp_attr_destroy_all(struct mrp_attr **head)
{
    while(*head)
        mrp_attr_destroy(head, *head);
}

int mrp_attr_subsequent(struct mrp_application *app, void *firstval,
    void *secondval, int type, int len)
{
    void *nextval;
    int subsequent;

    nextval = malloc(len);
    if (!nextval)
        return -ENOMEM;
    memcpy(nextval, firstval, len);
    app->nextval(type, len, nextval);
    subsequent = (memcmp(nextval, secondval, len) == 0);
    free(nextval);
    return subsequent;
}
