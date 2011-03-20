
#include "ATtiny13prog.h"
//�����ʹ��˵�����ڲ��Ķ�����������ͷ�ļ���

#define overtmval 80  //����.�����룽������


UCHAR volatile sign = 0;
UCHAR rxtime = asUART_TOP_DEFAULT;//�˴��������ͨѶ��ʱ�䳤�ȡ�
UCHAR step = 0;//�ڳ������й���������ָʾ��ǰ����ִ�еĲ������������ģ��֮���ܹ��໥��Ϲ���
UCHAR txval,rxval,txstep,rxstep;


UCHAR overtm = overtmval;
UCHAR tmloval;
UCHAR temp =0;


 /*
   *  Rxd ����ͬʱʱ��  INT0  �仯������������
   *
   *       +------+\\\\\\\\\\\ +-------+
   *       |             |              |              |
   * ---+            +-------+              +--------
   *  change |              |
   *           read bit     readbit
   */
SIGNAL(SIG_INTERRUPT0)  
{
      //RXd �仯,����INT0 �ж�
	UCHAR bak,half;
	bak = TCNT1;
	half = asUART_TIMER_TOP /2;
	setrx;
	clrint0; //���úü���Ϳ��Թر��ж���

	
     //Rxd �������bit�Ķ�ȡʱ�������
     //UART ���ڵ�һ��,�����������OCR1A
     //����1/2���ں����SIG_OUTPUT_COMPARE1A
	if(bak >= half)
	{
		OCR1A = bak - half;
	}
	else
	{
		OCR1A = bak + half;
	}
	rxstep = 0;
}


/*  ʱ�Ӷ�ʱ �ж�
  *  �����ڹ涨ʱ����
  *  ��ȡrxd pin ��ֵ,������ֽ�
  *
  */
SIGNAL(SIG_OUTPUT_COMPARE1A)
{
	static UCHAR  rxmsk = 1;
	static UCHAR temp = 0;//����һ�������õ���λ��Ԫ��
	if(testrx)
	{
		rxstep ++;
		switch(rxstep)
		{
			case 1:
			temp = 0;//rxval = 0;
			rxmsk = 1;//�ڴ˴����յ���������λ
			if(testrxd)
			{//�˴���������λ�����Զ��ָ��������״̬��
				clrintf;
				clrrx;
				setint0;
				break;
			}
			if(testrxok)
				seterr;
			clrrxok;
			break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			if(testrxd)
			{
				temp |= rxmsk;
//				rxval |= rxmsk;
			}
			rxmsk <<= 1;
			break;
			case 10:
			setrxok;
			rxval = temp;
			default:
			clrintf;
			clrrx;
			setint0;
		}
	}
}

/*
  *  ʱ�Ӷ�ʱ�ж�, 
  *  �Ժ㶨���ٶȷ���bit��txd, 
  *  ʹ��OC1B ����
  */
INTERRUPT(SIG_OUTPUT_COMPARE1B)//����ж���Ҫ�����������
{
	static unsigned int timehi = 25;
	static UCHAR  timelo = 100;
	static UCHAR txmask = 1;
	if(testtx)
	{
		txstep ++;
		switch(txstep)
		{
			case 1:
			OC1B_MODE_CLR;//��������λ
			txmask = 1;
			break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			if(txval & txmask)
			{
				OC1B_MODE_SET;
			}
			else
			{
				OC1B_MODE_CLR;
			}
			txmask <<= 1;
			break;
			case 10:
			OC1B_MODE_SET;//����ֹͣλ
			break;
			default:
			clrtx;
		}
	}
	wdt_reset();
	if(testtx || testrx || testt0ie)
	{
		setsta;
		timehi = 25;
		timelo = tmloval;
	}
	else
	{
		if(!(tstint0))//����ڽ��в����ʼ���������Ҫ�Զ��ָ��� || testt0ie
		{
			clrintf;
			NOP;
//			setsta;
			setint0;//������Ϊ�˷�ֹ�����ⲿӰ��ر����ж���ɽ��ղ��������ӵġ�
		}
		if(!(timelo--))
		{
			timelo = tmloval;
//			set01s;
			if(!(timehi--))
			{
				timehi = 25;
				if(testpro)
				{
					if(!(overtm--))
					{
						outprog();
					}
				}
				if(!teststa)
				{
					clrsta;
				}
				else
				{
					setsta;
				}
			}
		}
	}
}


