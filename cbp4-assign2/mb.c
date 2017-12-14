#include <stdio.h>

int main()
{

  int a = 0;
  int i,j;
  //first loop: misprediction on first and last iteration
  for (i=0; i<1000;i=i+2)
  {
    //second loop:misprediction on the very first one(j=1000, i=0) and last interation(j=0)
    for(j=1000;j>0;j=j-2)
    { 
       //After lerning time, it will mispredict at j=1000,996
       if(j%6==0)
       {
         a=a+1;       
       } 
    }
  }   
   printf("%d",a);
   return 0; 
}
