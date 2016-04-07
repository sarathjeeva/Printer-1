//***********************************************************************//
//                                                                       //
//                                                                       //
//      PRINTER DRIVER SOURCE FOR IMAGE PRINTING AND TEXT PRINTING       //
//                                                                       //
//                                                                       //
//***********************************************************************//

// This driver source have designed for both image printing and text printing
// Version : V1.0


//-------Kernel Header files included------

#include<linux/module.h>
#include<linux/fs.h>
#include<linux/gpio.h>
#include<linux/delay.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>

//-------Header files included------

#include "motor.h" 
#include "envy_code_12.h"
#include "test_reg_12.h"
#include "cousine12_bold.h"
//#include "DejaVuSansMono_bold14.h"
#include "envy_code_italic_12.h"

//-------macros defined -------

#define SPI_BUFF_SIZE	128
#define USER_BUFF_SIZE	2048

#define SPI_BUS 1
#define SPI_BUS_CS1 0
#define SPI_BUS_SPEED 1000000

//------function decleration--------------

int Noofbytes(void);
int paper_sensing(void);
void rotate(int);

//------Driver name-----------------------
const char this_driver_name[] = "printer";

//-------------
struct printer_control
{
	struct spi_message msg;
	struct spi_transfer transfer;
	u8 *tx_buff; 
	u8 *rx_buff;
};
struct printer_control printer_ctl;

//------------
struct printer_dev {
	struct semaphore spi_sem;
	struct semaphore fop_sem;
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct spi_device *spi_device;
	char *user_buff;
	u8 test_data;	
};
struct printer_dev printer_dev;

//-------------------------variable declarations---------------------
char **buff,temp_image,q;
int length,i,k,z,n,y=100,g[2000],w,x,odd,even,file=10,low_bat,rotate_pulse_count=1,rotate_loop=0,loop=0;
u8 tmp[48];
u32 *addr=&tmp;
char data_read[100];

//u32 *addr1=&tmp1;
//int l=40,lencheck=0;
char data[200];
//int z=1,,buffer_check=0;
int data_size=0,data_length,buffer_check1=0,allignment;
int start=0,end=0,j;
//char buffer[15000];
//u8 tmp1[48];
//int j;
//int s=0;
//int inc;
int no_of_lines=0,size,font;
int width=0,height,line_wise=0;
//int data_width,data_width_changed;
unsigned short int envy[50];
char tmp_buff[48];
//int start=0,end=0;

//-----------------------paper sensing variable--------------------------

struct file *f,*f1;
char buf1[1000000];
mm_segment_t fs;
mm_segment_t new;
char *value;
unsigned char data1,data2,data3;
char m;

unsigned int NoOfBytes;
unsigned int Temp=0;
unsigned char Dummy=0;

struct file *f2;
char buf[128];
char *paper_value;
char paper_buff[6];

unsigned int paper_NoOfBytes;
unsigned int paper_Temp=0;
unsigned char paper_Dummy=0;
   
//--------------------------To find the no of bytes in the protocol & height of the image--------------

int Noofbytes()
{
	
	NoOfBytes=((g[2]&0x0F)<<12);
		NoOfBytes|=((g[3]&0x0F)<<8);
		NoOfBytes|=((g[4]&0x0F)<<4);
		NoOfBytes|=(g[5]&0x0F);

		Dummy=NoOfBytes>>12&0x0F;
		Temp=Temp*0x0A+Dummy;
		Dummy=NoOfBytes>>8&0x0F;
		Temp=Temp*0x0A+Dummy;
		Dummy=NoOfBytes>>4&0x0F;
		Temp=Temp*0x0A+Dummy;
		Dummy=NoOfBytes&0x0F;
		Temp=Temp*0x0A+Dummy;
	NoOfBytes=Temp;

return NoOfBytes;

}

//-----------------------------------Motor Rotation-------------------------------------------------

void rotate(int rotate_loop)
{

for(loop=0;loop<rotate_loop;loop++)
{

if(rotate_pulse_count==1)
{
even_rotate();
rotate_pulse_count=2;
}

else if(rotate_pulse_count==2)
{
odd_rotate();
rotate_pulse_count=1;
}

}

}

//-----------------------------------PAPER sensing--------------------------------------------------

