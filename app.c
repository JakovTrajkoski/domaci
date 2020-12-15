#include <stdio.h>
#include <stdlib.h>

int main()
{
	int opcija = 0;
	FILE *my_stream;
	char broj[3];
	
	while (3)
	{
		printf("\n1: Upisi u FIFO bafer\n2: Ispisi iz FIFO bafera\n3: Izadji\n--->");
		scanf("%d", &opcija);

		switch(opcija)
		{
			case 1:
			{
				int uslov = 3;
				int uslov2 = 3;
				my_stream = fopen("/dev/fifo","w");
				if(my_stream == NULL){
					printf("Greska pri otvaranju /dev/fifo");
					return -1;
				}
	
				while(uslov2)
				{
					printf("uneti hex broj: ");
					scanf("%s",broj);
					if(uslov)
					{
						fprintf(my_stream,"%s",broj);
						uslov = 0;
					}
					else
					{
						if(broj[0] == 'Q')
							uslov2 = 0;
						else
							fprintf(my_stream,";%s",broj);
					}
				}
				fputs("\n",my_stream);
	
				if(fclose(my_stream)){
					printf("Greska pri zatvaranju /dev/fifo");
					return -1;
				}
				break;
			}
			case 2:
			{
				char *vrednost;
				size_t br_bita = 3;
				my_stream = fopen("/dev/fifo","r");
				if(my_stream == NULL){
					puts("Problem pri otvaranju /dev/fifo");
					return -1;
				}
	
				vrednost = (char *) malloc(br_bita);
				getline(&vrednost, &br_bita,my_stream);
				printf("procitan broj: %s\n", vrednost);
	
				if(fclose(my_stream)){
					puts("Problem pri zatvaranju bibilioteke");
					return -1;
				}
				break;
			}
			case 3:
				return 0;
		}
	}
}
