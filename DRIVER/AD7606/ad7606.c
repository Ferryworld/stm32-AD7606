#include "ad7606.h"

/* 复位变量宏定义 */
#define AD_RST PBout(8)

/* 过采样率控制引脚OS0宏定义 */
#define AD_OS0 PCout(0)

/* 过采样率控制引脚OS0，OS1，OS2宏定义*/
#define AD_OS1 PCout(1)
#define AD_OS2 PCout(2)

/* 设置AD采样范围。0为-5V~5V。1为-10V~10V。*/
#define AD_RANGE PCout(3)
/* SPI通信 片选信号*/
#define AD_CS PBout(9)
//#define AD_IRQ_FLAG PCout(6)
/*
 *简介：初始化AD模块各控制引脚
 *参数：无
*/
void ad7606_gpio_init()
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOB, ENABLE);  //使能GPIOC时钟

    // PC0,PC1,PC2引脚初始化，用于控制过采样倍数。OS0，OS1，OS2
    //初始化设置为下拉，默认采样倍数为0.
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       //普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // 100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;      //下拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);              //初始化GPIO

    GPIO_ResetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_6);  //三个引脚再次设置低位输出

    // PC3引脚初始化设置，用于控制采样范围，默认为上拉，10V。
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;           //对应RANGE引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       //普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // 100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);              //初始化GPIO

    GPIO_SetBits(GPIOC, GPIO_Pin_3);

    // PB8引脚初始化。控制AD模块复位重启。默认拉低。PB9为SPI1片选信号
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;  //对应RST引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;           //普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      // 100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //下拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);                  //初始化GPIO

    GPIO_ResetBits(GPIOB, GPIO_Pin_8);
}

/*
 *简介：初始化PWM波输出引脚，控制A/D芯片采样   输出引脚为TIM4的CH1和CH2，对应PB6和PB7引脚
 *输入参数:arr自动重装值
 *输入参数:psc。定时器时钟为84MHz，分频系数一般设定为84，计数频率为1MHz,设定重装在值为arr,则PWM的频率为1MHz/arr.arr=50的时候为20KHz。
*/
void ad7606_pwm_init(u32 arr, u32 psc)
{
    //此部分需手动修改IO口设置

    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);   // TIM14时钟使能
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  //使能PORTF时钟

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_TIM4);  // GPIOB6复用为定时器14

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;           // GPIOB6
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  //速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);              //初始化PB6

    TIM_TimeBaseStructure.TIM_Prescaler = psc;                   //定时器分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数模式
    TIM_TimeBaseStructure.TIM_Period = arr;                      //自动重装载值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);  //初始化定时器14

    //初始化TIM14 Channel1 PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;              //选择定时器模式:TIM脉冲宽度调制模式2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  //比较输出使能
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;       //输出极性:TIM输出比较极性低

    TIM_OC1Init(TIM4, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM1 4OC1

    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);  //使能TIM14在CCR1上的预装载寄存器

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_TIM4);  // GPIOF9复用为定时器14

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;           // GPIOB7
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  //速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);              //初始化PF9

    //初始化TIM14 Channel1 PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;              //选择定时器模式:TIM脉冲宽度调制模式2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  //比较输出使能
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;       //输出极性:TIM输出比较极性低

    TIM_OC2Init(TIM4, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM1 4OC1

    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);  //使能TIM14在CCR1上的预装载寄存器

    TIM_ARRPreloadConfig(TIM4, ENABLE);  // ARPE使能

    TIM_Cmd(TIM4, ENABLE);  //使能TIM14
}

/*
 *简介：芯片重启引脚RST设置。RST拉高至少50ns，然后拉低
 *参数：无
*/
void ad7606_rst(void)
{
    AD_RST = 1;
    delay_us(1);
    AD_RST = 0;
}

/*
 *简介：AD模块采样率设置
 *参数:mul_os.你想要设置的过采样倍数，0.2,4,8,16,32,64。如果输入的不是这些数中的，就默认为0。
*/
void ad7606_os_set(u8 mul_os)
{
    u16 num_os = 0;
    switch (mul_os)
    {
        case 0:
            num_os = 0x000;
            break;
        case 2:
            num_os = 0x001;
            break;
        case 4:
            num_os = 0x010;
            break;
        case 8:
            num_os = 0x011;
            break;
        case 16:
            num_os = 0x100;
            break;
        case 32:
            num_os = 0x101;
            break;
        case 64:
            num_os = 0x110;
            break;
        default:
            num_os = 0x000;
            break;
    }
    AD_OS0 = num_os & 0x001;
    AD_OS1 = (num_os >> 4) & 0x001;
    AD_OS2 = (num_os >> 8) & 0x001;
}

