#include <windows.h>
//#include <devioctl.h>
//#include <ntdddisk.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h> //free()
#include <strsafe.h>
#include <intsafe.h>
#include <string.h>
#include "burn.h"
#include <math.h>

PUCHAR tjtjBuffer = NULL, tjtjEnd = NULL;
PUCHAR dumpBuffer = NULL, dumpEnd = NULL;
PUCHAR copyBuffer, copyEnd = NULL;
PUCHAR writeBuffer = NULL;

int dumpLength = 0, copyLength = 0, commandsNum = 0;
int rawLength = 35280;
int commandLength = 36720;
int circleLength = 105840;

BOOL status = 0;
DWORD accessMode = 0, shareMode = 0;
HANDLE fileHandle = NULL;
ULONG alignmentMask = 0;
PUCHAR pUnAlignedBuffer = NULL;

SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
CHAR string[25];
ULONG	length = 0, errorCode = 0, returned = 0;

int writeLinear[24]={145,71,725,651,1641,1567,2221,2147,331,257,911,837,1827,1753,2407,2333,517,443,1097,1023,2013,1939,2593,2519};
UCHAR customCdb[16];
FILE *ModeSelect10Data,*dump,*sample;

int k = 0;
int m = 0;
int n = 0;
PUCHAR i = 0;
PUCHAR j = 0;
const float PI = 3.14159265358979323846f;

int LBA[33]={0x039D96, 0x04418B, 0x04EBEE, 0x059CC0, 0x0653FF, 0x0711AD, 0x07D5C8, 0x08A051, 0x097149, 0x0A48AE, 0x0B2682, 0x0C0AC3, 0x0CF572, 0x0DE690, 0x0EDE1B, 0x0FDC15, 0x10E07C, 0x11EB52, 0x12FC95, 0x141447, 0x153267, 0x1556F4, 0x1781F0, 0x18B359, 0x19EB31, 0x1B2977, 0x1C6E2A, 0x1DB94C, 0x1F0ADB, 0x2062d9, 0x21C145, 0x23261F, 0x249166};
int LBAstart;
float rStart = 26;

//0x039D96 - 0x039DF0 6 commands
//0x039D96 - 0x03AB88 25-25.1
//0x07D5C8 - 0x086ABA 31-32
//0x07D5C8 - 0x08FFAC 31-33
//0x1C6E2A - 0x1D031C 51-52
//0x08A051 - 0x0B8DBB 32-37

//--GrayCode variables
int linesPerMm = 3413;	//	3413
int partsNumber, partsWidth, partsOffset, partsShift;
int trackWidth, grayCodeBits = 13;
float partsRawWidth,
	raidSize = 0.96923f,
	floorHalf = 0.5f;
float trackRawWidth;

