// 2021.09.28 включаем и выключаем свет и помпу для микрозелени по расписанию,  umkiedu@gmail.com
//  чтоб небыло плесени сделать продувку вентилятором
//  свет можно включать в режиме заката - рассвета, если есть дополнительные реле
//  добавлен замер датчика освещенности, влажности, температуры - воздуха  AMT 1001 R=10kOm

#include <SoftwareSerial.h>
#include "amt1001_ino.h"
SoftwareSerial BTSerial(6, 7); // RX, TX

unsigned long time_den = 300000; //61200000;// время светлого дня (17 часов)
unsigned long time_noch = 180000; //25200000; // длительность сна (7 часов);
unsigned long time_poliv1 = 120000; //7200000; // время между поливами (120 мин)
unsigned long time_poliv2 =  60000; // длительность полива (1 мин)
unsigned long time_zamer =  5000; // длительность  между замерами (5 сек)

unsigned long currentTime, DelayNigth, DelayPoliv, DelayZamer;
int dinamikPin = 12;// пин пищалки

const int relay1 = 11;  // реле1 на pin D11
const int relay2 = 10;  // реле2  на pin D10
const int tempPin = A6; // analog pin
const int humPin = A7; // humidity
const int tempWater = A4; // analog temp Water
uint16_t step;
uint8_t temphumi[8];  //  массив из восьми байт для вывод данных либо в график либо на панель

int nStateNigth = 0; // статус ночь-0 день 1
int nStatePoliv = 0; // статус полив начат -0 , окончен - 1
int nStateZamer = 0; // статус замер окончен -0 , температура - 1, влажность - 2

// dla razbora paketa
#define MAX_MAS_BT 256
unsigned char inByte[MAX_MAS_BT];
unsigned char masByte[256];
bool flnp = false; //  flag nachala paketa
bool flb = false; // flag vtorogo baita
int f_numb_p = 0; // flag nomera paketa
int fl_pp = 0; // flag poluchenia paketa s data ot paneley
int tec_bt = 0;
int len_p = 0;
int kol_data = 0;

int zaprosDataPanel = 1;
int zaprosDataGraph = 0;
int zaprosDataGraphPeriod = 0;



void DayNigth(); // День-ночь
void Poliv();    // полив помпой
void Priem();    // прием данных из BT
void Knopka();   // Обработка данных по кнопке
void Zamer();    // Замер тмпературы и влажности воздуха
void DataPanelToBT(); //vidacha BT dannih dla paneley

void setup()  {
  BTSerial.begin(9600); // инициализируем  порт блютус
  Serial.begin(9600);   // инициализируем  порт сериал - шнур USB
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);

  currentTime = millis();
  // установить начальные состояния процессов
  nStateNigth = 0;
  nStatePoliv = 0;

  DelayNigth  = currentTime;
  DelayPoliv = currentTime;
  DelayNigth += time_den;   // тек. время с задержкой на светлое время
  DataPanelToBT();	// vidaem pervonacalnie ustanovki

  flb = false;
  flnp = false;

}

// циклим процессы  - конечные автоматы
void loop()  {

  currentTime = millis();
  DayNigth();  // процесс ДеньНочь
  Poliv();     // процесс Полива помпой
  Priem();    // Автомат приема и обработки данных с пульта
  Knopka(); // обрабатываем кнопки прямого упроавляние релюшками
  Zamer();    // Замер тмпературы и влажности воздуха
  if ( zaprosDataPanel == 1) {
    DataPanelToBT();
  }
}


// процесс ДеньНочь
void DayNigth()
{

  switch (nStateNigth) {
    // состояние День
    case 0:
      if (currentTime >= DelayNigth) {
        nStateNigth = 1;
        DelayNigth  = currentTime + time_noch; // переключаем на состояние ночь
        digitalWrite(relay1, HIGH);  // Включаем свет
      }
      break;
    // состояние Ночь
    case 1:
      if (currentTime >= DelayNigth) {
        nStateNigth = 2;
        DelayNigth  = currentTime + time_den; // переключаем на состояние день
        digitalWrite(relay1, LOW); // выключааем свет
      }
      break;
    case 2:
      if (currentTime >= DelayNigth) {
        nStateNigth = 0;  // сбрасываем состояние ночи в  статус день
        Serial.println("sbros den  r1 ");
      }
      break;
  }
}