int paper_sensing()
{

	for(i=0;i<128;i++)
	{
	        buf[i] = 0;
	}

	while(file>0)
	{
	
	f1 = filp_open("/sys/bus/iio/devices/iio\:device0/in_voltage6_raw", O_RDONLY, 0);

		if(f == NULL)
	        {
//	        printk("FILE NOT PRESENT\n");
		file=file-1;
	        }

	        else
		{
//	        printk("FILE PRESENT\n");
		file=0;
		}
	}

	file=10;
        new = get_fs();
        set_fs(get_ds());

//	printk(".....checking.....\n");
        f1->f_op->read(f1, buf, 128, &f1->f_pos);
        set_fs(new);
	paper_value=buf;
	filp_close(f1,NULL);

	for(i=0;i<4;i++)
	{
	
		paper_buff[i]=*paper_value;
		paper_value++;
	}

	paper_NoOfBytes=((paper_buff[0]&0x0F)<<12);
		paper_NoOfBytes|=((paper_buff[1]&0x0F)<<8);
		paper_NoOfBytes|=((paper_buff[2]&0x0F)<<4);
		paper_NoOfBytes|=(paper_buff[3]&0x0F);

		paper_Dummy=paper_NoOfBytes>>12&0x0F;
		paper_Temp=paper_Temp*0x0A+paper_Dummy;
		paper_Dummy=paper_NoOfBytes>>8&0x0F;
		paper_Temp=paper_Temp*0x0A+paper_Dummy;
		paper_Dummy=paper_NoOfBytes>>4&0x0F;
		paper_Temp=paper_Temp*0x0A+paper_Dummy;
		paper_Dummy=paper_NoOfBytes&0x0F;
		paper_Temp=paper_Temp*0x0A+paper_Dummy;
	paper_NoOfBytes=paper_Temp;

//	printk("paper presence = %d\n",paper_Temp);


	if(paper_Temp==0)
	{
		paper_sensing();
	}


//	if(paper_Temp>=4000 && paper_Temp<=4150 )
//	{
	if(paper_Temp>=1000 && paper_Temp<=3800 )
	{

//		printk("paper presence = %d\n",paper_Temp);
//		printk("Success\n");
//		filp_close(f1,NULL);
		f2 = filp_open("/usr/share/status/PRINTER_status", O_WRONLY, 0644);
		fs = get_fs();
	        set_fs(get_ds());  
		f2->f_op->write(f2, "0", 1, &f2->f_pos);
		set_fs(fs);	
//		printk(KERN_ALERT "...file created...\n");	

		filp_close(f2,NULL);
	}

	else if(paper_Temp<=500)
	{
//		printk("paper presence = %d\n",paper_Temp);
//		printk("Failure\n");
//		filp_close(f1,NULL);
		f2 = filp_open("/usr/share/status/PRINTER_status", O_WRONLY, 0644);
		fs = get_fs();
		set_fs(get_ds());  
		f2->f_op->write(f2, "1", 1, &f2->f_pos);
		set_fs(fs);	
		g[0]=95;
//		printk(KERN_ALERT "...file created...\n");	

		filp_close(f2,NULL);
	}

	return paper_Temp;
}

//-------------------------------The program starts from here------------------------------------------

