#include <reg52.h>
#include <intrins.h>

#define uchar unsigned char		// 以后unsigned char就可以用uchar代替
#define uint  unsigned int		// 以后unsigned int 就可以用uint 代替


sbit LED     = P2^0;					// 模式指示灯，亮是自动模式，灭是手动模式
sbit Lamp    = P1^0; 					// 台灯控制引脚
sbit Key1    = P1^2;					// 按键1，模式切换按键
sbit Key2    = P1^3; 					// 按键2，亮度减少按键
sbit Key3    = P1^4;					// 按键3，亮度增加按键
sbit ADC_CS  = P2^1; 					// ADC0832的CS引脚
sbit ADC_CLK = P2^2; 					// ADC0832的CLK引脚
sbit ADC_DAT = P2^3; 					// ADC0832的DI/DO引脚
sbit Module  = P1^1;					// 人体红外检测模块


uchar gCount=0;								// 全局计数变量
uchar gIndex;									// 亮度变量，0是最暗，9是最亮，一共10档
uint  gTime=0;								// 计时变量，用于计时多久没检测到有人



/*********************************************************/
// 毫秒级的延时函数，time是要延时的毫秒数
/*********************************************************/
void DelayMs(uint time)
{
    uint i,j;
    for(i=0; i<time; i++)
        for(j=0; j<112; j++);
}



/*********************************************************/
// ADC0832的时钟脉冲
/*********************************************************/
void WavePlus()					// ADC0832的时钟脉冲
{
    _nop_();
    ADC_CLK = 1;
    _nop_();
    ADC_CLK = 0;
}



/*********************************************************/
// 获取指定通道的A/D转换结果
/*********************************************************/
uchar Get_ADC0832()
{
    uchar i;
    uchar dat1=0;
    uchar dat2=0;

    ADC_CLK = 0;				// 电平初始化
    ADC_DAT = 1;
    _nop_();
    ADC_CS = 0;
    WavePlus();					// 起始信号
    ADC_DAT = 1;
    WavePlus();					// 通道选择的第一位
    ADC_DAT = 0;
    WavePlus();					// 通道选择的第二位
    ADC_DAT = 1;

    for(i=0; i<8; i++)		// 第一次读取
    {
        dat1<<=1;
        WavePlus();
        if(ADC_DAT)
            dat1=dat1|0x01;
        else
            dat1=dat1|0x00;
    }

    for(i=0; i<8; i++)		// 第二次读取
    {
        dat2>>= 1;
        if(ADC_DAT)
            dat2=dat2|0x80;
        else
            dat2=dat2|0x00;
        WavePlus();
    }

    _nop_();						// 结束此次传输
    ADC_DAT = 1;
    ADC_CLK = 1;
    ADC_CS  = 1;

    if(dat1==dat2)			// 返回采集结果
        return dat1;
    else
        return 0;
}



/*********************************************************/
// 定时器初始化及串口初始化
/*********************************************************/
void TimerInit()				// 定时器初始化及串口初始化
{



    TMOD=0x21;					// 定时器1为工作方式2，定时器0为工作方式1 
    SM0=0;
    SM1=1;
    TH0  = 0xfc;					// 给定时器0的TH0装初值
    TL0  = 0x18;					// 给定时器0的TL0装初值
    TR0=1;
    IE=0X82;

    TH1=0xfd;					 //253
    TL1=0xfd;
    SCON=0x50;					//串口为工作方式1,REN=1允许串口接受数据
    PCON=0x00;					//SMOD=0
    IP=0x10;				    //串口通信为高优先级
    TR1=1;
    ES = 1;
    EA=1;
}



/*********************************************************/
// 手动控制
/*********************************************************/
void ManualControl()			// 手动控制
{
    // 亮度减少
    if(Key2==0)					// 如果按键2被按下去
    {
        if(gIndex>0)			// 只要当前亮度不为最低才能减少亮度
        {
            gIndex--;				// 亮度降低一档
            DelayMs(300);		// 延时0.3秒
        }
    }

    // 亮度增加
    if(Key3==0)					// 如果按键3被按下去
    {
        if(gIndex<9)			// 只要当前亮度不为最高才能增加亮度
        {
            gIndex++;				// 亮度增加一档
            DelayMs(300);		// 延时0.3秒
        }
    }
}