void zeroArray(){	for (m=0;m>16;m++)customCdb[m]=0;	}
void sptwbCommand(UCHAR CdbLength, UCHAR SenseInfoLength, UCHAR DataIn, ULONG DataTransferLength, ULONG TimeOutValue, UCHAR Cdb[16]	)
{
	ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
	sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
	sptwb.spt.PathId = 0;
	sptwb.spt.TargetId = 1;
	sptwb.spt.Lun = 0;
	sptwb.spt.CdbLength = CdbLength;
	sptwb.spt.SenseInfoLength = SenseInfoLength;
	sptwb.spt.DataIn = DataIn;
	sptwb.spt.DataTransferLength = DataTransferLength;
	sptwb.spt.TimeOutValue = TimeOutValue;
	sptwb.spt.DataBufferOffset = 0;
	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);		//0x30
	sptwb.spt.Cdb[0] = Cdb[0];
	sptwb.spt.Cdb[1] = Cdb[1];
	sptwb.spt.Cdb[2] = Cdb[2];
	sptwb.spt.Cdb[3] = Cdb[3];
	sptwb.spt.Cdb[4] = Cdb[4];
	sptwb.spt.Cdb[5] = Cdb[5];
	if(CdbLength>6){
		sptwb.spt.Cdb[6] = Cdb[6];
		sptwb.spt.Cdb[7] = Cdb[7];
		sptwb.spt.Cdb[8] = Cdb[8];
		sptwb.spt.Cdb[9] = Cdb[9];
	}
	if(CdbLength>10){
		sptwb.spt.Cdb[10] = Cdb[10];
		sptwb.spt.Cdb[11] = Cdb[11];
	}
	if(CdbLength>12){
		sptwb.spt.Cdb[12] = Cdb[12];
		sptwb.spt.Cdb[13] = Cdb[13];
		sptwb.spt.Cdb[14] = Cdb[14];
		sptwb.spt.Cdb[15] = Cdb[15];
	}
	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);

	status = DeviceIoControl(fileHandle,
								IOCTL_SCSI_PASS_THROUGH,
								&sptwb,
								sizeof(SCSI_PASS_THROUGH),
								&sptwb,
								length,
								&returned,
								FALSE);

	PrintStatusResults(status,returned,&sptwb,length);
}
void modeSelect(){
	fopen_s(&ModeSelect10Data, "V:\\ModeSelect10HIGH.bin", "rb" );
	printf("            *****       Mode Select (10)         *****\n");

	ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
	writeBuffer = AllocateAlignedBuffer(200,alignmentMask, &pUnAlignedBuffer);
	fread(writeBuffer, 200, 1, ModeSelect10Data);
	fclose(ModeSelect10Data);
	sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptdwb.sptd.PathId = 0;
	sptdwb.sptd.TargetId = 1;
	sptdwb.sptd.Lun = 0;
	sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	sptdwb.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
	sptdwb.sptd.DataTransferLength = 200;
	sptdwb.sptd.TimeOutValue = 65535;
	sptdwb.sptd.DataBuffer = writeBuffer;
	sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);	//0x30
	sptdwb.sptd.Cdb[0] = SCSIOP_MODE_SELECT10;
	sptdwb.sptd.Cdb[1] = 16;                         // Data mode
	sptdwb.sptd.Cdb[8] = 200;
	length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(fileHandle,
								IOCTL_SCSI_PASS_THROUGH_DIRECT,
								&sptdwb,
								length,
								&sptdwb,
								length,
								&returned,
								FALSE);

	PrintStatusResults(status,returned,(PSCSI_PASS_THROUGH_WITH_BUFFERS)&sptdwb,length);
}
void modeWrite(){

			sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
			sptdwb.sptd.PathId = 0;
			sptdwb.sptd.TargetId = 1;
			sptdwb.sptd.Lun = 0;
			sptdwb.sptd.CdbLength = CDB12GENERIC_LENGTH;
			sptdwb.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
			sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
			sptdwb.sptd.DataTransferLength = commandLength;
			sptdwb.sptd.TimeOutValue = 5000;
			sptdwb.sptd.DataBuffer = writeBuffer;
			sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);	//0x30
			sptdwb.sptd.Cdb[0] = SCSIOP_WRITE12;
		//--LBA
			sptdwb.sptd.Cdb[3] = (unsigned char)(LBAstart / 0x10000);
			sptdwb.sptd.Cdb[4] = (unsigned char)(LBAstart / 0x100);
			sptdwb.sptd.Cdb[5] = (unsigned char)LBAstart;

			sptdwb.sptd.Cdb[9] = 0x0F;
			length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
			status = DeviceIoControl(fileHandle,IOCTL_SCSI_PASS_THROUGH_DIRECT,&sptdwb,length,&sptdwb,length,&returned,FALSE);
}
void synchronizeCache(){
	printf("            *****     Synchronize Cache       *****\n");
	zeroArray();
	customCdb[0] = 0x35;
	sptwbCommand(10,32,2,0,65535,customCdb);
}
void preventAllowMediumRemoval(){
	printf("            *****     Prevent Allow Medium Removal       *****\n");
	zeroArray();
	customCdb[0] = 0x1E;
	sptwbCommand(CDB6GENERIC_LENGTH,32,2,0,65535,customCdb);
}
void startStopUnit(){
	printf("            *****       Start Stop Unit         *****\n");
	zeroArray();
	customCdb[0] = SCSIOP_START_STOP_UNIT;
	customCdb[4] = 2;
	sptwbCommand(CDB6GENERIC_LENGTH,32,2,0,65535,customCdb);
}
void testUnitReady(){
	printf("            *****       Test Unit Ready         *****\n");
	zeroArray();
	customCdb[0] = 0;
	sptwbCommand(CDB6GENERIC_LENGTH,32,2,0,65535,customCdb);
}

