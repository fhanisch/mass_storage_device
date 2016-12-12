//gcc -o flash_disk flashdisk.c -lusb-1.0

// TODO: Tatsächliche SCSI Command-Block-Länge verwenden (momentan sind es immer 16 Byte)
// TODO: mehere Command Abfrage schleifen

#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#define INTERFACE 0

#define END_POINT_IN 0x81
#define END_POINT_OUT 0x02

#define INQUIRY_LENGTH                0x24
#define READ_CAPACITY_LENGTH          0x08

#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

static uint8_t cdb_length[256] = {
//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  0
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  1
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  2
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  3
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  4
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  5
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  6
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  7
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  8
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  9
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  A
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  F
};

struct command_block_wrapper {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
};

struct command_status_wrapper {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};

static void display_buffer_hex(unsigned char *buffer, unsigned size)
{
	unsigned i, j, k;

	for (i=0; i<size; i+=16) {
		printf("\n  %08x  ", i);
		for(j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				printf("%02x", buffer[i+j]);
			} else {
				printf("  ");
			}
			printf(" ");
		}
		printf(" ");
		for(j=0,k=0; k<16; j++,k++) {
			if (i+j < size) {
				if ((buffer[i+j] < 32) || (buffer[i+j] > 126)) {
					printf(".");
				} else {
					printf("%c", buffer[i+j]);
				}
			}
		}
	}
	printf("\n" );
}

int send_cmd_block(libusb_device_handle *handle, uint8_t *cdb, uint32_t dataLength, uint8_t direction)
{
	int ret;
	int transferred;
	struct command_block_wrapper cbw;
	uint8_t cdb_len;
	
	cdb_len = cdb_length[cdb[0]];		
	memset(&cbw, 0, sizeof(cbw));
	cbw.dCBWSignature[0] = 'U';
	cbw.dCBWSignature[1] = 'S';
	cbw.dCBWSignature[2] = 'B';
	cbw.dCBWSignature[3] = 'C';
	cbw.dCBWTag = 1;
	cbw.dCBWDataTransferLength = dataLength;
	cbw.bmCBWFlags = direction;
	cbw.bCBWLUN = 0;
	cbw.bCBWCBLength = cdb_len;
	memcpy(cbw.CBWCB, cdb, cdb_len);
	
	ret = libusb_bulk_transfer(handle, END_POINT_OUT, (unsigned char*)&cbw, 31, &transferred, 1000);	
	if (ret!=0)
	{
		printf("bulk_transfer failed %d\n",ret);			
		return 1;
	}
	printf("   Sent %d CDB bytes\n", transferred);
	
	return 0;
}

int transferData(libusb_device_handle *handle, unsigned char end_point, uint8_t *buffer, uint32_t dataLength)
{	
	int ret;
	int transferred;
	
	ret = libusb_bulk_transfer(handle, end_point, (unsigned char*)buffer, dataLength, &transferred, 1000);
	if (ret!=0)
	{
		printf("bulk_transfer failed %d\n",ret);
		return 1;
	}
	
	if (end_point == END_POINT_IN)
		printf("   Received ");
	else
		printf("   Sent ");
		
	printf("%d bytes\n", transferred);
		
	return 0;
}

int get_cmd_status(libusb_device_handle *handle)
{
	int ret;
	int transferred;
	struct command_status_wrapper csw;
		
	ret = libusb_bulk_transfer(handle, END_POINT_IN, (unsigned char*)&csw, 13, &transferred, 1000);
	if (ret!=0)
	{
		printf("get_cmd_status failed: %d\n",ret);
		return 1;
	}
		
	return 0;
}

void close_dev(libusb_device_handle *handle)
{
	libusb_reset_device(handle);
	libusb_release_interface(handle, INTERFACE);
	libusb_attach_kernel_driver(handle,INTERFACE);	
	libusb_exit(NULL);
}

