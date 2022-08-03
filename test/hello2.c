#include <stdio.h>

int main()
{

    int i;
	int j;
	i=0;
	while(1)
	{
		if(i < 10){
		 	printf("hello, world%d\n",i);
		}else{
		   j = i/2;
		    printf("task%d\n",j);
		}
	    i++;
	}
	return 0;
}