/*
  *  chkrx  ʹ����� 2ms �Ķ�ʱ����
  *  ����UART �����źŵļ��
  */
SIGNAL(SIG_OVERFLOW0)
{
	seterr;
	TCCR0 = 0;//ʱ��1�رա�
	clrt0ie;//�ر�ʱ��0����ж�
	//Ϊ�˼ӿ����ٶȣ��ڴ˲���ǿ���������ŵķ������˳���ⲿ�֡�
//	TCCR1A = TCCR1A_MODE;
	TCCR1B = TCCR1B_MODE;
	UART_PORT |= (1 << rxd);
	UART_DDR |= (1 << rxd);
}


/*SIGNAL(SIG_OVERFLOW1)
{
	step ++;
}*/



void init(void)
{
	//�ڴ˽�����������
	HVSPI_PORT 	= 	SPI_PORT_INIT;
	HVSPI_DDR	=	SPI_DDR_INIT;//
	UART_PORT	=	UART_PORT_INIT;//
	UART_DDR 	= 	UART_DDR_INIT;//
	tmloval =	100;
//���¿�ʼ�Զ�ʱ����������
	TIFR |= 0B01000100;//��Ϊ����ָ����Ȼ�ܹ������ǣ���ͬʱҲ��������ָ��ʱ�����жϡ�
	OCR1A = 0;
	OCR1B = 0;//rxtime /2 - 5;//��Ϊ���������8M��Ƶ�ʣ������������е�ʱ������˰˷�Ƶ��
	asUART_TIMER_TOP = asUART_TOP_DEFAULT;  
	TCCR1A = TCCR1A_MODE;
	TCCR1B = TCCR1B_MODE;
	TCNT1 = 0;
	clrt0ie;
	sett1ae;
	sett1be;
	setby;
	setint0;
//	sett1ie;//��ʱ��1����ж�
//ע���ڴ˴򿪿��Ź���������ʱ��Ϊ16����
	wdt_enable(WDTO_15MS);//��ͨѶ�������жԿ��Ź����и�λ��
	sei();
//	wdt_reset();
}


UCHAR receive(void)//�����������յĲ��֣�Ҳ�����Լ�����������������
{
	while(!testrxok);
	clrrxok;
	return(rxval);
}

void send(UCHAR sval)
{
	while(testtx);//�ȴ���һ�����͵�������ɡ�
//	sett1ae;
	txval = sval;
	txstep = 0;
	settx;
}

void inprog(void)
{
	UCHAR i;
	if(!(testpro))
	{
		HVSPI_DDR  = SPI_DDR_INIT_POWER;
		HVSPI_PORT = SPI_PORT_INIT_POWER;//��Դ�ϵ磬��λ�õ͡�
		NOP;
		for(i = 0; i<4;i++)
		{
			setsci;
			NOP;
			clrsci;
		}
		setHV;//��12V��ѹ��
		_delay_us(2);
		sdoin;//sdo��Ϊ�������š�
		HVSPI_PORT |= (1 << sdo);//��SDO���������ܡ�
		_delay_ms(50);//�ӳ���ʱ�ȴ�����ʱ��Ϊ50���롣
		overtm = overtmval;
		setpro;//�Ѿ������˱��״̬��
	}
}

void outprog(void)
{
	if(testpro)
	{
		clrsci;
		NOP;
		HVSPI_DDR &= ~(1 << HVout);
		HVSPI_PORT  &= ~(1 << HVout);
		_delay_us(1);
//		clrvcc;
//		_delay_us(10);
		HVSPI_PORT 	= 	SPI_PORT_INIT;
		HVSPI_DDR	=	SPI_DDR_INIT;//
		clrpro;//�����Ѿ��˳��˱��״̬��
	}
}

