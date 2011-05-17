#include	"hamming.h"

/* =================================================================
 *
 *       Filename:  hamming.c
 *
 *    Description:  main functions that creates a 
 *
 *        Version:  1.0 Created:  03/17/2011 07:12:29 PM Revision:  none
 *        Compiler:  gcc
 *
 *         Author:  César Prados Boda (cp), c.prados@gsi.de Company:  GSI
 *
 * ==================================================================
 */

uint32_t *hamming_encoder (uint32_t *payload, uint16_t length)
{
    uint8_t num_parity_bits;
    hamming_vector *parity_check_vector;
    hamming_vector *filter;
    uint32_t *dPayload;
    uint32_t i,j;
    uint16_t vector_l;
    uint16_t filter_n, numero_filt;
    uint32_t distance2parityBits, parity_bit_pos;
    uint8_t  parity_bit; 
    uint16_t wordsINpayload; //words of 32 bits
    uint32_t n_int,b_int, n_int_p;
	

    dPayload = (uint32_t *)malloc(length*sizeof(uint32_t));

    // number of parity bits for a given word of length l
    num_parity_bits = hamming_init(0,READ);
    dbg("NUM PARITY BITS %d \n",num_parity_bits);

    // number of word in the vector
    vector_l = vector_length(num_parity_bits);
   
    // number of filter for a given rate
    filter_n = filter_number(num_parity_bits);

    // gets the parity vector
    parity_check_vector = vector_generator(READ_VECTOR);

    // gets the filter vector
    filter = vector_generator(READ_FILTER);
    
    // gets the length of the word of the payload
    wordsINpayload = (LOG_2_32 >> length) +1;

    for(i=0;i<num_parity_bits;i++)
    {   

        dbg("***** Vector for Parity Bit %d ***** \n",i);

        for(j=0;j<=(LOG_2_32 >> num_parity_bits);j++)
        {
               printf("%x",*((parity_check_vector+i)->pvector+j));
        }
        printf("\n");
    }

    dbg("%d parity bits for a given string %d bits \n ",num_parity_bits,length);


    distance2parityBits = 0;

    numero_filt=0;

/**************************************** PARITY BITS **********************************/
    for(i=0;i<num_parity_bits;i++)
    {
       parity_bit=parity_check(payload, (parity_check_vector+i)->pvector,length,vector_l);
       
       parity_bit_pos = (BIT_MASK << i); 
       //n_int = LOG_2_32  >> (BIT_MASK << i); // Int division of Number / 32

       n_int_p = (int) parity_bit_pos / BITS_VECTOR;

       *(dPayload + n_int_p) ^= (BIT_MASK & parity_bit) << (parity_bit_pos-1);//adds parity bit to the positions 0,1,2,4,8,16 etc...


      
/**************************************** FRAMES BITS **********************************/

       if(i>0) // the bit 0 is out of this counts since the p2 is consecutive to p1 and doesn't
               // follow the algorithm.
       {
           //calculates the distance between the current parity bit and the next
           //to insert the original bits in the payload

           distance2parityBits += (BIT_MASK << (i-1) )-1;  // 0, 1, 3 etc       

           //n_int = LOG_2_32 >> distance2parityBits;
           n_int = (int) distance2parityBits / BITS_VECTOR;

           //b_int = distance2parityBits & MOD_BIT_32;
           b_int = distance2parityBits %  BITS_VECTOR;
     
                for(j=0;(j<= n_int) && (numero_filt <= filter_n) ; j++)
                {
                    dbg("Distance %d pvector  %x payload %x  \n",distance2parityBits,*(filter+numero_filt)->pvector+j,*(payload+j));
                    *(dPayload + n_int_p) |= ((*(payload+n_int) & *((filter+numero_filt)->pvector+j)) >> (b_int)) << (BIT_MASK << i);  
                    numero_filt++;
                }
       }

    }

/******************************************************************************************/


    for(i=0;i<payload_length(num_parity_bits);i++)
    {
         printf("%d ", (*dPayload >> i ) & 0x1);
    }
    printf("\n");

    for(i=0;i<payload_length(num_parity_bits);i++)
    {
         printf("%d ", (*payload >> i ) & 0x1);
    }
    printf("\n");


    return dPayload; 
}		/* -----  end of function h amming_encoder  ----- */

