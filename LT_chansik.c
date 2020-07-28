#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>


#define MOTOR_PORT      PORTD
#define MOTOR_PORT_DDR   DDRD

#define SENSOR_PORT      PINC
//STATE 
#define STATE_STOP 0x00
#define STATE_RUN 0x01
#define STATE_LEFT 0x02
#define STATE_RIGHT 0x03

volatile unsigned char STATE;//STATE 외부 스위치로 바꾸기 위해 
/////////////////////////////////
volatile int FLAG_RUN;
volatile int FLAG_LEFT;
volatile int FLAG_RIGHT;
volatile int FLAG_STOP;
//////////////////////////////////
void port_init(void)
{
   PORTA = 0x00;
   DDRA = 0x00;
   PORTB = 0x00;
   DDRB = 0x00;
   PORTC = 0x00;
   DDRC = 0x00;
   PORTD = 0x00;
   DDRD = 0x00;
   PORTE = 0x00;
   DDRE = 0x00;//LED 로 STATE 보여주기 위함
   PORTF = 0xF0;
   DDRF = 0x00;
   PORTG = 0x00;
   DDRG = 0x03;
}

void init_devices(void)
{
   cli(); //disable all interrupts
   XDIV = 0x00; //xtal divider
   XMCRA = 0x00; //external memory
   port_init();

   MCUCR = 0x00;
   EICRA = 0x00; //extended ext ints
   EICRB = 0x0F; //extended ext ints
   EIMSK = 0x30;
   TIMSK = 0x00; //timer interrupt sources
   ETIMSK = 0x00; //extended timer interrupt sources
   sei(); //re-enable interrupts
   //all peripherals are now initialized
}


/* Stepping Motor derive---------------------------*/
unsigned char  LEFTmotorOneClock(unsigned char step, char dir)
{
   step = step & 0x0f;
   if (dir) {
      switch (step) {
      case 0x09: step = 0x01; break;
      case 0x01: step = 0x03; break;
      case 0x03: step = 0x02; break;
      case 0x02: step = 0x06; break;
      case 0x06: step = 0x04; break;
      case 0x04: step = 0x0c; break;
      case 0x0c: step = 0x08; break;
      case 0x08: step = 0x09; break;
      default: step = 0x0c; break;
      }
   }
   else {
      switch (step) {
      case 0x09: step = 0x08; break;
      case 0x01: step = 0x09; break;
      case 0x03: step = 0x01; break;
      case 0x02: step = 0x03; break;
      case 0x06: step = 0x02; break;
      case 0x04: step = 0x06; break;
      case 0x0c: step = 0x04; break;
      case 0x08: step = 0x0c; break;
      default: step = 0x0c; break;
      }
   }
   return step;

}


/* Stepping Motor derive---------------------------*/
unsigned char  RIGHTmotorOneClock(unsigned char step, char dir)
{
   step = step & 0xf0;
   if (dir) {
      switch (step) {
      case 0x90: step = 0x10; break;
      case 0x10: step = 0x30; break;
      case 0x30: step = 0x20; break;
      case 0x20: step = 0x60; break;
      case 0x60: step = 0x40; break;
      case 0x40: step = 0xc0; break;
      case 0xc0: step = 0x80; break;
      case 0x80: step = 0x90; break;
      default: step = 0xc0; break;
      }
   }
   else {
      switch (step) {
      case 0x90: step = 0x80; break;
      case 0x10: step = 0x90; break;
      case 0x30: step = 0x10; break;
      case 0x20: step = 0x30; break;
      case 0x60: step = 0x20; break;
      case 0x40: step = 0x60; break;
      case 0xc0: step = 0x40; break;
      case 0x80: step = 0xc0; break;
      default: step = 0xc0; break;
      }
   }
   return step;
}


void delay(int n)
{
   volatile int i, j;
   for (i = 1; i < n; i++)
   {
      for (j = 1; j < 185; j++);
   }
}