UCHAR ioopt(UCHAR sdival,UCHAR siival)//��������ⲿ��Ҫ��̵�оƬ���в���������ʱ��Ҳ������
{//�ڲ����ڷ��������������ݲ���Ļ���sdo�����¸��ֽ��з����ϸ�sdi�����ݡ�
	UCHAR sdoval = 0;
	UCHAR step = 0;
	UCHAR inmask = 0x80,outmask = 0x80;
	if(!(testpro))
	{
		inprog();
//		_delay_ms(100);//�������ӳ������������ʱ�䣬��ʹ����ź��ȶ���
	}
	overtm = overtmval;
	for(step = 0; step < 11;step++)
	{
		switch(step)
		{
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			if(siival & outmask)
			{
				setsii;
			}
			else
			{
				clrsii;
			}
			if(sdival & outmask)
			{
				setsdi;
			}
			else
			{
				clrsdi;
			}
			outmask >>= 1;
			break;
			default:
			clrsdi;
			clrsci;
		}
		_delay_us(1);
		setsci;//��������������
		_delay_us(1);
		clrsci;//�½��ص�ʱ������ȶ���׼����ȡ�������
		if(inmask)
		{
			if(testsdo)//�����Ƕ�ȡ����
			{
				sdoval |= inmask;
			}
			inmask >>= 1;//���������ƹ����и�λ��д0,�����Ժ�����λ�����ж�������ź���Ӱ�졣
		}
	}
	return(sdoval);
}

UCHAR clrchip(void)
{
	ioopt(0x80,0x4c);
	ioopt(0x0,0x64);
	ioopt(0x0,0x6c);
	while(!(testsdo));
	return(ioopt(0x0,0x4c));
//	send(0x0);
//	outprog();
}

UCHAR readfl(void)
{
	ioopt(0x4,0x4c);
	ioopt(0x0,0x68);
	return(ioopt(0x0,0x6c));
//	outprog();
}

UCHAR readfh(void)
{
	ioopt(0x4,0x4c);
	ioopt(0x0,0x7a);
	return(ioopt(0x0,0x7e));
//	outprog();
}

UCHAR readlb(void)
{
	ioopt(0x4,0x4c);
	ioopt(0x0,0x78);
	return(ioopt(0x0,0x7c));
//	outprog();
}

void readsb(void)
{
	UCHAR i;
	ioopt(0x8,0x4c);
	for(i = 0 ;i < 3;i++)
	{
		ioopt(i,0x0c);
		ioopt(0x0,0x68);
		send(ioopt(0x0,0x6c));
	}
//	outprog();
}

UCHAR readcb(void)
{
	ioopt(0x8,0x4c);
	ioopt(0x0,0x0c);
	ioopt(0x0,0x78);
	return(ioopt(0x0,0x7c));
//	outprog();
}


UCHAR writefl(UCHAR sdival)
{
	ioopt(0x40,0x4c);
	ioopt(sdival,0x2c);
	ioopt(0x0,0x64);
	_delay_ms(5);
	temp = ioopt(0x0,0x6c);
	while(!(testsdo));
	return(temp);
//	outprog();
//	send(0);	
}

UCHAR writefh(UCHAR sdival)
{
	ioopt(0x40,0x4c);
	ioopt(sdival & 0x1f,0x2c);
	ioopt(0x0,0x74);
	_delay_ms(5);
	temp = ioopt(0x0,0x7c);
	while(!(testsdo));
	return(temp);
//	outprog();
//	send(0);	
}

UCHAR writelb(UCHAR sdival)
{
	ioopt(0x20,0x4c);
	ioopt(sdival & 0x3,0x2c);
	ioopt(0x0,0x64);
	temp = ioopt(0x0,0x6c);
	while(!(testsdo));
	return(temp);
//	outprog();
//	send(0);	
}

UCHAR autounld(void)//�Զ���������
{
	UCHAR readval;
	readval = readlb();//�ȶ�ȡ����λ
	if(~(readval | 0xfc))
	{//���оƬ��������Ҫ�ȶ�оƬ���в���������
		clrchip();
	}
	//д����˿λ�ĵ�λ����
	if(readfl() != 0b01101010)
	{
		writefl(0b01101010);
	}
	//д����˿λ�ĸ�λ����
	if(readfh() != 0xff)
	{//����˿λ�ָ�����ֵ
		writefh(0xff);
	}
	outprog();
	return(0);
}