/*
 *===================================================================
 * parity_check
 * input  --> uint_8t pointer to payload, uint16_t parity word 
 * output --> uint8_t parity of the word
 * ==================================================================
 * */
uint8_t parity_check(uint32_t *payload,uint32_t *parity_check_mask, uint16_t length, uint16_t vector_l)
{
    uint16_t parity=0;
    uint32_t i;

    // the payload is AND against the mask of the parity bit
    // from this result the numbers of bits in the string is counted
    // if it's ODD --> 1 parity bit 
    // if it's EVEN --> 0 parity bit
    // the word is divided in code word of 64 bits
    //payload 
    //

    for(i=0; i <= vector_l; i++)
        parity ^= (0x1 & bitcount(*(parity_check_mask+i) & (*payload+i))) ? 1  : 0;

//    dbg("Result Parity %d \n",parity);

//    dbg("FAST PARITY %d \n", parity);

    return parity;
}


uint16_t find_num_parity_bits(uint16_t length_word)
{
    uint16_t num_parity_bits = 0;

dbg("LENGTH %d \n", length_word);
    if(length_word<=4)
        num_parity_bits = 3;
    else if(length_word<=11)
        num_parity_bits = 4;
    else if(length_word<=32)
        num_parity_bits = 5;

    else if(length_word<=64)
        num_parity_bits = 6;

    else if(length_word<=128)
        num_parity_bits = 7;

    else if(length_word<=336)
        num_parity_bits = 8;

    else if(length_word<=672)
        num_parity_bits = 9;

    else if(length_word<=1344)
        num_parity_bits = 10;

    else if(length_word<=2688)
        num_parity_bits = 11;

    else if(length_word<=5376)
        num_parity_bits = 12;

    else if(length_word<=10752)
        num_parity_bits = 13;
    else
        num_parity_bits = 14;


    return num_parity_bits;
}
/*
 *===================================================================
 * bitcout
 * input 
 * output
  * ==================================================================
 * */


uint16_t bitcount(uint16_t n)
{
    uint16_t uCount;
    uCount = n
           - ((n >> 1) & 033333333333)
           - ((n >> 2) & 011111111111);
   return (uint16_t) (((uCount + (uCount >> 3))& 030707070707) % 63);
}


/*
 *===================================================================
 * hamming_decoder
 * input 
 * output
  * ==================================================================
* */