static void printer_prepare_spi_message(void)
{

data_read[48]=0,data_read[49]=1,data_read[50]=2,data_read[51]=3,data_read[52]=4,data_read[53]=5,data_read[54]=6,data_read[55]=7,data_read[56]=8,data_read[57]=9,data_read[65]=10,data_read[66]=11,data_read[67]=12,data_read[68]=13,data_read[69]=14,data_read[70]=15;

	paper_Temp=0;	 	
	spi_message_init(&printer_ctl.msg);
	printer_dev.spi_device->bits_per_word=8;

	for(i=0;i<48;i++)
	{
		tmp[i]=0;
	}

//	printk(KERN_ALERT "length is ...........   %d  \n",length);

	for(k=0;k<=1;k++)
	{
		g[k]=(**(buff))-32;
		++(*buff);
	}

//	paper_sensing();

//---------------------------------------PROTOCOL satarting------------------------------------------

if(g[0]==94)    //~ -----------start byte of protocol
{

paper_Temp=0;

	switch(g[1])   
	{

		//********************************** Paper feed **********************************

		case 50: // R - motor rotation

			for(k=2;k<=5;k++)
			{
				g[k]=(**(buff))-32;
				++(*buff);
			}

			Noofbytes();
//			printk("No of Bytes = %d\n",Temp);

			for(i=0;i<Temp;i++)
			{
				empty_rotate();
			}

			Temp=0;
		break;

		//******************** ENGLISH PRINTING *************************

		case 37:  //E ----------------------english printing  g[1]

			no_of_lines=0;

			for(k=2;k<=5;k++)
			{
				g[k]=(**(buff))-32;
				++(*buff);
//				printk("g[%d] = %d\n",k,g[k]);
			}

			Noofbytes();

//			printk("No of Bytes = %d\n",Temp);

			for(k=6;k<=Temp;k++)
			{
				g[k]=(**(buff))-32;
				++(*buff);
				if(g[k]==94)
				{
					no_of_lines++;
				}
			}

			if(g[Temp-1]==1)
			{
//				printk("last byte is received...\n");
				buffer_check1=6;

				for(line_wise=0;line_wise<no_of_lines;line_wise++)
				{
					data_size=1;
					do
					{
						data[data_size]=g[buffer_check1];
						data_size++;
						buffer_check1++;
					}while(g[buffer_check1]!=94);

				buffer_check1=buffer_check1+1;
				data_length=data_size;
//				printk("Data_length=%d\n",data_length);

			even=0;
			odd=1;

//---------------------------------------------------------------------------------------------

			if(data[data_size-1]==44)  // left -------------------------
			{
			allignment=1;
			}
			else if(data[data_size-1]==50)
			{
			allignment=2;
			}
			else if(data[data_size-1]==35)
			{
			allignment=3;
			}


				if(data[data_size-2]==51)  // small
				{
				size=1;
				}
				else if(data[data_size-2]==45)  //medium
				{
				size=2;
				}
				else if(data[data_size-2]==44)  //large
				{
				size=3;
				}

				if(data[data_size-3]==50)  // Regular
				{
				font=1;
				}
				else if(data[data_size-3]==34)  // Bold
				{
				font=2;
				}
				else if(data[data_size-3]==41)  // Italy
				{
				font=3;
				}

			

				for(height=0;height<19;height++)
				{

					for(width=1;width<=32 && width<(data_size-3);width++)
					{
						if(font==1)
						{
						envy[width]=envy_12[data[width]][height];
//						envy[width]=DejaVu_R16[data[width]][height];
						}
						else if(font==2)
						{
						envy[width]=LuxiMono12[data[width]][height];
//						envy[width]=DejaVu_R16[data[width]][height];
						}
						else if(font==3)
						{
						envy[width]=Envy_italic12[data[width]][height];
//						envy[width]=DejaVu_R16[data[width]][height];
						}
					}

					tmp_buff[0]=((envy[1] & 0xff00) >> 8 );
					tmp_buff[1]=((envy[1] & 0x00f0) | ((envy[2] & 0xf000) >> 12));
					tmp_buff[2]=((envy[2] & 0x0ff0) >> 4);

					tmp_buff[3]=((envy[3] & 0xff00) >> 8 );
					tmp_buff[4]=((envy[3] & 0x00f0) | ((envy[4] & 0xf000) >> 12));
					tmp_buff[5]=((envy[4] & 0x0ff0) >> 4);

					tmp_buff[6]=((envy[5] & 0xff00) >> 8 );
					tmp_buff[7]=((envy[5] & 0x00f0) | ((envy[6] & 0xf000) >> 12));
					tmp_buff[8]=((envy[6] & 0x0ff0) >> 4);

					tmp_buff[9]=((envy[7] & 0xff00) >> 8 );
					tmp_buff[10]=((envy[7] & 0x00f0) | ((envy[8] & 0xf000) >> 12));
					tmp_buff[11]=((envy[8] & 0x0ff0) >> 4);

					tmp_buff[12]=((envy[9] & 0xff00) >> 8 );
					tmp_buff[13]=((envy[9] & 0x00f0) | ((envy[10] & 0xf000) >> 12));
					tmp_buff[14]=((envy[10] & 0x0ff0) >> 4);

					tmp_buff[15]=((envy[11] & 0xff00) >> 8 );
					tmp_buff[16]=((envy[11] & 0x00f0) | ((envy[12] & 0xf000) >> 12));
					tmp_buff[17]=((envy[12] & 0x0ff0) >> 4);

					tmp_buff[18]=((envy[13] & 0xff00) >> 8 );
					tmp_buff[19]=((envy[13] & 0x00f0) | ((envy[14] & 0xf000) >> 12));
					tmp_buff[20]=((envy[14] & 0x0ff0) >> 4);

					tmp_buff[21]=((envy[15] & 0xff00) >> 8 );
					tmp_buff[22]=((envy[15] & 0x00f0) | ((envy[16] & 0xf000) >> 12));
					tmp_buff[23]=((envy[16] & 0x0ff0) >> 4);

					tmp_buff[24]=((envy[17] & 0xff00) >> 8 );
					tmp_buff[25]=((envy[17] & 0x00f0) | ((envy[18] & 0xf000) >> 12));
					tmp_buff[26]=((envy[18] & 0x0ff0) >> 4);

					tmp_buff[27]=((envy[19] & 0xff00) >> 8 );
					tmp_buff[28]=((envy[19] & 0x00f0) | ((envy[20] & 0xf000) >> 12));
					tmp_buff[29]=((envy[20] & 0x0ff0) >> 4);

					tmp_buff[30]=((envy[21] & 0xff00) >> 8 );
					tmp_buff[31]=((envy[21] & 0x00f0) | ((envy[22] & 0xf000) >> 12));
					tmp_buff[32]=((envy[22] & 0x0ff0) >> 4);

					tmp_buff[33]=((envy[23] & 0xff00) >> 8 );
					tmp_buff[34]=((envy[23] & 0x00f0) | ((envy[24] & 0xf000) >> 12));
					tmp_buff[35]=((envy[24] & 0x0ff0) >> 4);

					tmp_buff[36]=((envy[25] & 0xff00) >> 8 );
					tmp_buff[37]=((envy[25] & 0x00f0) | ((envy[26] & 0xf000) >> 12));
					tmp_buff[38]=((envy[26] & 0x0ff0) >> 4);

					tmp_buff[39]=((envy[27] & 0xff00) >> 8 );
					tmp_buff[40]=((envy[27] & 0x00f0) | ((envy[28] & 0xf000) >> 12));
					tmp_buff[41]=((envy[28] & 0x0ff0) >> 4);

					tmp_buff[42]=((envy[29] & 0xff00) >> 8 );
					tmp_buff[43]=((envy[29] & 0x00f0) | ((envy[30] & 0xf000) >> 12));
					tmp_buff[44]=((envy[30] & 0x0ff0) >> 4);

					tmp_buff[45]=((envy[31] & 0xff00) >> 8 );
					tmp_buff[46]=((envy[31] & 0x00f0) | ((envy[32] & 0xf000) >> 12));
					tmp_buff[47]=((envy[32] & 0x0ff0) >> 4);

/*
					tmp[0]=((envy[1] & 0xff00) >> 8 );
					tmp[1]=(envy[1] & 0x00ff);
					tmp[2]=((envy[2] & 0xff00) >> 8 );
					tmp[3]=(envy[2] & 0x00ff);
					tmp[4]=((envy[3] & 0xff00) >> 8 );
					tmp[5]=(envy[3] & 0x00ff);
					tmp[6]=((envy[4] & 0xff00) >> 8 );
					tmp[7]=(envy[4] & 0x00ff);
					tmp[8]=((envy[5] & 0xff00) >> 8 );
					tmp[9]=(envy[5] & 0x00ff);
					tmp[10]=((envy[6] & 0xff00) >> 8 );
					tmp[11]=(envy[6] & 0x00ff);
					tmp[12]=((envy[7] & 0xff00) >> 8 );
					tmp[13]=(envy[7] & 0x00ff);
					tmp[14]=((envy[8] & 0xff00) >> 8 );
					tmp[15]=(envy[8] & 0x00ff);
					tmp[16]=((envy[9] & 0xff00) >> 8 );
					tmp[17]=(envy[9] & 0x00ff);
					tmp[18]=((envy[10] & 0xff00) >> 8 );
					tmp[19]=(envy[10] & 0x00ff);
					tmp[20]=((envy[11] & 0xff00) >> 8 );
					tmp[21]=(envy[11] & 0x00ff);
					tmp[22]=((envy[12] & 0xff00) >> 8 );
					tmp[23]=(envy[12] & 0x00ff);
					tmp[24]=((envy[13] & 0xff00) >> 8 );
					tmp[25]=(envy[13] & 0x00ff);
					tmp[26]=((envy[14] & 0xff00) >> 8 );
					tmp[27]=(envy[14] & 0x00ff);
					tmp[28]=((envy[15] & 0xff00) >> 8 );
					tmp[29]=(envy[15] & 0x00ff);
					tmp[30]=((envy[16] & 0xff00) >> 8 );
					tmp[31]=(envy[16] & 0x00ff);
					tmp[32]=((envy[17] & 0xff00) >> 8 );
					tmp[33]=(envy[17] & 0x00ff);
					tmp[34]=((envy[18] & 0xff00) >> 8 );
					tmp[35]=(envy[18] & 0x00ff);
					tmp[36]=((envy[19] & 0xff00) >> 8 );
					tmp[37]=(envy[19] & 0x00ff);
					tmp[38]=((envy[20] & 0xff00) >> 8 );
					tmp[39]=(envy[20] & 0x00ff);
					tmp[40]=((envy[21] & 0xff00) >> 8 );
					tmp[41]=(envy[21] & 0x00ff);
					tmp[42]=((envy[22] & 0xff00) >> 8 );
					tmp[43]=(envy[22] & 0x00ff);
					tmp[44]=((envy[23] & 0xff00) >> 8 );
					tmp[45]=(envy[23] & 0x00ff);
					tmp[46]=((envy[24] & 0xff00) >> 8 );
					tmp[47]=(envy[24] & 0x00ff);
*/
/*
tmp[0]=((envy[1] & 0xff00) >> 8 );
tmp[1]=((envy[1] & 0x00c0) | ((envy[2] & 0xfc00) >> 10));
tmp[2]=(((envy[2] & 0x03c0) >> 2 ) | ((envy[3] & 0xf000) >> 12));
tmp[3]=(((envy[3] & 0x0fc0) >> 4 ) | ((envy[4] & 0xc000) >> 14));
tmp[4]=((envy[4] & 0x3fc0) >> 8 );

tmp[5]=((envy[5] & 0xff00) >> 8 );
tmp[6]=((envy[5] & 0x00c0) | ((envy[6] & 0xfc00) >> 10));
tmp[7]=(((envy[6] & 0x03c0) >> 2 ) | ((envy[7] & 0xf000) >> 12));
tmp[8]=(((envy[7] & 0x0fc0) >> 4 ) | ((envy[8] & 0xc000) >> 14));
tmp[9]=((envy[8] & 0x3fc0) >> 8 );

tmp[10]=((envy[9] & 0xff00) >> 8 );
tmp[11]=((envy[9] & 0x00c0) | ((envy[10] & 0xfc00) >> 10));
tmp[12]=(((envy[10] & 0x03c0) >> 2 ) | ((envy[11] & 0xf000) >> 12));
tmp[13]=(((envy[11] & 0x0fc0) >> 4 ) | ((envy[12] & 0xc000) >> 14));
tmp[14]=((envy[12] & 0x3fc0) >> 8 );

tmp[15]=((envy[13] & 0xff00) >> 8 );
tmp[16]=((envy[13] & 0x00c0) | ((envy[14] & 0xfc00) >> 10));
tmp[17]=(((envy[14] & 0x03c0) >> 2 ) | ((envy[15] & 0xf000) >> 12));
tmp[18]=(((envy[15] & 0x0fc0) >> 4 ) | ((envy[16] & 0xc000) >> 14));
tmp[19]=((envy[16] & 0x3fc0) >> 8 );

tmp[20]=((envy[17] & 0xff00) >> 8 );
tmp[21]=((envy[17] & 0x00c0) | ((envy[18] & 0xfc00) >> 10));
tmp[22]=(((envy[18] & 0x03c0) >> 2 ) | ((envy[19] & 0xf000) >> 12));
tmp[23]=(((envy[19] & 0x0fc0) >> 4 ) | ((envy[20] & 0xc000) >> 14));
tmp[24]=((envy[20] & 0x3fc0) >> 8 );

tmp[25]=((envy[21] & 0xff00) >> 8 );
tmp[26]=((envy[21] & 0x00c0) | ((envy[22] & 0xfc00) >> 10));
tmp[27]=(((envy[22] & 0x03c0) >> 2 ) | ((envy[23] & 0xf000) >> 12));
tmp[28]=(((envy[23] & 0x0fc0) >> 4 ) | ((envy[24] & 0xc000) >> 14));
tmp[29]=((envy[24] & 0x3fc0) >> 8 );

tmp[30]=((envy[25] & 0xff00) >> 8 );
tmp[31]=((envy[25] & 0x00c0) | ((start=((data_size-4)*10);
start=384-start;
start=((start/8));

for(i=0;i<start;i++)
{
tmp[i]=0;
}
j=0;
for(i=start;i<48;i++)
{
tmp[i]=tmp_buf[j];
j++;
}envy[26] & 0xfc00) >> 10));
tmp[32]=(((envy[26] & 0x03c0) >> 2 ) | ((envy[27] & 0xf000) >> 12));
tmp[33]=(((envy[27] & 0x0fc0) >> 4 ) | ((envy[28] & 0xc000) >> 14));
tmp[34]=((envy[28] & 0x3fc0) >> 8 );

tmp[35]=((envy[29] & 0xff00) >> 8 );
tmp[36]=((envy[29] & 0x00c0) | ((envy[30] & 0xfc00) >> 10));
tmp[37]=(((envy[30] & 0x03c0) >> 2 ) | ((envy[31] & 0xf000) >> 12));
tmp[38]=(((envy[31] & 0x0fc0) >> 4 ) | ((envy[32] & 0xc000) >> 14));
tmp[39]=((envy[32] & 0x3fc0) >> 8 );

tmp[40]=((envy[33] & 0xff00) >> 8 );
tmp[41]=((envy[33] & 0x00c0) | ((envy[34] & 0xfc00) >> 10));
tmp[42]=(((envy[34] & 0x03c0) >> 2 ) | ((envy[35] & 0xf000) >> 12));
tmp[43]=(((envy[35] & 0x0fc0) >> 4 ) | ((envy[36] & 0xc000) >> 14));
tmp[44]=((envy[36] & 0x3fc0) >> 8 );

tmp[45]=((envy[37] & 0xff00) >> 8 );
tmp[46]=((envy[37] & 0x00c0) | ((envy[38] & 0xfc00) >> 10));
tmp[47]=(((envy[38] & 0x03c0) >> 2 ) | ((envy[39] & 0xf000) >> 12));

*/


if(data_length>=32)
{
data_length=32;
}

if(allignment==1) // left allignment
{
for(i=0;i<48;i++)
{
tmp[i]=tmp_buff[i];
}
}

else if(allignment==2) // right allignment
{
start=((data_size-4)*12);
start=384-start;
start=((start/8));

for(i=0;i<start;i++)
{
tmp[i]=0;
}
j=0;
for(i=start;i<48;i++)
{
tmp[i]=tmp_buff[j];
j++;
}
}

else if(allignment==3) // center allignment
{
start=((data_size-4)*12);
start=384-start;
start=((start/2)+(start%2));
start=((start/8));
end=((data_size-4)*12);
end=((end/8)+(end%8));
end=(start+end);

for(i=0;i<start;i++)
{
tmp[i]=0;
}
j=0;
for(i=start;i<end;i++)
{
tmp[i]=tmp_buff[j];
j++;
}
for(i=end;i<48;i++)
{
tmp[i]=0;
}

}
					spi_write(printer_dev.spi_device, addr, 48);

					if(size==1) // 10 pulse
					{
					rotate(1);
					}
					else if(size==2) // 20 pulse
					{
					rotate(2);	
					}
					else if(size==3) // 30 pulse
					{
					rotate(3);
					}
					else if(size==4) // 40 pulse
					{
					rotate(4);
					}

			}

			for(i=0;i<48;i++)
			{
			tmp[i]=0;
			}

			spi_write(printer_dev.spi_device, addr, 48);

			rotate(2);

			}
			}

			Temp=0;
		break;
	
	// ----------------- IMAGE PRINTING ----------------------------

		case 41: // I - Image printing


			for(k=2;k<=6;k++)
			{
				g[k]=(**(buff))-32;
				++(*buff);
			}

//			if(g[6]==1)
//			{
//				printk("LOW BATTERY PRINTING\n");
//				low_bat=1;
//			}

//			else
//			{
//				printk("FULL BATTERY PRINTING\n");
				low_bat=0;
//			}

			Noofbytes();
			printk("No of Bytes = %d\n",Temp);

			for(i=0;i<48;i++)
			{
				tmp[i]=0;
			}

			for(i=0;i<1000000;i++)
			{
				buf1[i]=0;
			}

			f = filp_open("/usr/share/status/PRINTER_image", O_RDONLY, 0);

			if(f == NULL)
			{
//					printk(KERN_ALERT "filp_open error!!.\n");
			}
			else
			{
				fs = get_fs();
				set_fs(get_ds());
				f->f_op->read(f, buf1, 1000000, &f->f_pos);
				set_fs(fs);
				value=buf1;
			}

			even=0;
			odd=1;

			for(x=0;x<Temp;x++)
			{
				for(i=0;i<49;i++)
				{
					if((*value)!=10)
					{
						data1=*value;
						value++;
						y++;
						data2=*value;
						value++;
						y++;

						data1=data_read[data1];
						data2=data_read[data2];

						data3=(((data1&0x0f)<<4) | (data2&0x0f));
						w=7;
						q=0;

						while(w>=0)
						{
						    temp_image |= (((data3 >> q)&1)<< w);
						    w--;
						    q++;
						}
//temp_image=data3;

						tmp[i]=temp_image;
						temp_image=0;

					}

					else
					{
						i=49;
						value++;
					}

				}

				spi_write(printer_dev.spi_device, addr, 48);

				rotate(1);
			}

			for(i=0;i<48;i++)
			{
				tmp[i]=0;
			}

			spi_write(printer_dev.spi_device, addr, 48);

			rotate(5);

			gpio_direction_output(45,0);
			gpio_direction_output(44,0);
			gpio_direction_output(26,0);

			Temp=0;
			paper_Temp=0;

		break;

	}
}

	memset(printer_ctl.rx_buff, 0, SPI_BUFF_SIZE);
	spi_message_add_tail(&printer_ctl.transfer, &printer_ctl.msg);
}




