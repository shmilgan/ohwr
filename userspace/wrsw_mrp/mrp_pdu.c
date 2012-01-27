#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "malloc.h"

#include "mrp.h"
#include "mrp_pdu.h"
#include "mrp_attr.h"

inline static int pdu_may_pull(struct mrpdu *pdu, int len)
{
    return (pdu->len - pdu->pos) >= len;
}

inline static void pdu_pull(struct mrpdu *pdu, int len)
{
    pdu->pos += len;
}

inline static int pdu_tailroom(struct mrpdu *pdu)
{
    return ETH_DATA_LEN - pdu->pos;
}

inline static int leaveall_event(uint8_t octet)
{
    return octet >> 5; // 3 bits
}

inline static int number_of_values(uint8_t high, uint8_t low)
{
    return ((high & 0x1f) << 8) | low; //13 bits
}

/* Returns number of ThreePackedEvents in a VectorAttribute
   @param nval number of events in encoded in VectorAttribute */
inline static int ntpe(int nval)
{
    return ceil((double)nval / 3);
}


/* Returns the message reception event corresponding to the AttributeEvent*/
static enum mrp_event mrp_event(enum mrp_attr_event e) {
    switch (e) {
    case MRP_NEW:
        return MRP_EVENT_R_NEW;
    case MRP_JOIN_IN:
        return MRP_EVENT_R_JOIN_IN;
    case MRP_IN:
        return MRP_EVENT_R_IN;
    case MRP_JOIN_MT:
        return MRP_EVENT_R_JOIN_MT;
    case MRP_MT:
        return MRP_EVENT_R_MT;
    case MRP_LV:
        return MRP_EVENT_R_LV;
    default:
        return MRP_EVENT_UNKOWN;
    }
}

/* Unpacks an event packed in a ThreePackeEvent.
   @param idx event index (0-2) */
static int unpack_tpe(uint8_t tpe, int idx)
{
    switch(idx) {
    case 0:
        return (tpe / 6) / 6;
    case 1:
        return (tpe / 6) % 6;
    case 2:
        return tpe % 6;
    default:
        return -1; // just to compile
    }
}

/* Packs an event into a ThreePackedEvent, according to its index
   @param tpe current value of the ThreePackedEvent
   @param idx index of the event (0, 1 or 2) */
static uint8_t pack_tpe(enum mrp_event event, uint8_t tpe, int idx)
{
    switch(idx) {
    case 2:
        return tpe + event;
    case 1:
        return tpe + (event * 6);
    case 0:
        return tpe + ((event * 6) * 6);
    default:
        return 255; // just to compile
    }
}

/* Parse MRP EndMark.
   @return 0 if no EndMark parsed. -1 if EndMark found and parsed */
static int mrp_pdu_parse_end_mark(struct mrpdu *pdu)
{
    if (!pdu_may_pull(pdu, MRP_END_MARK_LEN))
        return -1;
    if (((pdu->buf[pdu->pos] << 8) | pdu->buf[pdu->pos + 1] ) == MRP_END_MARK) {
        pdu_pull(pdu, MRP_END_MARK_LEN);
        return -1;
    }
    return 0;
}

/* Parse MRP Attribute.
   @param pdu (in) pointer to first octet of attribute to decode.
   @param attrype attribute type
   @param attrlen length of attribute first value */
static void mrp_pdu_parse_attr(struct mrp_participant *part,
                               struct mrpdu *pdu,
                               int attrtype,
                               int attrlen)
{
    int i, j;
    int leaveall;           // leaveall event value
    int nval;               // number of values (a.k.a. events)
    int e;                  // number of ThreePackedEvents
    int event;
    uint8_t tpe;            // ThreePackedEvent
    void *attrval;          // attr value
    void *nextval;          // next attr value
    struct mrp_attr *attr;

    /* Vector Header */
    leaveall = leaveall_event(pdu->buf[pdu->pos]);
    nval     = number_of_values(pdu->buf[pdu->pos], pdu->buf[pdu->pos + 1]);
    pdu_pull(pdu, MRP_ATTR_HDR_LEN);

    /* First Value */
    attrval = &pdu->buf[pdu->pos];
    pdu_pull(pdu, attrlen);

    /* If attribute type is not recognised, message events are not processed */
    if (attrtype > part->app->maxattr)
        goto discard;

    if (leaveall)
        mrp_attrtype_event(part, attrtype, MRP_EVENT_R_LA);

    /* Vector */
    e = ntpe(nval);
    nextval = malloc(attrlen);
    memcpy(nextval, attrval, attrlen);
    for (i = 0; i < e; i++) {
        tpe = pdu->buf[pdu->pos + i];
        for (j = 0; j < 3; j++, nval--) {
            if (nval > 0) {
                event = attr_event(unpack_tpe(tpe, j));
                if (event == MRP_EVENT_UNKOWN)
                    continue; /* Unknown events are silently discarded */
                /* Process event */
                attr = mrp_attr_lookup(part->mad, attrtype, attrlen, nextval);
                if (!attr) {
                    attr = mrp_attr_create(attrtype, attrlen, nextval);
                    if (!attr) // TODO handle NOMEM err
                        break;
                    mrp_attr_insert(&part->mad, attr);
                }
                mrp_attr_event(attr, event);
                part->app->nextval(attrtype, attrlen, nextval);
            }
        }
    }
    free(nextval);

discard:
    pdu_pull(pdu, e);
}