uint32_t *hamming_decoder (uint32_t *payload, uint16_t length)
{
    uint8_t num_parity_bits;
    hamming_vector *parity_check_vector;
    hamming_vector *filter;
    uint32_t *ePayload;
    uint32_t i,j;
    uint16_t vector_l;
    uint16_t filter_n, numero_filt;
    uint32_t distance2parityBits, parity_bit_pos;
    uint32_t  parity_bit_rx,parity_bit; 
    uint32_t n_int,b_int,error_check;
	

    ePayload = (uint32_t *)malloc(length*sizeof(uint32_t));

    parity_bit = (uint32_t *)malloc(num_parity_bits*sizeof(uint32_t));

    parity_bit_rx = (uint32_t *)malloc(num_parity_bits*sizeof(uint32_t));

    // number of parity bits for a given word of length l
    num_parity_bits = hamming_init(0,READ);
    dbg("NUM PARITY BITS IN ENCODED PACKET %d \n",num_parity_bits);

    // number of word in the vector
    vector_l = vector_length(num_parity_bits);
   
    // number of filter for a given rate
    filter_n = filter_number(num_parity_bits);

    // gets the parity vector
    parity_check_vector = vector_generator(READ_VECTOR);

    filter = vector_generator(READ_DFILTER);
     
    numero_filt = 0;
    parity_bit_rx = 0;
    parity_bit = 0;

//******************************************PARITY BITS RECEIVED ***********************/
    parity_bit_rx ^=  *(payload) & BIT_MASK ;

    for(i=1;i<num_parity_bits;i++)
    {
        
        n_int = (BIT_MASK << i) / BITS_VECTOR; //todo change to the faster bitwise operation 
        b_int = (BIT_MASK << i) & MOD_BIT_32;  // 2,4,8 ...
     
   
        parity_bit_rx ^= ((*(payload + n_int) & (BIT_MASK << b_int-1)) >> (b_int - 1) ) << i;

/**************************************** FRAMES BITS **********************************/

        *(ePayload + n_int) |= ((*(payload+n_int) & *((filter+numero_filt)->pvector+n_int))>>(i+1));  
//        dbg("Filter %d i %d payload %x \n",*((filter+numero_filt)->pvector+n_int), (i+1),*(payload+n_int));
        numero_filt++;  

    }

/********************************************PARITY BITS*********************/
    
    for(i=0;i<num_parity_bits;i++)
    {
       parity_bit  ^= parity_check(ePayload, (parity_check_vector+i)->pvector,length,vector_l) << i;
    }

/******************************************** CHECK THE PARITY BITS AND FIX***********/
    
    dbg("PARIT BIT RX %x PARITY BIT %x \n", parity_bit, parity_bit_rx);
    
    error_check = parity_bit ^ parity_bit_rx;




    dbg("CHECK %d \n",error_check);

    if(error_check != 0)
    {
        n_int = error_check / BITS_VECTOR;
        b_int = error_check & BITS_VECTOR;
        
//        *(ePayload+n_int) ^= (BIT_MASK << b_int); // Fix the bit 

    }
        

    for(i=0;i<payload_length(num_parity_bits);i++)
    {
         printf("%d ", (*ePayload >> i ) & 0x1);
    }
    printf("\n");

    return ePayload; 
   
}		/* -----  end of function hamming_decoder  ----- */

/* 
 * ===  FUNCTION  ====================================================
 *         Name:  vector genarator
 *  Description:  creates the vectors of the hamming matrix
 *                and the filter for the encoded word.
 *                in -> 
 *                      code_rate -> value of the code rate
 *                      multi_tag -> 1, write 0, value 
 *                out -> NULL in case of error, and if in:
 *                       WRITE        hamming vector
 *                       READ_VECTOR  hamming vector
 *                       FREE         NULL
 *                       READ_FILTER  filter vector                  
 * =================================================================
 * */

hamming_vector *vector_generator(uint16_t  multitag)
{
    static hamming_vector  *vector; //Pointer to array of pointer, that points to the parity check vector for encoding
    static hamming_vector  *filter; //Pointer to array of poiinter, that points to the filter vectors
    static hamming_vector  *dFilter; //Pointer to array of poiinter, that points to the filter vectors
    static hamming_vector  *dVector; //Pointer to array of pointer, that points to the parity check vector for decoding
    uint16_t code_rate;
    uint16_t vector_l;
    uint16_t payload_l;
    uint16_t filter_n;
    uint16_t i;

    // gets the code rate setted 
    code_rate = hamming_init(0,READ);
    // gets the length of the parity check vector for a given code_rate
    vector_l = vector_length(code_rate);
    // gets the length of the maximum payload for a given code_rate
    payload_l =  payload_length(code_rate);
    // gets the number of filter needed for a given code rate
    filter_n = filter_number(code_rate);

    if(multitag == WRITE) //if write
    {     

         // allocation of code_rate  PARITY CHECK VECTORS vector_l length
        if( func_malloc(&vector,code_rate,vector_l))
           return NULL;

        //allocation of N filter vector     
        if(func_malloc(&filter,filter_n,vector_l))  //TODO calculate the length of the vector correctly 
           return NULL;
        //allocation of N filter vector     
        if(func_malloc(&dFilter,filter_n,vector_l))  //TODO calculate the length of the vector correctly 
           return NULL;
        //allocation of N filter vector     
        //allocation of N filter vector     
        if(func_malloc(&dVector,code_rate,vector_l))  //TODO calculate the length of the vector correctly 
           return NULL;

        // The vector are calculated 
        if(generator(vector,payload_l)) 
        {
            dbg("Error Hamming Vector Encoder");
            return NULL;
        }

        dbg("Vector generated \n");

        // the filters are calculated
        if(filter_generator(filter,filter_n))
        {
            dbg("Error Filter Vector");
            return NULL;
        }
        // the filters are calculated
        if(dFilter_generator(dFilter,filter_n))
        {
            dbg("Error Filter Vector");
            return NULL;
        }

        // the vector for decoding is calculated
        if(dGenerator(dVector,payload_l))
        {
            dbg("Error Hamming Vector Decoder");
            return NULL;
        }
       
        return vector;

    }else if(multitag == CLEAN) //if clean
    {
        dbg("Pointer Vector freeded");
        free(vector);
        return NULL;

    }else if(multitag == READ_VECTOR)
    {

        return vector;

    }else if(multitag == READ_FILTER)
    {

        return filter;

    }else if(multitag == READ_DFILTER)
    {
        return dFilter;
    }
    else if(multitag == READ_DVECTOR)
    {
        return dVector;
    }

    return NULL;
}