int
__cdecl
main(int argc,__nullterminated char *argv[])
{
	if (argc < 2) {
		//printf("Examples:\n    spti F:");
		argv[1] = "F:";
		//return;
	}
	StringCbPrintf(string, sizeof(string), "\\\\.\\%s", argv[1]);
	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	accessMode = GENERIC_WRITE | GENERIC_READ;

//--открываем привод
	fileHandle = CreateFile(string, accessMode, shareMode, NULL, OPEN_EXISTING, 0, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		errorCode = GetLastError();
		printf("Error opening %s. Error: %d\n", string, errorCode);
		PrintError(errorCode);
		return;
	}

//--узнаём параметры устройства и маску выравнивания
	status = GetAlignmentMaskForDevice(fileHandle, &alignmentMask);

//--если не удаётся запросить параметры устройства - ошибка
	if (!status ) {
		errorCode = GetLastError();
		printf("Error getting device and/or adapter properties; error was %d\n", errorCode);
		PrintError(errorCode);
		CloseHandle(fileHandle);
		return;
	}

//--выбираем режим
	modeSelect();

//--tjtjBuffer
	tjtjBuffer = malloc(96);
	i = tjtjBuffer;
	tjtjEnd = tjtjBuffer+96;
	memset(i, 0x54, 1);i++;
	memset(i, 0x4A, 1);i++;
	while (i<tjtjEnd){	memcpy(i, tjtjBuffer, 2);i+=2;	}
	//for(i;i<tjtjEnd;i+=2)memcpy(i, tjtjBuffer, 2);

//--buffers
	dumpLength = circleLength *2;
	dumpBuffer = malloc(dumpLength);
	dumpEnd = dumpBuffer + circleLength;

	copyLength = commandLength * 3;
	copyBuffer = malloc(copyLength);
	copyEnd = copyBuffer + copyLength;

	LBAstart = LBA[(int)(rStart-25)];
	//trackRawWidth = linesPerMm * (raidSize /6);
	trackRawWidth = linesPerMm * 6*PI*rStart/(4096 - 3*PI*15);
	trackWidth = (int)(floorHalf + trackRawWidth);
	commandsNum = trackWidth*3;

	for (n=0;n<grayCodeBits;n++){

	//--gen dump
		partsNumber = (int)pow(2,n);
		if(n==0) partsNumber = 2;
		partsRawWidth = (float) circleLength / partsNumber;
		partsShift = (int)(floorHalf + (partsRawWidth/2));
		if(n==0) partsShift = 0;
		memset(dumpBuffer,0x4A,circleLength);
		for(k=0;k<partsNumber;k+=2){
			partsOffset = (int)((k* partsRawWidth)+floorHalf);
			partsWidth = (int)(((k+1)*partsRawWidth)+floorHalf) - partsOffset;
			partsOffset += partsShift; 
			memset( (dumpBuffer+partsOffset), 0x54, partsWidth);
		}
		memcpy(dumpBuffer + circleLength, dumpBuffer, circleLength);

	//--tests
		//fopen_s(&dump,"V:\\dump.bin","wb+"); if(n==0)fwrite(dumpBuffer, circleLength, 1, dump); fclose(dump);
	//--struct dump
		i = dumpBuffer;	j = dumpBuffer;	k = 0;
		while(i<dumpEnd){
			j = i + writeLinear[k];		//if(j>=dumpEnd) memset(i,0x4A,1); else memcpy(i,j,1);
			memcpy(i,j,1);
			i++;k++;if(k>23)k=0;
		}
	
	//--rewrite structed dump to copybuffer with tjtj blocks
		i=copyBuffer;	j=dumpBuffer;
		while (i<copyEnd){
			memcpy(i,j,2352);i+=2352;j+=2352;
			memcpy(i,tjtjBuffer,96);i+=96;
		}
		//free(dumpBuffer);

	//--write desu
		printf("            *****       Fukken WRITE         *****\n");
		i = copyBuffer;
		if(n==(grayCodeBits - 2)) commandsNum*=2;
		for(k=0;k<commandsNum;k++){
			ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
			writeBuffer = AllocateAlignedBuffer(commandLength,alignmentMask, &pUnAlignedBuffer);
			memcpy(writeBuffer, i, commandLength);
			modeWrite();
			free(writeBuffer);
			LBAstart += 0xF;
			i += commandLength;
			if(i>=copyEnd) i=copyBuffer;
		}
	
	}


//--stop
	synchronizeCache();
	for(k=0;k<10;k++){
		testUnitReady();
		if(sptwb.spt.ScsiStatus == 0) break;
		Sleep(20);
	}
	preventAllowMediumRemoval();
	for(k=0;k<10;k++){
		startStopUnit();
		if(sptwb.spt.ScsiStatus == 0) break;
		Sleep(50);
	}


//--clean
	//if (pUnAlignedBuffer != NULL) free(pUnAlignedBuffer);
	CloseHandle(fileHandle);
}