// процесс Полива помпой
void Poliv()
{
  switch (nStatePoliv) {
    case 0:
      if (currentTime >= DelayPoliv) {
        nStatePoliv = 1;
        digitalWrite(relay2, LOW); // включаем помпу на время полива
        DelayPoliv = currentTime + time_poliv2; //период сколько работает помпа

      }
      break;
    case 1:
      if (currentTime >= DelayPoliv) {
        nStatePoliv = 2;
        digitalWrite(relay2, HIGH); // выключаем помпу и ждем по времени
        DelayPoliv = currentTime + time_poliv1; // период через который помпа включается для полива

      }
      break;
    case 2:
      if (currentTime >= DelayPoliv) {
        nStatePoliv = 0;  // сбрасываем состояние порлива в начальный статус
        Serial.println("sbros poliv  r2 ");
      }
      break;
  }

}


void Priem() // выполняем циклически опрос порта и отправляем все байты с блютуса в шнур
{
  int  i, count; //i - это элемент массива команды из 4 байт
  count = BTSerial.available();
  for (i = 0; i < count; i++) {
    if (( kol_data + i + 1) >= MAX_MAS_BT) kol_data = 0; //chtoby ne perepolnalsa massiv
    inByte[kol_data + i] = BTSerial.read();

    delay(10);
    Serial.print(inByte[kol_data + i], HEX); // вывод в COM порт побайтоно в шестнадцатиричной системе
    Serial.print(" "); // ставим пробел между байтами, чтобы удобно было смотреть монитор порта
  }
  if (count > 0) {
    //  Serial.println();
    //    Serial.print(count); // вывод в COM порт побайтоно в шестнадцатиричной системе
    // Serial.println();
  }
  kol_data = kol_data + count;
  if (kol_data > 0) {
    Serial.print("RAzBOR "); // вывод в COM порт побайтоно в шестнадцатиричной системе
    Serial.println();
    //Serial.print("flb=");
    //Serial.print(flb);
    //Serial.println();
    //Serial.print("flnp=");
    //Serial.print(flnp);
    //Serial.println();

    Serial.print(kol_data);
    Serial.println();
  }
  for (i = 0; i < kol_data; i++) {
    Serial.print(inByte[i], HEX); // вывод в COM порт побайтоно в шестнадцатиричной системе
    Serial.print(" "); // ставим пробел между байтами, чтобы удобно было смотреть монитор порта
    if (flb == false) { // poka ne naiden 2 bait nachala paketa — on ge ego priznak
      if (flnp == false) { //poka nachalo paketa ne naideno (1 byte)
        if (inByte[i] == 0x7f) { //pervii bait nachala paketa
          flnp = true;
          //  Serial.print("1 byte"); // вывод в COM порт побайтоно в шестнадцатиричной системе
          //Serial.println();
          continue;
        }
      }
      else {

        if (inByte[i] != 0x7e) { //sleduyshii bite ne  7e — eto ne nachalo paketa!!!
          flnp = false;
          continue;
        }
        else {
          flb = true;
          continue;
        }
      }
    }//if (flb==0
    else { //razbiraem paket
      tec_bt++;// nomer baita v pakete
      if (tec_bt == 1) {
        len_p = inByte[i];

      }
      if (tec_bt == 2) {


        if (inByte[i] == 1) {
          zaprosDataPanel = 1;
          Serial.print("Zapros data panel");
          Serial.println();


        }
        if (inByte[i] == 2) {
          Serial.print("Zapros data graph");
          Serial.println();
          zaprosDataGraph = 1;
        }
        if (inByte[i] == 3) {
          zaprosDataGraphPeriod = 1;
          Serial.print("Zapros data graph period ON");
          Serial.println();
        }
        if (inByte[i] == 4) {
          zaprosDataGraphPeriod = 0;
          Serial.print("Zapros data graph period OFF");
          Serial.println();
        }
        if (inByte[i] == 5) { //data ot paneley
          fl_pp = 1;
          Serial.print("Poluchili data ot panely");
          Serial.println();
          continue;
        }
      }
      if (fl_pp == 1) {
        if (tec_bt == 3)   time_den =  60000 * inByte[i]; // время светлого дня ( часов)
        if (tec_bt == 4)   time_noch =  60000 * inByte[i]; // время сна ( часов);
        if (tec_bt == 5)   time_poliv1 =  60000 * inByte[i]; // время между поливами (min)
        if (tec_bt == 6) {
          time_poliv2 =   60000 * inByte[i]; // время полива (min)
          tone(dinamikPin, 1000, 500); // Play midi
        }

      }


      if (tec_bt + 2 >= len_p) { // konec paketa
        flb = 0;
        flnp = 0;
        tec_bt = 0;
        len_p = 0;
        fl_pp = 0;
        Serial.print("End pacet");
        Serial.println();
      }
    }
  }//for(i=0;i<kol_data;i++){

  if (kol_data > 0) Serial.println();

  kol_data = kol_data - i;




}


