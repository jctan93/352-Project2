#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main()
{
  long n = 1011101010111;
 
  long temp = n;
  
  int decimal=0, i=0, rem;
    while (n!=0)
    {
        rem = n%10;
        n/=10;
        decimal += rem*pow(2,i);
        ++i;
    }
   printf("%ld in decimal is %d\n", temp, decimal);
   
   
   char* test = (char*)malloc(100* sizeof(char));
   sprintf(test, "%ld", temp);
   
   printf("Converted binary to string is %s\n", test);
   
    char *ret = malloc (100* sizeof(char));
    for (i = 0; i <= 5; i++)
    {
        ret[i] = test[strlen(test) - (5 - i)];
    }
   
   printf("%s\n", ret);
   
  /*
  r = n;
  
  char* stringnum = malloc(20* sizeof(char));
  char* temp = malloc(10*sizeof(char));
  char* temp2 = malloc(20* sizeof(char));
  strcpy(stringnum, "");
  strcpy(temp, "");
  strcpy(temp2, "");
  while(n > 0)
  {
	int a = n%2;
	sprintf(temp,"%d", a);
	strcpy(temp2, stringnum);
	strcpy(stringnum, temp);
	strcat(stringnum, temp2);
	
	n = n/2;
  }
 
  printf("%d in binary is %s\n", r, stringnum);
 
  return 0;
  */
}
