
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hal_client.h>
#include <minipc.h>
#include <rtud_exports.h>


static struct minipc_ch *rtud_ch;
static hexp_port_list_t plist;
void rtudexp_get_fd_list(rtudexp_fd_list_t *list, int start_from)
{
	minipc_call(rtud_ch, 200, &rtud_export_get_fd_list, list,
		    start_from);
}

#define RTU_MAX_ENTRIES 8192



int fetch_rtu_fd(rtudexp_fd_entry_t *d, int *n_entries)
{
	int start = 0, n = 0;
	rtudexp_fd_list_t list;
	
	do {
		rtudexp_get_fd_list(&list, start);
	//	printf("num_rules %d\n", list.num_rules);
		
		memcpy( d+n, list.list, sizeof(rtudexp_fd_entry_t) * list.num_rules);
		start=list.next;	
		n+=list.num_rules;	
	} while(start > 0);

//	printf("%d rules \n", n);
	*n_entries = n;
}


static int cmp_entries(const void *p1, const void *p2)
{
	rtudexp_fd_entry_t *e1 = (rtudexp_fd_entry_t *)p1;
	rtudexp_fd_entry_t *e2 = (rtudexp_fd_entry_t *)p2;
	
	
	return memcmp(e1->mac, e2->mac, 6);
//           return strcmp(* (char * const *) p1, * (char * const *) p2);
}

char *mac_to_string(uint8_t mac[])
{
    char str[40];
        snprintf(str, 40, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
            return strdup(str); //FIXME: can't be static but this takes memory
            }
            
char *decode_ports(int dpm)
{
	static char str[80],str2[20];
	int i;

	if((dpm & ((1<<plist.num_physical_ports)-1)) == ((1<<plist.num_physical_ports)-1))
	{
		strcpy(str,"ALL");
		return str;		
	}	
	strcpy(str,"");



	for(i=0;i<plist.num_physical_ports;i++)
	{
		sprintf(str2,"%d ", i);
		if(dpm&(1<<i)) strcat(str,str2);
	}	

	if(dpm & (1<<plist.num_physical_ports))
		strcat(str, "CPU");
	
	return str;
}            
            
            
            
main()
{
	rtudexp_fd_entry_t fd_list[RTU_MAX_ENTRIES];

	int n_entries;
	int i;
	
	if(	halexp_client_init() < 0)
	{
		printf("Can't conenct to HAL mini-rpc server\n");
		return -1;
	}


	rtud_ch = minipc_client_create("rtud", 0);
	
	if(!rtud_ch)
	{
		printf("Can't conenct to RTUd mini-rpc server\n");
		return -1;
	}
	
	halexp_query_ports(&plist);
	fetch_rtu_fd(fd_list, &n_entries);

  qsort(fd_list, n_entries,  sizeof(rtudexp_fd_entry_t), cmp_entries);
	
	printf("RTU Filtering Database Dump: %d rules\n", n_entries);
	printf("\n");
	printf("MAC                       Dst.ports              Type                   Age [s]\n");
	printf("----------------------------------------------------------------------------------\n");

	for(i=0;i<n_entries;i++)
	{
		printf("%-25s %-22s %s (hash %03x:%x)   ", 
			mac_to_string(fd_list[i].mac), 
			decode_ports(fd_list[i].dpm), 
			fd_list[i].dynamic ? "DYNAMIC":"STATIC ", 
			fd_list[i].hash,
			fd_list[i].bucket);
		if(fd_list[i].dynamic)
			printf("%d\n", fd_list[i].age);
		else
			printf("-\n");
	}
	printf("\n");	
}