void Knopka() //  обрабатываем нажатие кнопки
{
  switch (inByte[7]) {
    case 221:
      digitalWrite(relay2, LOW); // включаем помпу по пульту
      inByte[3] = 0;
      Serial.print("q221  ");
      break;
    case 222:
      digitalWrite(relay2, HIGH); // выключаем помпу по пульту
      inByte[3] = 0;
      Serial.print("q222  ");
      break;
    case 211:

      digitalWrite(relay1, LOW); // включаем свет по пульту
      inByte[3] = 0;
      Serial.print("q211  ");
      break;
    case 212:
      digitalWrite(relay1, HIGH); // выключаем свет по пульту
      inByte[3] = 0;
      Serial.print("q212  ");
      break;
  }
}

void Zamer()    // Замер тмпературы и влажности воздуха
{
  switch (nStateZamer) {
    case 0:
      if (currentTime >= DelayZamer) {
        nStateZamer = 1;
        // Get Temperature
        step = analogRead(tempPin);
        temphumi[4] = amt1001_gettemperature(step) + 0; //  вычисляем температуру в С  с поправочным коэфициентом
        DelayZamer = currentTime + time_zamer; //период следующего замера Т
        Serial.print(" T=  ");
        Serial.print(temphumi[4]);
      }
      break;

    case 1:
      if (currentTime >= DelayZamer) {
        nStateZamer = 2;
        // Get Humidity
        step = analogRead(humPin);
        double volt = (double)step * (5.0 / 1023.0);
        temphumi[5] = amt1001_gethumidity(volt); //  вычисляем влажность в %
        DelayZamer = currentTime + time_zamer; // период следующего замера Н
        Serial.print(" H=  ");
        Serial.print(temphumi[5]);
      }
      break;

    case 2: // отправляем данные в порт BT
      if (currentTime >= DelayZamer) {
        nStateZamer = 0;  // сбрасываем состояние замера в начальный статус
        temphumi[0] = 0x7f;
        temphumi[1] = 0x7e;
        temphumi[2] = 6;   // Длинна кодограммы
        temphumi[3] = 2;   // номер кодограммы
        DelayZamer = currentTime + time_zamer; // период отправки данных в порт BT
        if ((zaprosDataGraph == 1) || (zaprosDataGraphPeriod == 1))        BTSerial.write(temphumi, 6);

        if (zaprosDataGraph == 1)  zaprosDataGraph = 0; // sbrasivaem flag vidachi dannih
      }
      break;
  }
}

void DataPanelToBT() { //vidacha BT dannih dla paneley
  uint8_t dtPanel[8];  //  массив из восьми байт для вывод данных либо в график
  dtPanel[0] = 0x7f;
  dtPanel[1] = 0x7e;
  dtPanel[2] = 8;   // Длинна кодограммы
  dtPanel[3] = 1;   // номер кодограммы

  dtPanel[4] =	time_den / 60000;
  dtPanel[5] =	time_noch / 60000;
  dtPanel[6] =	time_poliv1 / 60000 ;
  dtPanel[7] =	time_poliv2 / 60000 ;

  BTSerial.write(dtPanel, 8);
  /*
    Serial.println(time_den / 3600000); // Выводим время светлого дня (17 часов)
    Serial.println(time_noch / 3600000); // Выводим длительность сна (7 часов);
    Serial.println(time_poliv1 / 60000); // Выводимвремя между поливами (120 мин)
    Serial.println(time_poliv2 / 60000); // Выводим длительность полива (1 мин)
  */
  Serial.println(dtPanel[4]); // Выводим время светлого дня (17 часов)
  Serial.println(dtPanel[5]); // Выводим длительность сна (7 часов);
  Serial.println(dtPanel[6]); // Выводимвремя между поливами (120 мин)
  Serial.println(dtPanel[7]); // Выводим длительность полива (1 мин)

  Serial.println("datta panel to BT");

  zaprosDataPanel = 0;
}
