/*************************************
hamming_matrix.c

Cesar Prados

this source generates either the vector
of the parity checks bits for a given 
(64,) with m as the numbers bits of the word 
**************************************/



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>

#define BIT_MASK 0x1

#define LENGTH 71 

typedef struct {
    uint8_t b : 1;
} bit;


int main ( void )
{

int i=0,j=0;
int position_bit;
bool power_2;
bit  *parity_check_mask;
int  position_matrix=0;
uint64_t  *h_parity_check_mask; 



parity_check_mask = (bit *)malloc(LENGTH*sizeof(bit));
h_parity_check_mask = (uint64_t *)malloc(sizeof(uint64_t));

 for(i=1;i<=LENGTH;i++)
 {
    position_matrix = 0;

    power_2= ((i & (i - 1)) == 0);

    memset(parity_check_mask, '0', LENGTH *sizeof(bit));
    memset(h_parity_check_mask, '\0',sizeof(uint64_t));
     
 if(power_2)
    {
       printf("****Mask for Parity bit %d ******\n",i);        
       position_bit = ffsl(i)-1;

       for(j=1;j<=LENGTH;j++)
       {
            if((0x1 & (j>>position_bit)) && !((j & (j - 1))==0))
           {
               
                (parity_check_mask+position_matrix)->b ^= BIT_MASK;
		        *h_parity_check_mask ^=  ( BIT_MASK << position_matrix );

                if((*h_parity_check_mask & 0x1) != 0x1){
                        printf("change %d \n",j);
                        printf("VALUE %llx ",*h_parity_check_mask);
                }
                position_matrix++;
           }
           else if(!((j & (j - 1))==0) && !(0x1 & (j>>position_bit))  )
           {
                position_matrix++;
            }
        }
	
	printf("%llx \n",*h_parity_check_mask);
	printf("%d \n",position_matrix);
	for(j=0;j<position_matrix;j++)
      	{
        	printf("%d ",(parity_check_mask+j)->b); 

      	}
      printf("\n");
    }
 }


 free(parity_check_mask);

 free(h_parity_check_mask);
return 0;
}