//---------------------------------------SPI system calls------------------------------------------//

static ssize_t printer_write(struct file* F, const char *buf[], size_t count, loff_t *f_pos)
{

//printk(KERN_ALERT "count is ...........   %d  \n",count);
//printk(KERN_ALERT "*buf is ...........   %d  \n",*buf);
//printk(KERN_ALERT "buf+1  is ...........   %d  \n",*(buf+1));
//printk(KERN_ALERT "checking ...........   %s  \n",buf);

buff=&buf;
length=count;

ssize_t status = 0;
	
        if (down_interruptible(&printer_dev.spi_sem))
		return -ERESTARTSYS;

	if (!printer_dev.spi_device) 
        {		up(&printer_dev.spi_sem);
		return -ENODEV;
	}

	printer_prepare_spi_message();

	status = spi_sync(printer_dev.spi_device, &printer_ctl.msg);
	up(&printer_dev.spi_sem);


	if ( copy_from_user(printer_dev.user_buff, buf, count) ) 
        {
         return -EFAULT;
        }

if (status) 
	  {
		sprintf(printer_dev.user_buff, 
		"printer_do_one_message failed : %d\n",
		 status);
	
            }
	else 
        {
		sprintf(printer_dev.user_buff, 
		"Status: %d\n printing ..........  \n");	
	}
		
up(&printer_dev.fop_sem);
return count;
}