/* Parse MRP Message.
   @param pdu data to be unmarshalled. */
static void mrp_pdu_parse_msg(struct mrp_participant *part, struct mrpdu *pdu)
{
    int attrtype, attrlen;

    attrtype = pdu->buf[pdu->pos++];
    attrlen  = pdu->buf[pdu->pos++];
    while (pdu->len > pdu->pos) {
        mrp_pdu_parse_attr(part, pdu, attrtype, attrlen);
        if (mrp_pdu_parse_end_mark(pdu) < 0)
            break;
    }
}

/* Parse MRP PDU.
   @param pdu data to be unmarshalled. */
static void mrp_pdu_parse(struct mrp_participant *part, struct mrpdu *pdu)
{
    pdu->pos = 0;
    while(pdu->len > pdu->pos) {
        mrp_pdu_parse_msg(part, pdu);
        if (mrp_pdu_parse_end_mark(pdu) < 0)
            break;
    }
}

/* Check MRP PDU vector attribute format
   @return 0 if format is correct. -1 otherwise */
static int mrp_pdu_check_attr(struct mrpdu *pdu, int attrlen)
{
    int i;
    int nval;               // number of values (a.k.a. events)
    int e;                  // number of ThreePackedEvents

    /* Vector Attribute Header */
    if (!pdu_may_pull(pdu, MRP_ATTR_HDR_LEN)) {
        fprintf(stderr, "mrp: error parsing attr header: too short\n");
        return -1;
    }
    /* leaveall event (3 msbits): values other than 0 and 1 are reserved */
    if (leaveall_event(pdu->buf[pdu->pos]) > 1) {
        fprintf(stderr, "mrp: error parsing attr header: reserved leaveall\n");
        return -1;
    }
    /* The number of AttributeEvent values should be non-zero */
    nval = number_of_values(pdu->buf[pdu->pos], pdu->buf[pdu->pos + 1]);
    if (nval == 0) {
        fprintf(stderr, "mrp: error parsing attr header: num of values is 0\n");
        return -1;
    }
    pdu_pull(pdu, MRP_ATTR_HDR_LEN);

    /* First Value */
    if (!pdu_may_pull(pdu, attrlen)) {
        fprintf(stderr, "mrp: error parsing first value: too short\n");
        return -1;
    }
    pdu_pull(pdu, attrlen);

    /* Number of ThreePackedEvents */
    e = ntpe(nval);
    if (!pdu_may_pull(pdu, e)) {
        fprintf(stderr, "mrp: error parsing event vector: %d too short\n", e);
        return -1;
    }

    /* Vector */
    for (i = 0; i < e; i++) {
        if (pdu->buf[pdu->pos + i] > MAX_THREE_PACKED_EVENT_VAL) {
            fprintf(stderr, "mrp: error parsing event vector: reserved\n");
            return -1;
        }
    }
    pdu_pull(pdu, e);
    return 0;
}

/* Check MRP PDU message format.
   @return 0 if format is correct. -1 otherwise */
