/*
 * White Rabbit MRP (Multiple Registration Protocol)
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Encoding/decoding of MRP PDUs.
 *
 * Fixes:
 *              Alessandro Rubini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "mrp.h"

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
    return pdu->maxlen - pdu->pos;
}

inline static int leaveall_event(uint8_t octet)
{
    return octet >> 5; // 3 bits
}

inline static int number_of_values(uint8_t high, uint8_t low)
{
    return ((high & 0x1f) << 8) | low; //13 bits
}

inline static int pdu_nval(struct mrpdu *pdu, int vhdr)
{
    return number_of_values(pdu->buf[vhdr], pdu->buf[vhdr + 1]);
}

/* Returns number of ThreePackedEvents in a VectorAttribute
   @param nval number of events in encoded in VectorAttribute */
inline static int ntpe(int nval)
{
    return (nval + 2) / 3;
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
static int unpack_tpe(int tpe, int idx)
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
static uint8_t pack_tpe(enum mrp_attr_event event, int tpe, int idx)
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
    int i, j, event;
    int leaveall;           // leaveall event value
    int nval;               // number of values (a.k.a. events)
    int offset;             // event offset within vector
    int e;                  // number of ThreePackedEvents
    int mid;                // MAD attribute identifier
    uint8_t tpe;            // ThreePackedEvent
    void *firstval;         // first attr value
    struct mrp_application *app = part->port->app;

    /* Vector Header */
    leaveall = leaveall_event(pdu->buf[pdu->pos]);
    nval = pdu_nval(pdu, pdu->pos);
    pdu_pull(pdu, MRP_ATTR_HDR_LEN);

    /* First Value */
    firstval = &pdu->buf[pdu->pos];
    pdu_pull(pdu, attrlen);

    /* If attribute type is not recognised, message events are not processed */
    e = ntpe(nval);
    if (attrtype > app->maxattr)
        goto discard;

    if (leaveall) {
        mad_attrtype_event(part, attrtype, MRP_EVENT_R_LA);
        mad_participant_event(part, MRP_EVENT_R_LA);
    }

    /* Vector */
    for (i = 0, offset = 0; i < e; i++) {
        tpe = pdu->buf[pdu->pos + i];
        for (j = 0; j < 3; j++, offset++) {
            if (offset < nval) {
                /* Process event */
                event = mrp_event(unpack_tpe(tpe, j));
                fprintf(stderr, "mrp_pdu_parse_attr() - event=%d\n", event);
                
                if (event == MRP_EVENT_UNKOWN)
                    continue; /* Unknown events are silently discarded */
                mid = app->db_find_entry(attrtype, attrlen, firstval, offset);
                if (mid < 0) {
                    mid = app->db_add_entry(attrtype, attrlen, firstval, offset);
                    if ((mid < 0) || (mid >= app->numattr))
                        continue; // TODO handle nomem error
                }
                mad_attr_event(part, mid, event);
            }
        }
    }

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
    pdu->pos = 1;
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
    nval = pdu_nval(pdu, pdu->pos);
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
   @return 0 if endmark was actually encoded. -1 if there was no room */
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
    pdu->mhdr = pdu->pos;
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
void mrp_pdu_init(struct mrpdu *pdu, int tagged)
{
    pdu->len = 0;
    pdu->pos = 0;
    pdu->mhdr = 0;
    pdu->vhdr = 0;
    pdu->maxlen = tagged ? VLAN_ETH_DATA_LEN:ETH_DATA_LEN;
}

/* Read incoming MRP PDU.
   @return 0 if PDU was actually read. -1 if no data was available or PDU was
   discarded (v.g. due to incorrect format) */
int mrp_pdu_rcv(struct mrp_application *app)
{
    struct mrpdu pdu;               /* ethernet frame payload */
    struct mrp_participant *part;

    /* PDU should be processed by the participant associated the port
      (and vlan, if application requires tagged PDUs) */
    /* Locate participant. If found, its pdu.buf gets filled with data */
    part = mrp_socket_rcv(app, &pdu);
    if (!part)
        return -1;

    /* Check PDU format */
    if (mrp_pdu_check(&pdu) < 0) {
        fprintf(stderr, "mrp: error parsing pdu at octet %d\n", pdu.pos);
        return -1;
    }
    
    /* Parse PDU */
    mrp_pdu_parse(part, &pdu);
    return 0;
}

/* Send the MRPDU for the given participant.
   @param p pointer to MRP participant */
void mrp_pdu_send(struct mrp_participant *p)
{
    struct mrpdu *pdu = &p->pdu;

    /* end message and pdu (not strictly necessary)*/
    mrp_pdu_append_endmark(pdu);
    mrp_pdu_append_endmark(pdu);

    /* Ensure minimum ethernet payload */
    pdu->len = (pdu->len < MIN_ETH_DATA_LEN) ? MIN_ETH_DATA_LEN : pdu->pos;

    /* Send PDU through the open socket */
    mrp_socket_send(p);
}

/*
   Append and MRP attribute event into a participant PDU.
   @param part participant which contains PDU into which attribute should be
   appended.
   @param mid attribute index to which the event refers.
   @param event attribute event. In the case of a LEAVEALL event, the event is
   added to the VectorAttribute being currently encoded.
   @return 0 if attribute event was correctly appended. -1 if there is no space
   left to insert the attribute event. -EINVAL if invalid attribute */
int mrp_pdu_append_attr(struct mrp_participant *part,
                        int mid,
                        enum mrp_attr_event event)
{
    int new_pdu, new_msg, new_vec;
    int rb_pos, rb_vhdr, rb_mhdr;   // initial encoding positions (rollback)
    int nval;                       // number of values (vector header)
    int idx;                        // Index of event within ThreePackedEvent
    uint16_t vhdr;                  // Vector header
    uint8_t __attrtype;             // current message attr type
    void *firstval;
    uint8_t attrtype;
    uint8_t attrlen;
    void *attrval;
    struct mrp_application *app = part->port->app;
    struct mrpdu *pdu = &part->pdu;

    rb_pos  = pdu->pos;
    rb_vhdr = pdu->vhdr;
    rb_mhdr = pdu->mhdr;

    /* Obtain attr info from application */
    if (app->db_read_entry(&attrtype, &attrlen, &attrval, mid) < 0)
        /* should never happen, but... just in case, avoid processing further
           events for this attr. To recover from error, close and send PDU */
        goto nomem;

    /* If first attribute... */
    new_pdu = (pdu->pos == 0);
    if (new_pdu)
        pdu->buf[pdu->pos++] = app->proto.version;
    else
        __attrtype = pdu->buf[pdu->mhdr];

    /* If new attribute type... */
    new_msg = new_pdu || (attrtype != __attrtype);
    if (new_msg) {
        /* Close previous message (if exists) and append a new one */
        if (!new_pdu) {
            if (mrp_pdu_append_endmark(pdu) < 0)
                goto nomem;
        }
        if (mrp_pdu_append_msg(pdu, attrtype, attrlen) < 0)
            goto nomem;
    } else {
        nval = pdu_nval(pdu, pdu->vhdr);
        firstval = &pdu->buf[pdu->vhdr + MRP_ATTR_HDR_LEN];
    }

    /* If attribute value is not the following one in vector... */
    new_vec = new_msg ||
           (app->attr_cmp(attrtype, attrlen, attrval, firstval) != (nval + 1));
    if (new_vec) {
        /* Append new vector attribute. */
        if(mrp_pdu_append_vector_attr(pdu, attrlen, attrval) < 0)
            goto nomem;
        nval = 0;
    }

    /* Append event in the corresponding ThreePackedEvent */
    idx  = nval % 3;
    if (idx == 0) {
        /* Append new octet to the Vector */
        if (pdu_tailroom(pdu) < 1)
            goto nomem;
        pdu->buf[pdu->pos++] = 0; /* initial value for the tpe */
    }
    pdu->buf[pdu->pos - 1] = pack_tpe(event, pdu->buf[pdu->pos], idx);
    /* note: do _not_ update encoding pos after adding the packed event! */
    nval++;
    /* Update Vector Header */
    vhdr = htons((part->leaveall * 8192) + nval);
    memcpy(pdu->buf + pdu->vhdr, &vhdr, MRP_ATTR_HDR_LEN);
    /* no need to update encoding position, since we are just updating */

    return 0;

nomem:
    /* rollback pdu changes */
    pdu->pos  = rb_pos;
    pdu->vhdr = rb_vhdr;
    pdu->mhdr = rb_mhdr;

    return -1;
}

/* Returns 1 if the MRPDU is full */
int mrp_pdu_full(struct mrpdu *pdu)
{
    return pdu_tailroom(pdu) == 0;
}

int mrp_pdu_empty(struct mrpdu *pdu)
{
    return pdu_tailroom(pdu) == pdu->maxlen;
}