//------------------------
static ssize_t printer_read(struct file *filp, char __user *buff, size_t count,
			loff_t *offp)
{
	
        size_t len;
	ssize_t status = 0;

	if (!buff) 
		return -EFAULT;

	if (*offp > 0) 
		return 0;

	if (down_interruptible(&printer_dev.fop_sem)) 
		return -ERESTARTSYS;
/*

      	status = printer_do_one_message();

	if (status) 
        {
		sprintf(printer_dev.user_buff, 
			"printer_do_one_message failed : %d\n",
			status);
	}
	else 
        {
		sprintf(printer_dev.user_buff, 
			"Status: %d\n printing ..........  \n");	
	}

*/
	len = strlen(printer_dev.user_buff);
 
	if (len < count) 
		count = len;

	if (copy_to_user(buff, printer_dev.user_buff, count))  {
		printk(KERN_ALERT "printer_read(): copy_to_user() failed\n");
		status = -EFAULT;
	} else {
		*offp += count;
		status = count;
	}

	up(&printer_dev.fop_sem);
	return status;	
}

//---------------------
static int printer_open(struct inode *inode, struct file *filp)
{	
	int status = 0;

	if (down_interruptible(&printer_dev.fop_sem)) 
		return -ERESTARTSYS;

	if (!printer_dev.user_buff) 
        {
		printer_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);
		if (!printer_dev.user_buff) 
			status = -ENOMEM;
	}	

	up(&printer_dev.fop_sem);

	return status;
}