int main(int argc, char **argv)
{
	int ret;	
	libusb_device_handle *handle = NULL;	
	uint8_t lun;
	uint8_t cdb[16];	// SCSI Command Descriptor Block
	uint8_t buffer[1024];
	char vid[9], pid[9], rev[5];
	int i;
	uint32_t max_lba, block_size;
	double device_size;	
	char bWrite=0;
	char *str=NULL;

	printf("*** Mass Storage Device Test ***\n\n");
	
	if (argc>1)
	{
		str = argv[1];
		bWrite = 1;
	}
	
	ret = libusb_init(NULL);
	if (ret) return 1;
	printf("Init OK\n");
		
	handle = libusb_open_device_with_vid_pid(NULL,0x090c,0x1000); // Flash Disk
	if (!handle)
	{
		printf("Kein device gefunden!\n");
		return 2;
	}
	libusb_reset_device(handle);
	libusb_detach_kernel_driver(handle,INTERFACE);
	
	ret = libusb_claim_interface(handle,INTERFACE);
	if (ret < 0)
	{
		printf("usb_claim_interface error %d\n", ret);
		return 3;
	}
	printf("claimed interface\n");
	
	ret = libusb_control_transfer(handle,0b00100001,0b11111111,0,0,NULL,0,0);
	if (ret) return 4;
	printf("Reset OK\n");
	ret = libusb_control_transfer(handle,0b10100001,0b11111110,0,0,&lun,1,0);
	if (ret<0)
	{
		printf("Error %d\n",ret);
		return 5;
	}
	printf("Anzahl LUN = %d\n",lun);
	
	//Inquiry
	printf("Sending Inquiry:\n");
	memset(buffer, 0, sizeof(buffer));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = 0x12;	// Inquiry
	cdb[4] = INQUIRY_LENGTH;
	
	ret = send_cmd_block(handle, cdb, INQUIRY_LENGTH, LIBUSB_ENDPOINT_IN);
	if (ret)
	{
		printf("send block command failed\n");
		close_dev(handle);
		return 1;
	}
		
	ret = transferData(handle, END_POINT_IN, buffer, INQUIRY_LENGTH);
	if (ret)
	{
		printf("transfer data failed\n");
		close_dev(handle);
		return 1;
	}
	
	ret=get_cmd_status(handle);
	if (ret)
	{
		close_dev(handle);
		return 1;
	}
	
	// The following strings are not zero terminated
	for (i=0; i<8; i++) {
		vid[i] = buffer[8+i];
		pid[i] = buffer[16+i];
		rev[i/2] = buffer[32+i/2];	// instead of another loop
	}
	vid[8] = 0;
	pid[8] = 0;
	rev[4] = 0;
	printf("   VID:PID:REV \"%8s\":\"%8s\":\"%4s\"\n", vid, pid, rev);	
		
	// Read capacity
	printf("Reading Capacity:\n");
	memset(buffer, 0, sizeof(buffer));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = 0x25;	// Read Capacity	
	
	ret = send_cmd_block(handle, cdb, READ_CAPACITY_LENGTH, LIBUSB_ENDPOINT_IN);
	if (ret)
	{
		printf("send block command failed\n");
		close_dev(handle);
		return 1;
	}
	
	ret = transferData(handle, END_POINT_IN, buffer, READ_CAPACITY_LENGTH);
	if (ret)
	{
		printf("transfer data failed\n");
		close_dev(handle);
		return 1;
	}
	
	ret=get_cmd_status(handle);
	if (ret)
	{
		close_dev(handle);
		return 1;
	}
	
	max_lba = be_to_int32(&buffer[0]);
	block_size = be_to_int32(&buffer[4]);
	device_size = ((double)(max_lba+1))*block_size/(1024*1024*1024);
	printf("   Max LBA: %08X, Block Size: %08X (%.2f GB)\n", max_lba, block_size, device_size);		
		
	// Write Data
	if (bWrite)
	{
		printf("Write Data:\n");
		memset(buffer, 0, sizeof(buffer));
		memset(cdb, 0, sizeof(cdb));
		cdb[0] = 0x2A;	// Write(10)
		cdb[5] = 10;	
		cdb[8] = 0x02;	// 1 block		
				
		ret = send_cmd_block(handle, cdb, 1024, LIBUSB_ENDPOINT_OUT);
		if (ret)
		{
			printf("send block command failed\n");
			close_dev(handle);
			return 1;
		}
		
		strcpy((char*)buffer,"Das ist das Haus vom Nikolaus!!!!!!\nUnd nebenan vom Weihnachtsmann!!!!!!!\n*** Jucheh Jucheh ***");
		if (str) strcpy((char*)buffer+200,str);
		strcpy((char*)buffer+512,"Na das ist aber fein!!!");
		if (str) strcpy((char*)buffer+512+200,str);
		
		ret = transferData(handle, END_POINT_OUT, buffer, 1024);
		if (ret)
		{
			printf("transfer data failed\n");
			close_dev(handle);
			return 1;
		}
		
		ret=get_cmd_status(handle);
		if (ret)
		{
			close_dev(handle);
			return 1;
		}
	}
	
	// Read Data
	printf("Read Data:\n");
	memset(buffer, 0, sizeof(buffer));
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = 0x28;	// Read(10)	
	cdb[5] = 10; //an LBA 0 und 32 bereits gespeicherte Strings
	cdb[8] = 0x02;	// 1 block
	
	ret = send_cmd_block(handle, cdb, 1024, LIBUSB_ENDPOINT_IN);
	if (ret)
	{
		printf("send block command failed\n");
		close_dev(handle);
		return 1;
	}
	
	ret = transferData(handle, END_POINT_IN, buffer, 1024);
	if (ret)
	{
		printf("transfer data failed\n");
		close_dev(handle);
		return 1;
	}
	get_cmd_status(handle);
	if (ret)
	{
		close_dev(handle);
		return 1;
	}
	
	display_buffer_hex(buffer, 1024);	
		
	close_dev(handle);
	return 0;
}