/*
 *简介：设置AD模块的采样范围.
 *参数：num_range,范围设定值。5，设置参数范围为-5V~5V.10,参数范围为-10V~10V。默认为10。
*/
void ad7606_range_set(u8 num_range)
{
    if (num_range == 5)
    {
        AD_RANGE = 0;
    }
    else
        AD_RANGE = 1;
}
/*
 *简介：开发板SPI1初始化，用于和AD模块通信。SPI设置，CPHA=0，在SCK第一个条边沿采样数据。CPOL=1，空闲状态，高点平。
 *参数：无
*/
void spi1_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);  //使能GPIOB时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);   //使能SPI1时钟

    // GPIOFB3,4,5初始化设置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;  // PB3~5复用功能输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;                            //复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;                          //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;                      // 100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;                            //上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);                                  //初始化

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);  // PB3复用为 SPI1
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);  // PB4复用为 SPI1
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);  // PB5复用为 SPI1

    //这里只针对SPI口初始化
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE);   //复位SPI1
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, DISABLE);  //停止复位SPI1

    SPI_InitStructure.SPI_Direction =
        SPI_Direction_2Lines_FullDuplex;  //设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;       //设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;  //设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;         //串行同步时钟的空闲状态为高电平
    SPI_InitStructure.SPI_CPHA =
        SPI_CPHA_2Edge;  //串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    SPI_InitStructure.SPI_NSS =
        SPI_NSS_Soft;  // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    SPI_InitStructure.SPI_BaudRatePrescaler =
        SPI_BaudRatePrescaler_8;  //定义波特率预分频的值:波特率预分频值为256
    SPI_InitStructure.SPI_FirstBit =
        SPI_FirstBit_MSB;  //指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial = 7;  // CRC值计算的多项式
    SPI_Init(SPI3, &SPI_InitStructure);       //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

    SPI_Cmd(SPI3, DISABLE);
    SPI_Cmd(SPI3, ENABLE);  //使能SPI外设
}

/*
 *SPI1速度设置函数
 *SPI速度=fAPB2/分频系数
 *@ref SPI_BaudRate_Prescaler:SPI_BaudRatePrescaler_2~SPI_BaudRatePrescaler_256
 *fAPB2时钟一般为84Mhz�
*/
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));  //判断有效性
    SPI1->CR1 &= 0XFFC7;                                             //位3-5清零，用来设置波特率
    SPI1->CR1 |= SPI_BaudRatePrescaler;                              //设置SPI1速度
    SPI_Cmd(SPI1, ENABLE);                                           //使能SPI1
}

/*
 *简介：定时器5通道1输入捕获配置
 *输入参数arr：自动重装值(TIM2,TIM5是32位的!!)
 *输入参数psc：时钟预分频�
*/
void ad7606_busy_cap(u32 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_ICInitTypeDef TIM5_ICInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);   // TIM5时钟使能
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //使能PORTA时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;           // GPIOA0
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  //速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;      //下拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);              //初始化PA0

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM5);  // PA0复用位定时器5

    TIM_TimeBaseStructure.TIM_Prescaler = psc;                   //定时器分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数模式
    TIM_TimeBaseStructure.TIM_Period = arr;                      //自动重装载值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

    //初始化TIM5输入捕获参数
    TIM5_ICInitStructure.TIM_Channel = TIM_Channel_1;                 // CC1S=01 	选择输入端 IC1映射到TI1上
    TIM5_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;     //上升沿捕获
    TIM5_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;  //映射到TI1上
    TIM5_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;            //配置输入分频,不分频
    TIM5_ICInitStructure.TIM_ICFilter = 0x00;                         // IC1F=0000 配置输入滤波器 不滤波
    TIM_ICInit(TIM5, &TIM5_ICInitStructure);

    TIM_ITConfig(TIM5, TIM_IT_Update | TIM_IT_CC1, ENABLE);  //允许更新中断 ,允许CC1IE捕获中断

    TIM_Cmd(TIM5, ENABLE);  //使能定时器5

    NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  //抢占优先级2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;         //子优先级2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                            //根据指定的参数初始化VIC寄存器、
}
/*
*SPI1 读写一个字节
*TxData:要写入的字节
*返回值:读取到的字数
*/
u16 SPI3_Read2Byte(void)
{
    u8 retry = 0;
    SPI_Cmd(SPI3, ENABLE);  //使能SPI外设
    AD_CS = 0;
    SPI_I2S_SendData(SPI3, 0x0000);
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET)
    {
        retry++;
        if (retry > 200)
            return 0x1010;
    }
    AD_CS = 1;
    SPI_Cmd(SPI3, DISABLE);  //使能SPI外设
    return SPI_I2S_ReceiveData(SPI3);
}
//定时器5中断服务程序
void TIM5_IRQHandler(void)
{
    u8 i;
    u16 data[8];
    if (TIM_GetITStatus(TIM5, TIM_IT_CC1) != RESET)  //捕获1发生捕获事件
    {
        // AD_IRQ_FLAG=1;
        for (i = 0; i < 8; i++)
        {
            data[i] = SPI3_Read2Byte();
        }
        for (i = 0; i < 8; i++)
        {
            printf("%x次的AD值为:%x\r\n", i, data[i]);
            delay_us(1);
        }
    }
    TIM_ClearITPendingBit(TIM5, TIM_IT_CC1 | TIM_IT_Update);  //清除中断标志位
                                                              // AD_IRQ_FLAG=0;
}