/*********************************************************/
// 自动控制
/*********************************************************/
void AutoControl(uchar num)							// 自动控制
{
    if(num<59)														// 最暗
        gIndex=0;
    else if((num>65)&&(num<81))						// 第二亮
        gIndex=1;
    else if((num>87)&&(num<103))					// 第三亮
        gIndex=2;
    else if((num>109)&&(num<125))
        gIndex=3;
    else if((num>131)&&(num<147))
        gIndex=4;
    else if((num>153)&&(num<169))
        gIndex=5;
    else if((num>175)&&(num<191))
        gIndex=6;
    else if((num>197)&&(num<213))
        gIndex=7;
    else if((num>219)&&(num<235))
        gIndex=8;
    else if(num>241)										 // 最亮
        gIndex=9;
}



/*********************************************************/
// 主函数
/*********************************************************/
void main()
{
    uchar ret;

    TimerInit(); 					// 定时器初始化及串口初始化

    LED=0;								// 指示灯点亮(自动模式指示灯)
    ret=Get_ADC0832();		// 获取AD采集结果(环境光照强度)
    AutoControl(ret);			// 上电先进行一次自动亮度控制
    AutoControl(ret+7);

    while(1)
    {
        /* 模式切换控制 */
        if(Key1==0)					// 如果按键1被按下去
        {
            LED=~LED;					// 切换LED灯状态
            DelayMs(10);			// 延时消除按键按下的抖动
            while(!Key1);			// 等待按键释放
            DelayMs(10);			// 延时消除按键松开的抖动
        }

        /* 亮度控制 */
        if(LED==1)							// 如果LED是灭的
        {
            ManualControl();			// 则进行手动控制
        }
        else										// 如果LED是亮的
        {
            if(gTime<10000)
            {
                ret=Get_ADC0832();		// 获取AD采集结果(环境光照强度)
                AutoControl(ret);			// 进行自动控制
                DelayMs(200);
            }
        }

        /*检测是否有人*/
        if(Module==1)
        {
            gTime=0;										// 检测到有人，则把10秒计时清零
        }
        if(gTime>10000)								// 如果gTime的值超过了10000
        {
            gTime=10000;								// 则把gTime的值重新赋值为10000，避免过大溢出
            gIndex=0;										// 如果10秒检测不到有人，则把台灯熄灭
        }
    }
}


/*********************************************************/
// 定时器0服务程序，1毫秒
/*********************************************************/
void Timer0(void) interrupt 1
{
    TH0  = 0xfc;						// 给定时器0的TH0装初值
    TL0  = 0x18;						// 给定时器0的TL0装初值	

    gTime++;							// 每1毫秒，gTime变量加1
    gCount++;							// 每1毫秒，gCount变量加1

    if(gCount==10)				// 如果gCount加到10了
    {
        gCount=0;						// 则将gCount清零，进入新一轮的计数
        if(gIndex!=0)				// 如果说台灯不是最暗的(熄灭)
        {
            Lamp=0;						// 则把台灯点亮
        }
    }
    if(gCount==gIndex)		// 如果gCount计数到和gIndex一样了
    {
        if(gIndex!=9)				// 如果说台灯不是最亮的
        {
            Lamp=1;						// 则把台灯熄灭
        }
    }
}


//串口服务的函数
void time() interrupt 4				//串口服务的函数
{
    if(RI)
    {
        RI=0;

        if(SBUF=='a')//亮度减
        {
            if(gIndex>0)			// 只要当前亮度不为最低才能减少亮度
            {
                gIndex--;				// 亮度降低一档		DelayMs(300);		// 延时0.3秒
            }
        }

        if(SBUF=='b')  //亮度加
        {
            if(gIndex<9)			// 只要当前亮度不为最高才能增加亮度
            {
                gIndex++;				// 亮度增加一档
                //	DelayMs(300);		// 延时0.3秒
            }
        }

        if(SBUF=='c') //自动模式
        {
            LED=0;
        }

        if(SBUF=='d') //手动模式
        {
            LED=1;
        }

		if(SBUF=='e') //熄灭
        {
            gIndex=0;
        }

		if(SBUF=='f') //最亮
        {
            gIndex=9;
        }
    }
}


