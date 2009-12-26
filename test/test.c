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
PUCHAR imageBuffer = NULL;
PUCHAR imageEnd = NULL;
PUCHAR imagePtr = NULL;
PUCHAR rewrBuffer = NULL;
PUCHAR rewrEnd = NULL;
PUCHAR rewrPtr = NULL;
int bSize = 105840;//35280 9696	105840
int bShift = 0; //2242 2499
int i = 0;

VOID
Rewrite(int delta)
{
rewrPtr-=delta;
if(rewrPtr<rewrBuffer) rewrPtr += bSize;
if(rewrPtr>rewrEnd) rewrPtr -= bSize;
memcpy(rewrPtr,imagePtr,1);
imagePtr+=1;
}

int main()
{
FILE *file,*bin;
imageBuffer = malloc(bSize);
imageEnd = imageBuffer + bSize; 
imagePtr = imageBuffer;
//fopen_s(&bin,"was.bin","r");fread(imageBuffer, sizeof(byte), bSize, bin);fclose(bin);
memset(imageBuffer,0x4A,bSize);memset((imageBuffer+(bSize/3*2)),0x54,(bSize/3));

rewrBuffer = malloc(bSize);
rewrEnd = rewrBuffer + bSize;
rewrPtr = rewrBuffer + bShift;

while (i<bSize){

Rewrite(0);
Rewrite(73); Rewrite(111);
Rewrite(73); Rewrite(111);
Rewrite(73); Rewrite(133);
Rewrite(73); Rewrite(111);
Rewrite(73); Rewrite(111);
Rewrite(73);
Rewrite(469);
Rewrite(73); Rewrite(111);
Rewrite(73); Rewrite(111);
Rewrite(73); Rewrite(133);
Rewrite(73); Rewrite(111);
Rewrite(73); Rewrite(111);
Rewrite(73);
i+=24;
rewrPtr = rewrBuffer + i;
}

fopen_s(&file,"file.txt","w");
fwrite(rewrBuffer, bSize, 1, file);
//fwrite(imageBuffer, bSize, 1, file);
fclose(file);

return 0;

}
