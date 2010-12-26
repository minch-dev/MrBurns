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
#include "burn.h"
#include <math.h>



PUCHAR tjtjBuffer = NULL;
PUCHAR tjtjEnd = NULL;
PUCHAR imageBuffer = NULL;
PUCHAR imageEnd = NULL;
PUCHAR rewrBuffer = NULL;
PUCHAR rewrEnd = NULL;
PUCHAR copyBuffer = NULL;
PUCHAR copyEnd = NULL;
PUCHAR writeBuffer = NULL;

float LBAstart = 0x08A051;
float LBAend = 0x0B8DBB;
//0x07D5C8 - 0x086ABA 31-32
//0x1C6E2A - 0x1D031C 51-52
//0x08A051 - 0x0B8DBB 32-37
int circle = 105840;	//35280 9696	105840
int copyLength = 110160;
int commandLength = 36720;
int bShift = 0;	// 2499
int k = 0;
PUCHAR i = 0;
PUCHAR j = 0;

VOID
Rewrite(int delta)
{
	i-=delta;
	if(i<rewrBuffer) i += circle;
	if(i>rewrEnd) i -= circle;
	memcpy(i,j,1);
	j+=1;
}

int
__cdecl
main(int argc,__nullterminated char *argv[])
{
	BOOL status = 0;
	DWORD accessMode = 0, shareMode = 0;
	HANDLE fileHandle = NULL;
	ULONG alignmentMask = 0;
	PUCHAR pUnAlignedBuffer = NULL;

	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	CHAR string[25];
	FILE *ModeSelect10Data,*dump,*sample;
	ULONG	length = 0, errorCode = 0, returned = 0;

	if (argc < 2) {printf("Examples:\n    spti F:");return;}

	StringCbPrintf(string, sizeof(string), "\\\\.\\%s", argv[1]);
	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	accessMode = GENERIC_WRITE | GENERIC_READ;

	fileHandle = CreateFile(string, accessMode, shareMode, NULL, OPEN_EXISTING, 0, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		errorCode = GetLastError();
		printf("Error opening %s. Error: %d\n", string, errorCode);
		PrintError(errorCode);
		return;
	}

	// узнаём параметры устройства и "маску выравнивания"
	status = GetAlignmentMaskForDevice(fileHandle, &alignmentMask);

	//если не удаётся запросить параметры устройства - ошибка
	if (!status ) {
		errorCode = GetLastError();
		printf("Error getting device and/or adapter properties; error was %d\n", errorCode);
		PrintError(errorCode);
		CloseHandle(fileHandle);
		return;
	}


	
	fopen_s(&ModeSelect10Data, "V:\\ModeSelect10.bin", "rt" );
	printf("            *****       Mode Select (10)         *****\n");

	ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
	writeBuffer = AllocateAlignedBuffer(200,alignmentMask, &pUnAlignedBuffer);
	fread(writeBuffer, 200, 1, ModeSelect10Data);

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



	printf("            *****       Write         *****\n");

//--tjtjBuffer
	tjtjBuffer = malloc(96);
	i = tjtjBuffer;
	tjtjEnd = tjtjBuffer+96;
	memset(i, 0x54, 1);i++;
	memset(i, 0x4A, 1);i++;
	while (i<tjtjEnd){	memcpy(i, tjtjBuffer, 2);i+=2;	}
	i = 0;

//--buffers
	imageBuffer = malloc(circle);
	imageEnd = imageBuffer + circle;
	rewrBuffer = malloc(circle);
	rewrEnd = rewrBuffer + circle;
	copyBuffer = malloc(copyLength);
	copyEnd = copyBuffer + copyLength;
	
	//fopen_s(&dump,"V:\\dump.bin","a+");
	//fopen_s(&sample, "V:\\sample.bin", "rt" );
	while (LBAstart <= LBAend){
	//--gen circle
		//memset(imageBuffer,0x4A,circle);
		memset(imageBuffer,0x4A,circle);memset((imageBuffer+(circle/3*2)),0x54,(circle/3));

	//--struct circle
		k = 0;
		i = rewrBuffer + bShift;
		j = imageBuffer;
		while (k<circle){
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
			k+=24;
			i = rewrBuffer + k;
		}

	//--rewrite circle to copybuffer with tjtj blocks
		i=copyBuffer;
		j=rewrBuffer;
		while (i<copyEnd){
			memcpy(i,j,2352);i+=2352;j+=2352;
			memcpy(i,tjtjBuffer,96);i+=96;
		}

	//--sample way
		//fread(copyBuffer, copyLength, 1, sample);

		//send 3 commands
		for(k=0;k<3;k++){
			ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
			writeBuffer = AllocateAlignedBuffer(commandLength,alignmentMask, &pUnAlignedBuffer);
			memcpy(writeBuffer, (copyBuffer + k*commandLength), commandLength);

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
			LBAstart+=0xF;
		}
		//fwrite(copyBuffer, copyLength, 1, dump);
	}
	//fclose(dump);
	//fclose(sample);

	printf("            *****       Start Stop Unit         *****\n");

	ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

	sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
	sptwb.spt.PathId = 0;
	sptwb.spt.TargetId = 1;
	sptwb.spt.Lun = 0;
	sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
	sptwb.spt.SenseInfoLength = 32;	//SPT_SENSE_LENGTH
	sptwb.spt.DataIn = 2;	//SCSI_IOCTL_DATA_UNSPECIFIED
	sptwb.spt.DataTransferLength = 0;
	sptwb.spt.TimeOutValue = 65535;
	sptwb.spt.DataBufferOffset = 0;
	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);		//0x30
	sptwb.spt.Cdb[0] = SCSIOP_START_STOP_UNIT;
	sptwb.spt.Cdb[4] = 2;
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
	if (pUnAlignedBuffer != NULL) {
		free(pUnAlignedBuffer);
	}

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