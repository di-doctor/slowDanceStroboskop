#include <GyverTM1637.h>
#include <TimerOne.h>
#include <GyverEncoder.h>

#define CLK 2
#define DT 3
#define SW 4
Encoder enc(CLK, DT, SW);  // для работы c кнопкой
int period = 5000;
int expo = 400;
int difFreq = 50;
GyverTM1637 disp(5, 6);
byte mode = 0;
byte brightMode = 5;
byte arrMode[] = {_P, _E, _d};
int *arrParam[] = {&period, &expo, &difFreq};
int paramMax[] = {10000, 800, 200};
int paramMin[] = {1000, 100, -200};
int paramStep[] = {100, 5, 1};

#define LIGHT_PIN 7       // пин драйвера свет
#define DRV_1 8           // пин драйвера 1 соленоид
#define DRV_2 9           // пин драйвера 2 соленоид

boolean flashState, motorState;
uint16_t lightTimer, flashDelay;
uint32_t motorPrev, lightPrev, setPrev;
//...................................... SETUP.......................................................................

void setup() {
  // настраиваем пины как выходы
  pinMode(DRV_1, OUTPUT);
  pinMode(DRV_2, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);

  disp.clear();
  disp.brightness(5);  // яркость, 0 - 7 (минимум - максимум)
  Serial.begin(9600);
  enc.setType(TYPE2);
  //attachInterrupt(0, isr, CHANGE);    // прерывание на 2 пине! CLK у энка
  Timer1.initialize(1000);            // установка таймера на каждые 1000 микросекунд (= 1 мс)
  Timer1.attachInterrupt(timerIsr);   // запуск таймера
  setADCrate(2);    // ускоряем analogRead
  for (int i = 0; i <= 10; i++) {
    disp.displayInt(i);
    delay(100);    
  }
  disp.clear();
  disp.displayByte(_empty,_G,_o,_empty);
}

//.......................................INTERUPT_TIMER......................................................................

void timerIsr() {   // прерывание таймера
  enc.tick();     // отработка теперь находится здесь
}
//..........................................FUNCTIONS...................................................................

void workTurnUp(int *param, int maxParam, int paramStepItem) {
  *param = *param + paramStepItem;
  if (*param > maxParam) *param = maxParam;
  display(*param);
}

void workTurnDown(int *param, int minParam, int paramStepItem) {
  *param = *param - paramStepItem;
  if (*param < minParam) *param = minParam;
  display(*param);
}

void display(int counter) {
  if (mode == 0)  counter /= 10;
  disp.displayInt(counter);
  disp.displayByte(0, arrMode[mode]);
}
//..........отработка поворота энкодера

void turn() {
  if (enc.isRight())  workTurnUp(arrParam[mode], paramMax[mode], paramStep[mode]);

  if (enc.isLeft())  workTurnDown(arrParam[mode], paramMin[mode], paramStep[mode]);


  if (enc.isRightH()) {
    Serial.println("Right holded"); // если было удержание + поворот
    if (++brightMode > 7) brightMode = 7;
    disp.brightness(brightMode);
  }
  if (enc.isLeftH()) {
    Serial.println("Left holded");
    if (--brightMode < 0) brightMode = 0;
    disp.brightness(brightMode);
  }
  //  if (enc.isFastR()) {
  //    workTurnUp(arrParam[mode], paramMax[mode], paramStep[mode]);
  //  }
  //  if (enc.isFastL()) {
  //    workTurnDown(arrParam[mode], paramMax[mode], paramStep[mode]);
  //  }
}
//.....................................LOOP...................................................................

void loop() {
  if (enc.isSingle()) {
    Serial.print("Кнопка нажата mode--");
    if (++mode > 2) mode = 0;
    Serial.println(mode);
    display(*arrParam[mode]);
  }
  if (enc.isDouble()) {
    period = 5000;
    expo = 400;
    difFreq = 50;
    display(*arrParam[mode]);
  }
  if (enc.isTurn()) {
    turn();
    flashDelay = period * 2 + expo;
  }

  //................................................. таймер света
  if (micros() - lightPrev >= lightTimer) {
    lightPrev = micros();

    flashState = !flashState;               // инвертируем состояние вспышки

    // быстрый аналог функции digitalWrite
    // если используете другую модель Arduino (не Nano/UNO/Mini),
    // замените на digitalWrite
    bitWrite(PORTD, LIGHT_PIN, flashState); // подаём на вспышку

    // перенастраиваем таймер согласно состоянию
    if (flashState) lightTimer = expo;
    else lightTimer = flashDelay - expo;
  }

  //............................................... таймер драйвера
  if (micros() - motorPrev >= period) {
    motorPrev = micros();
    motorState = !motorState;

    // быстрый аналог функции digitalWrite
    // если используете другую модель Arduino (не Nano/UNO/Mini),
    // замените на digitalWrite
    bitWrite(PORTD, DRV_1, motorState);
    bitWrite(PORTD, DRV_2, !motorState);
  }
}



// ускорение чтения с АЦП
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
void setADCrate(byte mode) {
  if ((mode & (1 << 2))) sbi(ADCSRA, ADPS2);
  else cbi(ADCSRA, ADPS2);
  if ((mode & (1 << 1))) sbi(ADCSRA, ADPS1);
  else cbi(ADCSRA, ADPS1);
  if ((mode & (1 << 0))) sbi(ADCSRA, ADPS0);
  else cbi(ADCSRA, ADPS0);
}