/* 
 * ===  FUNCTION  ====================================================
 *  Name:  generator
 *  Description:  creates the vector
 *                in -> hamming_vector *
 *                       
 *                out -> hamming_vector *
 * =================================================================
 * */
uint16_t dFilter_generator(hamming_vector *filter,uint16_t filter_n )
{
    uint32_t i, distance=1,distance_r, j,bit_shift;
    uint16_t filter_number=0;
    bool power_2;


    for(i=3;(i<MAX_LENGTH_PAYLOAD)&&(filter_number<filter_n);i++)
    {
        power_2 = ((i & (i - 1)) == 0);
        
        if(power_2)
        {    
            filter_number++;
        }
        else
        {
            // position of the bit in uint32_t type 
            // bit_shift = (uint32_t)(i / BITS_VECTOR);
            bit_shift = LOG_2_32 >> i;
            // Modulo of a division in bitwise, X % Y == X & (Y-1)
            distance_r = i & MOD_BIT_32;

            *((filter+(filter_number))->pvector+bit_shift) ^= (BIT_MASK << distance_r-1);
 
        }

    }
    for(i=0;i<filter_number;i++) 
    {   

        dbg("***** Vector for Filter Deco %d ***** \n",i);

        for(j=0;j<= (int)(j/BITS_VECTOR) ;j++)
        {
               printf("%x",*((filter+i)->pvector+j));
        }
        printf("\n");

    }

return OK;
}
/* 
 * ===  FUNCTION  ====================================================
 *  Name:  generator
 *  Description:  creates the vector
 *                in -> hamming_vector *
 *                       
 *                out -> hamming_vector *
 * =================================================================
 * */
uint16_t filter_generator(hamming_vector *filter,uint16_t filter_n )
{
    uint32_t i, distance=1,distance_r, j,bit_shift;
    uint16_t filter_number=0;
    bool power_2;


    for(i=3;(i<MAX_LENGTH_PAYLOAD)&&(filter_number<filter_n);i++)
    {
        power_2 = ((i & (i - 1)) == 0);
        
        if(power_2)
        {    
            filter_number++;
        }
        else
        {
            // position of the bit in uint32_t type 
            // bit_shift = (uint32_t)(i / BITS_VECTOR);
            bit_shift = LOG_2_32 >> i;
            // Modulo of a division in bitwise, X % Y == X & (Y-1)
            distance_r = distance & MOD_BIT_32;

            *((filter+(filter_number))->pvector+bit_shift) ^= (BIT_MASK << distance_r-1);
 
           // *((filter+(filter_number))->pvector+bit_shift) ^= (BIT_MASK << (i-1));
            distance++;
        }

    }
    for(i=0;i<filter_number;i++) 
    {   

        dbg("***** Vector for Filter %d ***** \n",i);

        for(j=0;j<= (int)(j/BITS_VECTOR) ;j++)
        {
               printf("%x",*((filter+i)->pvector+j));
        }
        printf("\n");

    }

return OK;
}
/* 
 * * ===  FUNCTION  ====================================================
 *  Name:  generator
 *  Description:  creates the vector to encode a word
 *                in -> hamming_vector *
 *                       
 *                out -> hamming_vector *
 * =================================================================
 * */