static int mrp_pdu_check_msg(struct mrpdu *pdu)
{
    int attrtype, attrlen;

    if (!pdu_may_pull(pdu, MRP_MSG_HDR_LEN)) {
        fprintf(stderr, "mrp: error parsing msg header: too short\n");
        return -1;
    }
    /* attrtype: 0 is reserved and should not be used by MRP applications */
    attrtype = pdu->buf[pdu->pos];
    if (attrtype == 0) {
        fprintf(stderr, "mrp: error parsing msg header: reserved attr type\n");
        return -1;
    }
    /* Attribute lenght should be a non-zero integer (See IEEE Corrigendum 1)*/
    attrlen = pdu->buf[pdu->pos + 1];
    if (attrlen == 0) {
        fprintf(stderr, "mrp: error parsing msg header: attr length is 0\n");
        return -1;
    }
    pdu_pull(pdu, MRP_MSG_HDR_LEN);
    while (pdu->len > pdu->pos) {
        if (mrp_pdu_check_attr(pdu, attrlen) < 0)
            return -1;
        if (mrp_pdu_parse_end_mark(pdu) < 0)
            break;
    }
    return 0;
}

/* Check MRP PDU format.
   @return 0 if format is correct. -1 otherwise */
static int mrp_pdu_check(struct mrpdu *pdu)
{
    pdu->pos = 0;
    /* Protocol Version */
    pdu_pull(pdu, 1);
    while(pdu->len > pdu->pos) {
        if (mrp_pdu_check_msg(pdu) < 0)
            return -1;
        if (mrp_pdu_parse_end_mark(pdu) < 0)
            break;
    }
    return 0;
}


/* Append MRP_ENDMARK at current encoding position.
   @return 0 if endmark was actually encoded. -1 if there was no room*/
int mrp_pdu_append_endmark(struct mrpdu *pdu)
{
    if (pdu_tailroom(pdu) < MRP_END_MARK_LEN)
        return -1;
    memcpy(pdu->buf + pdu->pos, &endmark, MRP_END_MARK_LEN);
    pdu->pos += MRP_END_MARK_LEN;
    return 0;
}

static int mrp_pdu_append_msg(struct mrpdu *pdu, int attrtype, int attrlen)
{
    if (pdu_tailroom(pdu) < MRP_MSG_HDR_LEN)
        return -1;
    /* AttributeType */
    pdu->buf[pdu->pos++] = attrtype;
    /* AttributeLength */
    pdu->buf[pdu->pos++] = attrlen;
    return 0;
}

static int mrp_pdu_append_vector_attr(struct mrpdu *pdu, int attrlen,
    void *firstval)
{
    /* VectorHeader is updated once the event has been actually added */
    if (pdu_tailroom(pdu) < MRP_ATTR_HDR_LEN)
        return -1;
    pdu->vhdr = pdu->pos;
    pdu->buf[pdu->pos++] = 0;
    pdu->buf[pdu->pos++] = 0;

    /* FirstValue. Lower octect number has the most significant value */
    if (pdu_tailroom(pdu) < attrlen)
        return -1;
    memcpy(pdu->buf + pdu->pos, firstval, attrlen);
    pdu->pos += attrlen;
    return 0;
}

/* Initialise PDU encoding and decoding info */
void mrp_pdu_init(struct mrpdu *pdu)
{
    memset(pdu->buf, 0, ETH_DATA_LEN);
    pdu->len = 0;
    pdu->pos = 0;

    pdu->cur_attr = NULL;
    pdu->vhdr = 0;
}

/* Read incoming MRP PDU.
   @return 0 if PDU was actually read. -1 if no data was available or PDU was
   discarded (v.g. due to incorrect format) */
int mrp_pdu_rcv(struct mrp_application *app)
{
    struct mrpdu pdu;          /* ethernet frame payload */
    struct sockaddr_ll sl;      /* stores sending address */
    socklen_t len = sizeof(sl); /* lenght of sockaddr structure */

    /* Receive PDU */
    pdu.len = recvfrom(app->proto.fd, &pdu.buf, sizeof(pdu.buf), 0,
        (struct sockaddr*)&sl, &len);
    if (pdu.len <= 0)
        return -1;

    /* Check PDU format */
    if (mrp_pdu_check(&pdu) < 0) {
        fprintf(stderr, "mrp: error parsing pdu at octet %d\n", pdu.pos);
        return -1;
    }

    // app->rcv(port, pdu);

    /* Parse PDU */
    mrp_pdu_parse(&app->participants[sl.sll_ifindex], &pdu);

    return 0;
}

/* Send the MRPDU for the given participant.
   @param part pointer to MRP participant */