//------------------
static int printer_probe(struct spi_device *spi_device)
{
	if (down_interruptible(&printer_dev.spi_sem))
		return -EBUSY;

	printer_dev.spi_device = spi_device;

	up(&printer_dev.spi_sem);

	return 0;
}

//--------------------
static int printer_remove(struct spi_device *spi_device)
{
	if (down_interruptible(&printer_dev.spi_sem))
		return -EBUSY;
	
	printer_dev.spi_device = NULL;

	up(&printer_dev.spi_sem);

	return 0;
}

//--------------------
static int __init add_printer_device_to_bus(void)
{
	struct spi_master *spi_master;
	struct spi_device *spi_device;
	struct device *pdev;
	char buff[64];
	int status = 0;

	spi_master = spi_busnum_to_master(SPI_BUS);
	if (!spi_master) {
		printk(KERN_ALERT "spi_busnum_to_master(%d) returned NULL\n",
			SPI_BUS);
		printk(KERN_ALERT "Missing modprobe omap2_mcspi?\n");
		return -1;
	}

	spi_device = spi_alloc_device(spi_master);
	if (!spi_device) {
		put_device(&spi_master->dev);
		printk(KERN_ALERT "spi_alloc_device() failed\n");
		return -1;
	}

	spi_device->chip_select = SPI_BUS_CS1;

	/* Check whether this SPI bus.cs is already claimed */
	snprintf(buff, sizeof(buff), "%s.%u", 
			dev_name(&spi_device->master->dev),
			spi_device->chip_select);

	pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);
 	if (pdev) {
		/* We are not going to use this spi_device, so free it */ 
		spi_dev_put(spi_device);
		
		/* 
		 * There is already a device configured for this bus.cs  
		 * It is okay if it us, otherwise complain and fail.
		 */
		if (pdev->driver && pdev->driver->name && 
				strcmp(this_driver_name, pdev->driver->name)) {
			printk(KERN_ALERT 
				"Driver [%s] already registered for %s\n",
				pdev->driver->name, buff);
			status = -1;
		} 
	} else {
		spi_device->max_speed_hz = SPI_BUS_SPEED;
		spi_device->mode = SPI_MODE_3;
		spi_device->bits_per_word = 8;
		spi_device->irq = -1;
		spi_device->controller_state = NULL;
		spi_device->controller_data = NULL;
		strlcpy(spi_device->modalias, this_driver_name, SPI_NAME_SIZE);
		
		status = spi_add_device(spi_device);		
		if (status < 0) {	
			spi_dev_put(spi_device);
			printk(KERN_ALERT "spi_add_device() failed: %d\n", 
				status);		
		}				
	}

	put_device(&spi_master->dev);

	return status;
}