uint16_t generator(hamming_vector *vector, uint16_t payload_l)
{
    int index_matrix,position_bit;
    int i,j,parity_bits=0;
    int index_n,bit_shift;
    bool power_2;

    for(i=1 ; i<= payload_l ; i++)
    {
        index_matrix = 0;
        //check if the bit position is power of 2, bitwise 
        power_2 = ((i & (i - 1)) == 0);
        
        if(power_2)
        {    
            // calculation of the first bit of the bit position
            position_bit = ffs(i)-1;

            for(j=1;j<=payload_l ;j++)
            {
                // if the bit position j has a bit in the bit position i is a
                // parity check
                if((0x1 & (j>>position_bit)) && !((j & (j - 1))==0))
                {
                    // position of the bit in uint32_t type 
                    bit_shift = index_matrix % BITS_VECTOR;
                    //bit_shift = index_matrix & MOD_BIT_32;

                    // position in the array of uint32_t words
                    index_n = (int)(index_matrix/BITS_VECTOR);                     
                    //index_n = LOG_2_32 >> index_matrix;  
 
                    *((vector+parity_bits)->pvector+index_n) ^= (BIT_MASK << bit_shift);
                    index_matrix++; // keep the track of the position in the vector/matrix
                }
                else if(!((j & (j - 1))==0) && !(0x1 & (j>>position_bit))  )
                {
                    index_matrix++;
                }
            }
            
            parity_bits++; // keep the track of the number of parity check bit generated
        }

    }

    for(i=0;i<parity_bits;i++)
    {   

        dbg("***** Vector for Parity Bit %d ***** \n",i);

        for(j=2;j>=0;j--)//   (int)(index_matrix/BITS_VECTOR);j++)
        {
               printf("%x",*((vector+i)->pvector+j));
        }
        printf("\n");

    }
    
return OK;

}


/* 
 * ===  FUNCTION  ====================================================
 *  Name:  dGenerator
 *  Description:  creates the vector to decode a word
 *                in -> hamming_vector *
 *                       
 *                out -> hamming_vector *
 * =================================================================
 * */

uint16_t dGenerator(hamming_vector *dVector, uint16_t payload_l)
{
    uint32_t position_bit;
    uint32_t i,j,b,parity_bits=0;
    uint32_t index_n,bit_shift;
    bool power_2;

    for(i=1 ; i<= payload_l ; i++)
    {
        
        //check if the bit position is power of 2, bitwise 
        power_2 = ((i & (i - 1)) == 0);
    
        if(power_2)
        {    
           // calculation of the first bit of the bit position
           position_bit = ffs(i)-1;


            for(j=i+1;j<payload_l ;j++)
            {
                  
                power_2 = ((j & (j - 1))==0 );

                // if the bit position j has a bit in the bit position i is a
                // parity check
                if((0x1 & (j>>position_bit)) && !(power_2))
                {
  
                    // position of the bit in uint32_t type 
                    bit_shift = j % BITS_VECTOR;

                    //bit_shift = index_matrix & MOD_BIT_32;

                    // position in the array of uint32_t words
                    index_n = (int)(j/BITS_VECTOR);                     
                    //index_n = LOG_2_32 >> index_matrix;  
 
                     //printf("***Bit  %d  bit_shift %d j %d \n ",i,bit_shift,j);
                     *((dVector+parity_bits)->pvector+index_n) ^= (BIT_MASK << (bit_shift-1));

                    // *((dVector+parity_bits)->pvector+index_n) ^= 0;
                }
            }
            
            parity_bits++; // keep the track of the number of parity check bit generated
        }

    }

    for(b=0;b<parity_bits;b++)
    {   

        dbg("***** Vector for Parity Decoder Bit %d ***** \n",b);

        for(j=0;j<1;j++)
        {
               printf("%x",*((dVector+b)->pvector+j));
        }
        printf("\n");

    }
    
return OK;

}