VOID
PrintError(ULONG ErrorCode)
{
	CHAR errorBuffer[80];
	ULONG count;

	count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					ErrorCode,
					0,
					errorBuffer,
					sizeof(errorBuffer),
					NULL
					);

	if (count != 0) {
		printf("%s\n", errorBuffer);
	} else {
		printf("Format message failed.  Error: %d\n", GetLastError());
	}
}

VOID
PrintDataBuffer(PUCHAR DataBuffer, ULONG BufferLength)
{
	ULONG Cnt;

	printf("      00  01  02  03  04  05  06  07   08  09  0A  0B  0C  0D  0E  0F\n");
	printf("      ---------------------------------------------------------------\n");
	for (Cnt = 0; Cnt < BufferLength; Cnt++) {
		if ((Cnt) % 16 == 0) {
			printf(" %03X  ",Cnt);
			}
		printf("%02X  ", DataBuffer[Cnt]);
		if ((Cnt+1) % 8 == 0) {
			printf(" ");
			}
		if ((Cnt+1) % 16 == 0) {
			printf("\n");
			}
		}
	printf("\n\n");
}


PUCHAR
AllocateAlignedBuffer(ULONG size, ULONG AlignmentMask, PUCHAR *pUnAlignedBuffer)
{
	PUCHAR ptr;

	// NOTE: This routine does not allow for a way to free
	//       memory.  This is an excercise left for the reader.
	UINT_PTR    align64 = (UINT_PTR)AlignmentMask;

	if (AlignmentMask == 0) {
		ptr = malloc(size);
	} else {
		ULONG totalSize;

		ULongAdd(size, AlignmentMask, &totalSize);
		ptr = malloc(totalSize);
		*pUnAlignedBuffer = ptr;
		ptr = (PUCHAR)(((UINT_PTR)ptr + align64) & ~align64);
	}

	if (ptr == NULL) {
		printf("Memory allocation error.  Terminating program\n");
		exit(1);
	} else {
		return ptr;
	}
}

VOID
PrintStatusResults(
	BOOL status,DWORD returned,PSCSI_PASS_THROUGH_WITH_BUFFERS psptwb,
	ULONG length)
{
	ULONG errorCode;

	if (!status ) {
		printf( "Error: %d  ",
			errorCode = GetLastError() );
		PrintError(errorCode);
		return;
		}
	if (psptwb->spt.ScsiStatus) {
		PrintSenseInfo(psptwb);
		return;
		}
	else {
		printf("Scsi status: %02Xh, Bytes returned: %Xh, ",
			psptwb->spt.ScsiStatus,returned);
		printf("Data buffer length: %Xh\n\n\n",
			psptwb->spt.DataTransferLength);
		PrintDataBuffer((PUCHAR)psptwb,length);
		}
}


VOID
PrintSenseInfo(PSCSI_PASS_THROUGH_WITH_BUFFERS psptwb)
{
	int i;

	printf("Scsi status: %02Xh\n\n",psptwb->spt.ScsiStatus);
	if (psptwb->spt.SenseInfoLength == 0) {
		return;
		}
	printf("Sense Info -- consult SCSI spec for details\n");
	printf("-------------------------------------------------------------\n");
	for (i=0; i < psptwb->spt.SenseInfoLength; i++) {
		printf("%02X ",psptwb->ucSenseBuf[i]);
		}
	printf("\n\n");
}