//--------------------
static struct spi_driver printer_driver = {
	.driver = {
		.name =	this_driver_name,
		.owner = THIS_MODULE,
	},
	.probe = printer_probe,
	.remove =printer_remove,	

};

//---------------------
static int __init printer_init_spi(void)       // initializing the spi
{
	int error;

	printer_ctl.tx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!printer_ctl.tx_buff) {
		error = -ENOMEM;
		goto printer_init_error;
	}

	printer_ctl.rx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!printer_ctl.rx_buff) {
		error = -ENOMEM;
		goto printer_init_error;
	}

	error = spi_register_driver(&printer_driver);
	if (error < 0) {
		printk(KERN_ALERT "spi_register_driver() failed %d\n", error);
		goto printer_init_error;
	}

	error = add_printer_device_to_bus();
	if (error < 0) {
		printk(KERN_ALERT "add_printer_to_bus() failed\n");
		spi_unregister_driver(&printer_driver);
		goto printer_init_error;	
	}

	return 0;

printer_init_error:

	if (printer_ctl.tx_buff) {
		kfree(printer_ctl.tx_buff);
		printer_ctl.tx_buff = 0;
	}

	if (printer_ctl.rx_buff) {
		kfree(printer_ctl.rx_buff);
		printer_ctl.rx_buff = 0;
	}
	
	return error;
}