/* 
 * ===  FUNCTION  ====================================================
 *         Name:  func_allocate for the structure hamming_vector
 *  Description:  set/return the value of the code write
 *                in -> 
 *                      pointer and length to allocate 
 *                out -> 0-> OK 1-> Error    
 * =================================================================
 * */
uint16_t func_malloc(hamming_vector **vector,uint32_t  row, uint32_t column)
{

    uint32_t i;
    if((*vector = (hamming_vector *)malloc(row*sizeof(hamming_vector)))==NULL)
    {
        dbg("Error malloc vector \n");
        return ERROR;
    }

    // for every PARITY CHECK VECTOR FRAME_N bytes
    for(i=0;i<row;i++)
    {
        if(((*vector+i)->pvector = (uint32_t *)malloc(column*sizeof(uint32_t)))==NULL)
        {
            dbg("Error Malloc vector of vector");
            return ERROR;
        }
    }

    //memset(&*vector,'\0',sizeof(hamming_vector)*row);

return OK;
}
/* 
 * ===  FUNCTION  ====================================================
 *         Name:  hamming_int
 *  Description:  set/return the value of the code write
 *                in -> 
 *                      conf_code_rate -> value of the code rate
 *                      write_read     -> 1, write 0, value 
 *                out -> code rate
 * =================================================================
 * */
uint16_t hamming_init(uint16_t conf_code_rate,uint16_t write_read)
{
   static uint16_t code_rate;

   if(write_read)
    code_rate = conf_code_rate;

   return code_rate;
}
/* 
 * ===  FUNCTION  ====================================================
 *         Name: vector_length 
 *  Description:  return the length of the parity check vector for a given
 *                code rate, note it's a uint32_t vector, indeed return the
 *                number of uint32_t
 *                in -> 
 *                      code_rate -> value of the code rate
 *                out -> length of the vector
 * =================================================================
 * */

uint16_t filter_number(uint16_t code_rate)
{
    uint16_t filter_n;
 

    if(code_rate == RATE_7_4)
       filter_n = 2; 
    else if(code_rate == RATE_15_11) 
       filter_n = 3;
    else if(code_rate == RATE_31_26)
       filter_n = 4; 
    else if(code_rate == RATE_63_57) 
       filter_n = 5;
    else if(code_rate == RATE_127_120) 
        filter_n = 6;
    else if(code_rate == RATE_255_247) 
        filter_n = 7;
    else if(code_rate == RATE_1023_1013) 
        filter_n = 8;
    else if(code_rate == RATE_2047_2036) 
        filter_n = 8;
    else if(code_rate == RATE_4095_4083)
        filter_n = 10;
    else if(code_rate == RATE_8191_8178)    
        filter_n = 11; 
    else if(code_rate == RATE_12000_11986)
        filter_n = 12;
  
   return filter_n;
}

/* 
 * ===  FUNCTION  ====================================================
 *         Name: vector_length 
 *  Description:  return the length of the parity check vector for a given
 *                code rate, note it's a uint32_t vector, indeed return the
 *                number of uint32_t
 *                in -> 
 *                      code_rate -> value of the code rate
 *                out -> length of the vector
 * =================================================================
 * */