void option(void)
{
	UCHAR sdival;
	UCHAR siival;
	UCHAR sdoval;
	sdival = receive();
	siival = receive();
	//���½����յ��������͵����оƬ��ȥ��ͬʱ�����ص����ݶ�������
	if(siival < 0x80)
	{
		sdoval = ioopt(sdival,siival);
		send(sdoval);
	}
	else
	{//���ָ���ֽ����λΪ1��ʾҪ��������������ܡ�
		switch(siival)
		{
			case 0x81://�����״̬
			inprog();
			send(sign);
			break;
			case 0x82://�˳����״̬
			outprog();
			send(sign);
			break;
			case 0x83://���ص�ǰ״̬
			send(sign);
			break;
			case 0x84://ֱ�ӿ��ƶ˿�A�ķ���
			send(HVSPI_DDR);
			HVSPI_DDR = sdival;
			break;
			case 0x85://ֱ�ӿ��ƶ˿�A������
			send(HVSPI_PORT);
			HVSPI_PORT = sdival;
			break;
			case 0x86://
			send(HVSPI_PIN);
			break;
			case 0x87://
			send (HVSPI_PORT);
			break;
			case 0x88://
			send(HVSPI_DDR);
			break;
			case 0x90:
			send(readfl());
			break;
			case 0x91:
			send(readfh());
			break;
			case 0x92:
			send(readlb());
			break;
			case 0x93:
			send(readcb());
			break;
			case 0x94:
			readsb();
			break;
			case 0xa0:
			send(writefl(sdival));
			break;
			case 0xa1:
			send(writefh(sdival));
			break;
			case 0xa2:
			send(writelb(sdival));
			break;
			case 0xa3:
			send(clrchip());
			break;
			case 0xa4:
			send(autounld());//�Զ�����˿λ�ָ�����ֵ�����оƬ����������Զ�ִ�в���������
			break;
			default://������ַǷ��������˳������¼��ͨѶ������״̬��
			seterr;
		}
		if(siival != 0x81)
		{//���Ǳ�֤���Խ����ֶ����ԵĲ��֣����������������Ὣ���ģʽ�رգ��ֶ���������ֻ���ڣ����������
			outprog();
		}
	}

}

void chkrx(void)
{
//	setbusy;
//	step = 1;
	//���������²��ֿ�ʼ���ⲿ�Ĵ��������źŽ��м�⡣
	//���Զ������ɺ����Ὣ�����Ĳ������͸���λ����
	/* ���δ��������⣬�򲻻ᷢ������*/
	int val = ftime;//2500;
	TCCR0 = 0;     /* Timer0 stop */
	TIFR |= (1 << TOV0);  /*CLEAR timer0 overflow  flag*/
	TCNT0 = T0val; 
	clrint0;
	clrerr;
	sett0ie;//��ʱ��0����ж�
	if(testrxd)
	{
		while(testrxd);
		TCCR0 = TCCR0val;
		TCCR1B = 0;
		TCNT1 = 0;
		wdt_reset();
		while(!(testrxd));
		TCCR0 = 0;//ʱ��0�ر�
		TCCR1B = TCCR1B_MODE;
		wdt_reset();
		if(testerr)
		{
			UART_DDR &= ~(1 << rxd);
			rxtime = asUART_TOP_DEFAULT;
		}
		else
		{
			if(TCNT0 > mintm)
			{
				rxtime = TCNT0;
				temp = 0;
				while(val > 0)//оƬû�г˳���ָ������ü���������
				{
					temp ++;
					val -= rxtime;
				}
				tmloval = temp;
				send(rxtime);
			}
			else
			{
				seterr;
				rxtime = asUART_TOP_DEFAULT;
			}
		}
		asUART_TIMER_TOP = rxtime;
	}
	else
	{
		seterr;
	}
	clrintf;
	clrt0ie;//�رն�ʱ0�ж�
	setint0;
	if(!(testerr))//���ͨѶ�Զ�У�������ʳɹ��Ļ��������һ��ͨѶ���顣�Ա�֤ͨѶ��ȷ��
	{
		send(receive());
		if( rxval != 0x55)
		{
//			send(0xff);//ͨѶ�����ʱ���͵����ݡ�
			while(testtx);//�ȴ�������ɺ�����ָ���
			seterr;
			rxtime = asUART_TOP_DEFAULT;
			asUART_TIMER_TOP = rxtime;
			TCNT1 = 0;
		}
	}
//	clrbusy;
}

int main(void)
{
	init();//�ڳ�ʼ�������лὫ�ⲿ�ĵƹرյ��ġ�
	while(1)
	{
		seterr;
		while(testerr)
		{
			//���²��ִ�����׼����û��ͨѶʱ��ֻ��һ���������Զ��ָ��õġ�
			if(!testkey)//����������
			{
				_delay_ms(20);
				if(!testkey)
				{
					autounld();
					while(!testkey);
					_delay_ms(800);
				}
			}
			chkrx(); //�ڲ�clrerr, ���û�м�⵽,��������err
		}
		while(!(testerr))
		{
			 option(); //����������λ�����������
		}
	}
//	return(0);
}