//-----------------
static const struct file_operations printer_fops = {
	.owner =	THIS_MODULE,
	.read = 	printer_read,
	.open =		printer_open,	
        .write =        printer_write,
     };

//-----------------
static int __init printer_init_cdev(void)      //function to create device node
{
	int error;
	printer_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&printer_dev.devt, 0, 1, this_driver_name);
	if (error < 0) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", 
			error);
		return -1;
	}

	cdev_init(&printer_dev.cdev, &printer_fops);
	printer_dev.cdev.owner = THIS_MODULE;
	
	error = cdev_add(&printer_dev.cdev, printer_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(printer_dev.devt, 1);
		return -1;
	}	

	return 0;
}

//-------------------
static int __init printer_init_class(void)          //creating class
{
	printer_dev.class = class_create(THIS_MODULE, this_driver_name);

	if (!printer_dev.class) {
		printk(KERN_ALERT "class_create() failed\n");
		return -1;
	}

	if (!device_create(printer_dev.class, NULL, printer_dev.devt, NULL, 	
			this_driver_name)) {
		printk(KERN_ALERT "device_create(..., %s) failed\n",
			this_driver_name);
		class_destroy(printer_dev.class);
		return -1;
	}
	return 0;
}

//---------------------
static int __init printer_init(void)   
{
	memset(&printer_dev, 0, sizeof(printer_dev));   //allocating memory for printer_dev
	memset(&printer_ctl, 0, sizeof(printer_ctl));   

	sema_init(&printer_dev.spi_sem, 1);      //semaphore initialization function
	sema_init(&printer_dev.fop_sem, 1); 
	
	if (printer_init_cdev() < 0)       //function to create device nodes
		goto fail_1;
	
	if (printer_init_class() < 0)    //function to create class
		goto fail_2;

	if (printer_init_spi() < 0)      //function to initialize spi
		goto fail_3;

	//--------------------------- GPIO ----------------------------------------//

	gpio_request(48,"motor_line1");      // requesting the pin48 of gpio
	gpio_export(48,true);                // exporting it to the sysfs entry

	gpio_request(49,"motor_line2");
	gpio_export(49,true);

	gpio_request(60,"motor_line3");     // requesting the pin60 of gpio
	gpio_export(60,true);               // exporting it to the sysfs entry

	gpio_request(115,"motor_line4");
	gpio_export(115,true);

	gpio_request(116,"logic power");
	gpio_export(116,true);

	gpio_request(117,"Driver IC enable");
	gpio_export(117,true);

	gpio_request(67,"spi latch");
	gpio_export(67,true);

	gpio_request(45,"strobe1&2");
	gpio_export(45,true);

	gpio_request(44,"strobe3&4");
	gpio_export(44,true);

	gpio_request(26,"strobe5&6");
	gpio_export(26,true);


	gpio_direction_output(67,0); // ------------ SPI latch default---------
	gpio_direction_output(116,1);// --------------logic power -----------

return 0;

fail_3:
	device_destroy(printer_dev.class, printer_dev.devt);
	class_destroy(printer_dev.class);

fail_2:
	cdev_del(&printer_dev.cdev);
	unregister_chrdev_region(printer_dev.devt, 1);

fail_1:
	return -1;
}

//--------------------------------
static void __exit printer_exit(void)    // exit function to free all the resources 
{
	spi_unregister_device(printer_dev.spi_device);
	spi_unregister_driver(&printer_driver);

	device_destroy(printer_dev.class, printer_dev.devt);
	class_destroy(printer_dev.class);

	cdev_del(&printer_dev.cdev);
	unregister_chrdev_region(printer_dev.devt, 1);

	if (printer_ctl.tx_buff)
		kfree(printer_ctl.tx_buff);

	if (printer_ctl.rx_buff)
		kfree(printer_ctl.rx_buff);

	if (printer_dev.user_buff)
		kfree(printer_dev.user_buff);
}
//----------------------------------


module_init(printer_init);  // Driver always starts execution from here ( insmod ./printer.ko)
module_exit(printer_exit);  // Driver exectues this function while exit  (rmmod  printer.ko)           


MODULE_AUTHOR("Elango & SriNavamani");
MODULE_DESCRIPTION("printer module - SPI driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");