uint16_t vector_length(uint16_t code_rate)
{
    uint16_t vector_l;

    if((code_rate == RATE_7_4) ||
       (code_rate == RATE_15_11) ||
       (code_rate == RATE_31_26))
       vector_l = 1;
    else if(code_rate == RATE_63_57) 
       vector_l = 2;
    else if(code_rate == RATE_127_120) 
        vector_l = 4;
    else if(code_rate == RATE_255_247) 
        vector_l = 8;
    else if(code_rate == RATE_1023_1013) 
        vector_l = 32;
    else if(code_rate == RATE_2047_2036) 
        vector_l = 64;
    else if(code_rate == RATE_4095_4083)
        vector_l = 128;
    else if(code_rate == RATE_8191_8178)    
        vector_l = 256; 
    else if(code_rate == RATE_12000_11986)
        vector_l = 512;
  
   return vector_l;
}

/* 
 * ===  FUNCTION  ====================================================
 *         Name: payload_length 
 *  Description:  return the max length of the payload for a given
 *                code rate
 *                in -> 
 *                      code_rate -> value of the code rate
 *                out -> length of the payload
 * =================================================================
 * */
uint16_t payload_length(uint16_t code_rate)
{
    uint16_t payload_l;

    if(code_rate == RATE_7_4) 
        payload_l = 7;
    else if(code_rate == RATE_15_11) 
        payload_l = 15;
    else if(code_rate == RATE_31_26)
        payload_l = 31;
    else if(code_rate == RATE_63_57) 
        payload_l = 63;
    else if(code_rate == RATE_127_120) 
        payload_l = 127;
    else if(code_rate == RATE_255_247) 
        payload_l = 255;
    else if(code_rate == RATE_1023_1013) 
        payload_l = 1023;
    else if(code_rate == RATE_2047_2036) 
        payload_l = 2047;
    else if(code_rate == RATE_4095_4083)
        payload_l = 4095;
    else if(code_rate == RATE_8191_8178)    
        payload_l = 8191; 
    else if(code_rate == RATE_12000_11986)  
        payload_l = 16383;
  
   return payload_l;
}
/* 
 * ===  FUNCTION  ====================================================
 *         Name: random_error 
 *  Description:  introduces a random error in the encoded packet
 *                in ->  encoded packet
 *                out -> encoded packet with one error
 * =================================================================
 * */
uint16_t random_error(uint32_t *encoded)
{   
    uint32_t bit_error;
   
    bit_error = rand() % 10;
    *encoded ^= (BIT_MASK << bit_error);

    return OK;
}

//*******************************************************************

#ifdef EXE
int main ( int argc, char *argv[] )
{
 uint32_t  *payload,i;
 uint16_t length;
 uint32_t rate = RATE_127_120;
 uint32_t *encoded;
 uint32_t *decoded; 
 uint8_t  *string;

 if (( payload = (uint32_t *)malloc(1500*sizeof(uint8_t))) == NULL)
    {
        printf("Malloc in SendMsg \n");
    }
 
 if (( encoded = (uint32_t *)malloc(1500*sizeof(uint8_t))) == NULL)
    {
        printf("Malloc in SendMsg \n");
    }
 if (( decoded = (uint32_t *)malloc(1500*sizeof(uint8_t))) == NULL)
    {
        printf("Malloc in SendMsg \n");
    }
 
 if (( string= (uint32_t *)malloc(1500*sizeof(uint8_t))) == NULL)
    {
        printf("Malloc in SendMsg \n");
    }
 
 string = "HEY";

length = strlen(string);
memcpy((void *)payload,(const void *)string,length*7);

dbg("payload value %d length %d \n",*payload,length*7);

if(hamming_init(rate,WRITE) != rate)
    dbg("Error setting rate");
vector_generator(WRITE);

*payload=0xFFFF;

encoded =  hamming_encoder(payload, 11); 

for(i=0;i<32;i++)
{
     printf("%d ", (*encoded >> i ) & 0x1);
}
printf("\n");

random_error(encoded);

decoded = hamming_decoder(encoded,11);

for(i=0;i<32;i++)
{
     printf("%d ", (*decoded >> i ) & 0x1);
}
printf("\n");


 
free(payload);
vector_generator(CLEAN);


    return EXIT_SUCCESS;
}
#endif