unsigned char Getsensor()
{
   int num[16];
   int max = 0;
   int sensor_state = 0;
   int i;
   unsigned char sensor = 0;
   for (i = 0; i < 16; i++)
   {
      num[i] = 0;
   }
   for (i = 0; i < 10; i++)
   {
      sensor = SENSOR_PORT & 0x0F;
      if (sensor == 0x00)
         num[0]++;
      else if (sensor == 0x01)
         num[1]++;
      else if (sensor == 0x02)
         num[2]++;
      else if (sensor == 0x03)
         num[3]++;
      else if (sensor == 0x04)
         num[4]++;
      else if (sensor == 0x05)
         num[5]++;
      else if (sensor == 0x06)
         num[6]++;
      else if (sensor == 0x07)
         num[7]++;
      else if (sensor == 0x08)
         num[8]++;
      else if (sensor == 0x09)
         num[9]++;
      else if (sensor == 0x0A)
         num[10]++;
      else if (sensor == 0x0B)
         num[11]++;
      else if (sensor == 0x0C)
         num[12]++;
      else if (sensor == 0x0D)
         num[13]++;
      else if (sensor == 0x0E)
         num[14]++;
      else if (sensor == 0x0F)
         num[15]++;
   }
   max = num[0];
   for (i = 0; i < 15; i++)
   {
      if (max < num[i])
      {
         max = num[i];
         sensor_state = i;
      }
   }
   return (unsigned char)sensor_state;
}
void LED(unsigned char STATE)
{
   PORTF = STATE << 4;
}
void main(void)
{

   unsigned char sensor;
   STATE = 0;
   FLAG_RUN = 0; // flag 0으로 초기화
   volatile unsigned char stepleft = 0, stepright = 0;
   unsigned char stop_sensor = 0x09;
   int stop_falling = 0;
   int stop_flag = 0;
   unsigned char left_sensor = 0x08; // sensor 기본적으로 1 이기때문에 초기값 1
   int left_falling = 0;
   int left_flag = 0;
   unsigned char right_sensor = 0x01; // sensor 기본적으로 1 이기때문에 초기값 1
   int right_falling = 0;
   int right_flag = 0;

   int Left_out = 0;
   int Right_out = 0;
   int stop_clear=0;
   int stop_clear_clear=0;
   init_devices();

   MOTOR_PORT_DDR = 0xff;

   while (1) {
      sensor = Getsensor();//SENSOR_PORT & 0x0F;//Getsensor();
      LED(STATE);//LED 에  STATE 표시!
      stop_sensor = (sensor & 0x09);
      left_sensor = (sensor & 0x08);//  1 0 0 0 SENSOR 중  왼쪽 SENSOR
      right_sensor = (sensor & 0x01); // 0 0 0 0 SENSOR 중 오른쪽 SENSOR
      //////////////////////////////////////////////////////////////////////////////////////////////
      if (stop_sensor == 0x00)
         if (stop_falling != 1)
            stop_falling = 1;
      if (stop_falling == 1)
      {
         if (stop_sensor == 0x09)
         {
            stop_falling = 0;
            stop_flag++;
         }
      }

      //////////////////////////오른쪽 센서 측정
      if (right_sensor == 0x00) //검은석 만난 경우
         if (right_falling != 1) // falling 측정 1이 아닐시 1로 값 설정
            right_falling = 1;
      if (right_falling == 1) // falling 이 1 일 경우 
      {
         if (right_sensor == 0x01)// 흰선 만날 경우
         {
            right_falling = 0; // falling 0으로하고 
            right_flag = 1; // rising이므로 flag값 상승
         }
      }
      //////////////////////////////////////////////////////////////////////////////////////////////////
      ////////////////////////왼쪽 센서 측정
      if (left_sensor == 0x00) //검은석 만난 경우
         if (left_falling != 1) // falling 측정 1이 아닐시 1로 값 설정
            left_falling = 1;
      if (left_falling == 1) // falling 이 1 일 경우 
      {
         if (left_sensor == 0x08)// 흰선 만날 경우
         {
            left_falling = 0; // falling 0으로하고 
            left_flag = 1; // rising이므로 flag값 상승
         }
      }
      /////////////////////////////////////////////////////////////////////////////////
      switch (STATE)
      {
      case STATE_STOP://Stop
         stop_flag = 0;
         stop_falling = 0;
         left_flag = 0;
         left_falling = 0;
         right_flag = 0;
         right_falling = 0;
         if (FLAG_RUN == 1)//RUNFLAG
         {
            FLAG_RUN = 0;
            STATE = STATE_RUN;
         }
         break;
      case STATE_RUN:
         switch (sensor) {
         case 0x09://1001 정주행
            stop_clear++;
            if(stop_clear>10)
            {
               stop_clear_clear=0;
               if(stop_clear>150){ //150 cycle 최소거리 지나면 계속 직진하게끔.(refresh)
                  stop_flag=0;
                  stop_falling = 0;
                  left_flag = 0;
                  left_falling = 0;
                  right_flag = 0;
                  right_falling = 0;
                }
            }

            if(stop_flag > 2)
               STATE = STATE_STOP;
            stepright = RIGHTmotorOneClock(stepright, 1);
            stepleft = LEFTmotorOneClock(stepleft, 0);
            delay(5);
            break;
         case 0x0b://1011 오른쪽으로 틀어진 경우
            stepleft = LEFTmotorOneClock(stepleft, 0);
            stepleft = LEFTmotorOneClock(stepleft, 0);
            stepright = RIGHTmotorOneClock(stepright, 1);
            delay(6);
            break;
         case 0x03://0011 오른쪽으로 심하게 틀어진 경우
            stepleft = LEFTmotorOneClock(stepleft, 0);
            delay(6);
            break;
         case 0x0d://1101 왼쪽으로 틀어진 경우
            stepright = RIGHTmotorOneClock(stepright, 1);
            stepright = RIGHTmotorOneClock(stepright, 1);
            stepleft = LEFTmotorOneClock(stepleft, 0);
            delay(6);
            break;
         case 0x0C: //1100 왼쪽으로 심하게 틀어진 경우
            stepright = RIGHTmotorOneClock(stepright, 1);
            delay(6);
            break;
         default://나머지 경우 + FLAG제조
            stop_clear_clear++;
            if(stop_clear_clear>10)
               stop_clear=0;
            if ((left_sensor == 0x00) && (right_sensor == 0x00)) //sensor 둘다 꺼졌을 경우
            {
               if(left_flag != right_flag)
               {
                  if (left_flag == 1)
                  {
                     STATE = STATE_LEFT;
                  }
                  else if (right_flag == 1)
                  {
                     STATE = STATE_RIGHT;
                  }
               }
            }
            stepright = RIGHTmotorOneClock(stepright, 1);
            stepleft = LEFTmotorOneClock(stepleft, 0);
            delay(5);
            break;
         }
         break;
      case STATE_LEFT:
         for (Left_out = 0; Left_out < 300; Left_out++) 
         {
            stepleft = LEFTmotorOneClock(stepleft, 0);
            delay(4);
            MOTOR_PORT = stepleft | stepright;
         }
         left_flag = 0;
         right_flag = 0;
         left_falling = 0;
         right_falling = 0;
         stop_flag = 0;
         stop_falling = 0;

         STATE = STATE_RUN;
         break;
      case STATE_RIGHT:
         for (Right_out = 0; Right_out < 300; Right_out++)
         {
            stepright = RIGHTmotorOneClock(stepright, 1);
            delay(4);
            MOTOR_PORT = stepleft | stepright;
         }
         left_flag = 0;
         right_flag = 0;
         left_falling = 0;
         right_falling = 0;
         stop_flag = 0;
         stop_falling = 0;
         STATE = STATE_RUN;
         break;

      }
      MOTOR_PORT = stepleft | stepright;
   }
}

ISR(INT5_vect)
{
   DDRD = 0XFF;
   FLAG_RUN = 1;
   cli();
}