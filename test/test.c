#include <windows.h>
//#include <devioctl.h>
//#include <ntdddisk.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <strsafe.h>
#include <intsafe.h>
#include <string.h>
#include <math.h>
int writeLinear[24];
int readLinear[24]={71,73,111,73,111,73,133,73,111,73,111,73,469,73,111,73,111,73,133,73,111,73,111,73};
int i = 0;
int j = 0;
int k = 0;
int byteShift = 0;
int main()
{

for(k=0;k<24;k++){
	byteShift-=readLinear[k];
	i = byteShift;j = k;
	while(i<5000){
		if(i>=0){
			writeLinear[i]=j;
			break;
		}
		i+=24;j+=24;
	}
}
for(k=0;k<24;k++){
	printf("%d, ",writeLinear[k]);
}
return 0;
}
