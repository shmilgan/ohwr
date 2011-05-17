
#include	"define.h"
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdint.h>
#include    <stdbool.h>
#include    <math.h>
#include    <string.h>


/*
 * =====================================================================================
 *
 *       Filename:  hamming.h
 *
 *    Description:  header of hamming code
 *
 *        Version:  1.0
 *        Created:  03/17/2011 07:13:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Cesar Prados Boda (cp), c.prados@gsi.de
 *        Company:  GSI
 *
 * =====================================================================================
 */
typedef struct {
    uint32_t   *pvector;
}hamming_vector;

typedef struct {
    uint32_t   *mask;
}filter_vector;

uint32_t    *hamming_encoder (uint32_t *, uint16_t );
uint32_t    *hamming_decoder(uint32_t *, uint16_t);
uint8_t     parity_check(uint32_t * , uint32_t *,uint16_t, uint16_t );
uint16_t    bitcount(uint16_t);
uint16_t    find_num_parity_bits(uint16_t ); 
hamming_vector *vector_generator(uint16_t);
uint16_t    generator(hamming_vector *, uint16_t);
uint16_t    dGenerator(hamming_vector *, uint16_t);
uint16_t    hamming_init(uint16_t, uint16_t );
uint16_t    vector_length(uint16_t);
uint16_t    payload_length(uint16_t);
uint16_t    func_malloc(hamming_vector ** , uint32_t, uint32_t);
uint16_t    filter_number(uint16_t);
uint16_t    random_error(uint32_t *);
uint16_t    filter_generator(hamming_vector *,uint16_t );
uint16_t    dFilter_generator(hamming_vector *,uint16_t );