BOOL
GetAlignmentMaskForDevice(
	IN HANDLE DeviceHandle,
	OUT PULONG AlignmentMask
	)
{
	PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor = NULL;
	PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
	STORAGE_DESCRIPTOR_HEADER header = {0};

	BOOL ok = TRUE;
	BOOL failed = TRUE;
	ULONG i;

	*AlignmentMask = 0; // default to no alignment

	// Loop twice:
	//  First, get size required for storage adapter descriptor
	//  Second, allocate and retrieve storage adapter descriptor
	//  Third, get size required for storage device descriptor
	//  Fourth, allocate and retrieve storage device descriptor
	for (i=0;i<4;i++) {

		PVOID buffer = NULL;
		ULONG bufferSize = 0;
		ULONG returnedData;

		STORAGE_PROPERTY_QUERY query = {0};

		switch(i) {
			case 0: {
				query.QueryType = PropertyStandardQuery;
				query.PropertyId = StorageAdapterProperty;
				bufferSize = sizeof(STORAGE_DESCRIPTOR_HEADER);
				buffer = &header;
				break;
			}
			case 1: {
				query.QueryType = PropertyStandardQuery;
				query.PropertyId = StorageAdapterProperty;
				bufferSize = header.Size;
				if (bufferSize != 0) {
					adapterDescriptor = LocalAlloc(LPTR, bufferSize);
					if (adapterDescriptor == NULL) {
						goto Cleanup;
					}
				}
				buffer = adapterDescriptor;
				break;
			}
			case 2: {
				query.QueryType = PropertyStandardQuery;
				query.PropertyId = StorageDeviceProperty;
				bufferSize = sizeof(STORAGE_DESCRIPTOR_HEADER);
				buffer = &header;
				break;
			}
			case 3: {
				query.QueryType = PropertyStandardQuery;
				query.PropertyId = StorageDeviceProperty;
				bufferSize = header.Size;

				if (bufferSize != 0) {
					deviceDescriptor = LocalAlloc(LPTR, bufferSize);
					if (deviceDescriptor == NULL) {
						goto Cleanup;
					}
				}
				buffer = deviceDescriptor;
				break;
			}
		}

		// buffer can be NULL if the property queried DNE.
		if (buffer != NULL) {
			RtlZeroMemory(buffer, bufferSize);

			// all setup, do the ioctl
			ok = DeviceIoControl(DeviceHandle,
									IOCTL_STORAGE_QUERY_PROPERTY,
									&query,
									sizeof(STORAGE_PROPERTY_QUERY),
									buffer,
									bufferSize,
									&returnedData,
									FALSE);
			if (!ok) {
				if (GetLastError() == ERROR_MORE_DATA) {
					// this is ok, we'll ignore it here
				} else if (GetLastError() == ERROR_INVALID_FUNCTION) {
					// this is also ok, the property DNE
				} else if (GetLastError() == ERROR_NOT_SUPPORTED) {
					// this is also ok, the property DNE
				} else {
					// some unexpected error -- exit out
					goto Cleanup;
				}
				// zero it out, just in case it was partially filled in.
				RtlZeroMemory(buffer, bufferSize);
			}
		}
	} // end i loop

	// adapterDescriptor is now allocated and full of data.
	// deviceDescriptor is now allocated and full of data.

	if (adapterDescriptor == NULL) {
		printf("   ***** No adapter descriptor supported on the device *****\n");
	} else {
		//PrintAdapterDescriptor(adapterDescriptor);
		*AlignmentMask = adapterDescriptor->AlignmentMask;
	}

	if (deviceDescriptor == NULL) {
		printf("   ***** No device descriptor supported on the device  *****\n");
	} else {
		//PrintDeviceDescriptor(deviceDescriptor);
	}

	failed = FALSE;

Cleanup:
	if (adapterDescriptor != NULL) {
		LocalFree( adapterDescriptor );
	}
	if (deviceDescriptor != NULL) {
		LocalFree( deviceDescriptor );
	}
	return (!failed);

}

/*	for(k=0;k<24;k++){
		byteShift-=readLinear[k];i = structBuffer + byteShift;j = dumpBuffer + k;
		while(i<structEnd){if(i>=structBuffer) memcpy(i,j,1);if(j>=dumpEnd) memset(i,0x4A,1);i+=24;j+=24;}
	}
	free(dumpBuffer);*/
//PUCHAR structBuffer = NULL,structEnd = NULL;
//structBuffer = malloc(dumpLength); structEnd = structBuffer + dumpLength;
//int byteShift = 0;
//int readLinear[24]={71,73,111,73,111,73,133,73,111,73,111,73,469,73,111,73,111,73,133,73,111,73,111,73};
//int writeLinear[24]={145, 72, 727, 654, 1645, 1572, 2227, 2154, 339, 266, 921, 848, 1839, 1766, 2421, 2348, 533, 460, 1115, 1042, 2033, 1960, 2615, 2542};
//commandsNum = 3*((int)(((LBAend - LBAstart)/0x0F)/3))
//for(i=dumpBuffer+circleLength;i<dumpEnd;i+=circleLength) memcpy(i, dumpBuffer, circleLength);
//k = circleLength/20;for(m = 0;m<20;m+=2)memset(dumpBuffer+k*m,0x54,k);