void mrp_pdu_send(struct mrp_participant *part)
{
    int len;
    struct mrpdu *pdu = &part->pdu;
    struct mrp_application *app = part->app;

    static struct sockaddr_ll sl = {    // link level info
        .sll_family = PF_PACKET,
        .sll_halen  = ETH_ALEN
    };

    sl.sll_ifindex  = mrp_port(part);
    sl.sll_protocol = htons(app->proto.ethertype);
    memcpy(&sl.sll_addr, app->proto.address, ETH_ALEN);

    /* end message and pdu (not strictly necessary)*/
    mrp_pdu_append_endmark(pdu);
    mrp_pdu_append_endmark(pdu);

    /* Ensure minimum ethernet payload */
    pdu->len = (pdu->len < MIN_ETH_DATA_LEN) ? MIN_ETH_DATA_LEN:pdu->len;

    len = sendto(app->proto.fd, pdu->buf, pdu->len, 0,
        (struct sockaddr*)&sl, sizeof(sl));
    if (len < pdu->len)
        perror("mrp_pdu_send");
}

/*
   Append and MRP attribute event into a participant PDU.
   @param part participant which contains PDU into which attribute should be
   appended.
   @param attr attribute to which the event refers.
   @param event attribute event. In the case of a LEAVEALL event, the event is
   added to the VectorAttribute being currently encoded.
   @return 0 if attribute event was correctly appended. -1 if there is no space
   left to insert the attribute event */
int mrp_pdu_append_attr(struct mrp_participant *part,  struct mrp_attr *attr,
    enum mrp_event event)
{
    struct mrp_attr *cur_attr;
    struct mrpdu *pdu;
    int rb_pos, rb_vhdr;        // initial encoding positions (rollback)
    int nval;                   // number of values vector attr header
    int idx;                    // Index of event within ThreePackedEvent
    uint16_t vhdr;              // Vector header

    pdu = &part->pdu;
    cur_attr = pdu->cur_attr;

    /* If first attribute... */
    if (!cur_attr) {
        /* Init PDU */
        mrp_pdu_init(pdu);
        pdu->buf[pdu->pos++] = part->app->proto.version;
        cur_attr = attr;
    }

    rb_pos  = pdu->pos;
    rb_vhdr = pdu->vhdr;

    /* If new attribute type... */
    if (attr->type != cur_attr->type) {
        /* Close previous message and append a new one*/
        if (mrp_pdu_append_endmark(pdu) < 0)
            goto nomem;
        if (mrp_pdu_append_msg(pdu, attr->type, attr->len) < 0)
            goto nomem;
        cur_attr = attr;
    }
    /* If attribute value is not the following one... */
    if (!mrp_attr_subsequent(part->app, cur_attr->value,
        attr->value, attr->type, attr->len)) {
        /* Append new vector attribute. */
        if(mrp_pdu_append_vector_attr(pdu, attr->len, attr->value) < 0)
            goto nomem;
    }

    nval = number_of_values(pdu->buf[pdu->vhdr], pdu->buf[pdu->vhdr + 1]);
    /* Append event in the corresponding ThreePackedEvent */
    idx  = nval % 3;
    if (idx == 0) {
        /* Append new octet to the Vector */
        if (!pdu_may_pull(pdu, 1))
            goto nomem;
        pdu->pos++;
        pdu->buf[pdu->pos] = 0; /* initial value for the tpe */
    }
    pdu->buf[pdu->pos] = pack_tpe(event, pdu->buf[pdu->pos], idx);
    /* note: do _not_ update encoding pos after adding the packed event! */
    nval++;
    /* Update Vector Header */
    vhdr = htons((part->leaveall * 8192) + nval);
    memcpy(pdu->buf + pdu->vhdr, &vhdr, MRP_ATTR_HDR_LEN);
    /* no need to update encoding position, since we are just updating */

    pdu->cur_attr = cur_attr;
    return 0;

nomem:
    /* rollback pdu changes */
    pdu->pos  = rb_pos;
    pdu->vhdr = rb_vhdr;

    /* prepare pdu for delivery */
    mrp_pdu_append_endmark(pdu); /* message end */
    mrp_pdu_append_endmark(pdu); /* pdu end */
    pdu->len = pdu->pos;
    return -1;
}

/* Returns 1 if the MRPDU is full */
int mrp_pdu_full(struct mrpdu *pdu)
{
    return pdu_tailroom(pdu) == 0;
}
