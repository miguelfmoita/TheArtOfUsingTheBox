#include <Wire.h>
#include "I2C_Anything.h"
#include <LiquidCrystal.h>
#include "pitches.h"
#include <PWMServo.h>
PWMServo servoLatch;

LiquidCrystal lcd(7, 8, 10, 11, 12, 13);

#define ADDRESS_A 1
#define ADDRESS_B 2

int servoPin = 9;  
int servoLock = 180;                                                     // angle (deg) of "locked" servo
int servoUnlock = 0; 
byte WP=0;
byte fase=0;
volatile float range;
byte escolherWP=1;
int pnumero=0;
byte count=0;
byte nmusicas=0;
byte punicao=0;
byte timeoff=30;
unsigned long prevmillis;

int bot=0;
int botoes(){
  int bot0=analogRead(0);
  int bot1=analogRead(1);
  if (bot1>100 & bot1<300){bot=1;}
  else if (bot1>300 & bot1<600){bot=2;}
  else if (bot0>100 & bot0<300){bot=3;}
  else if (bot0>300 & bot0<550){bot=4;}
  else{bot=0;}
  return bot;
}

//-----------------------------------------------------------------------------

void dist(byte mode){
  if (range >=0){
    if (mode==1){
      delay(500);
      if (range<150.0){
        mode=2;
      }
    }
    if (mode==2){  
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Distancia:");
      lcd.setCursor(0,1);
      lcd.print(range);  
      delay(500);
      if (range<100){
        chegaste();
        WP=WP-100;
        fase++;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
    }
    else if (mode==3){
      delay(500);
      if (range<100.0){
        WP=10;
        fase++;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
    }
  }
}

//-----------------------------------------------------------------------------

void chegaste(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Chegaste!"));
  tone(5, NOTE_C5, 250);
  delay(250*1.3);
  tone(5, NOTE_G5, 750);
  delay(750*1.3);
  delay(2000);
  lcd.clear();
}

//-----------------------------------------------------------------------------

void dirigir(String WPlat,String WPlong){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Dirige-te para"));
  lcd.setCursor(0,1);
  lcd.print(F("as coordenadas:"));
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(WPlat);
  lcd.setCursor(0,1);
  lcd.print(WPlong);
  delay(1000);
  lcd.clear();
}

//-----------------------------------------------------------------------------

void enigma(){
  bot=botoes();
  switch (bot){
    case 4:
    WP++;
    fase=0;
    lcd.clear();
    Wire.beginTransmission(ADDRESS_B);
    Wire.write(WP);
    Wire.endTransmission();
    break;
  default:
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Decifra o enigma"));
    lcd.setCursor(0,1);
    lcd.print(F("Enter"));
    delay(300);
    break;
  }
}

//----------------------------------------------
// Steering wheel potentiometer
const int POTPIN = 1;
//const int MAXPOT = 800; // no need to turn the wheel all the way to 1023 :)
 
// Piezo speaker
const int SPEAKERPIN = 5;
 
const int RANDSEEDPIN = 2; // an analog pin that isn't connected to anything
 
const int MAXSTEPDURATION = 300; // Start slowly, each step is 1 millisec shorter.
const int MINSTEPDURATION = 150; // This is as fast as it gets
 
const int NGLYPHS = 6;
// the glyphs will be defined starting from 1 (not 0),
// to enable lcd.print() of null-terminated strings
byte glyphs[NGLYPHS][8] = {
  // 1: car up
  { B00000,
    B01110,
    B11111,
    B01010,
    B00000,
    B00000,
    B00000,
    B00000}
  // 2: car down
  ,{B00000,
    B00000,
    B00000,
    B00000,
    B01110,
    B11111,
    B01010,
    B00000}
  // 3: truck up
  ,{B00000,
    B11110,
    B11111,
    B01010,
    B00000,
    B00000,
    B00000,
    B00000}
  // 4: truck down
  ,{B00000,
    B00000,
    B00000,
    B00000,
    B11110,
    B11111,
    B01010,
    B00000}
  // 5: crash up
  ,{B10101,
    B01110,
    B01110,
    B10101,
    B00000,
    B00000,
    B00000,
    B00000}
  // 6: crash down
  ,{B00000,
    B00000,
    B00000,
    B10101,
    B01110,
    B01110,
    B10101,
    B00000}
};
 
const int NCARPOSITIONS = 4;
 
// Each position is mapped to a column of 2 glyphs
// Used to make sense when I had a 5th position
// where car or crash was drawn as 2 glyphs
// (can't do that since 0 terminates strings),
// so it's kinda silly now, but it ain't broke :)
const char BLANK=32;
char car2glyphs[NCARPOSITIONS][2] = {
  {1,BLANK},{2,BLANK},{BLANK,1},{BLANK,2}
};
char truck2glyphs[NCARPOSITIONS][2] = {
  {3,BLANK},{4,BLANK},{BLANK,3},{BLANK,4}
};
char crash2glyphs[NCARPOSITIONS][2] = {
  {5,BLANK},{6,BLANK},{BLANK,5},{BLANK,6}
};
 
const int ROADLEN = 15; // LCD width (not counting our car)
int road[ROADLEN]; // positions of other cars
char line_buff[2+ROADLEN]; // aux string for drawRoad()
int road_index;
int car_pos=0;
// Off-the-grid position means empty column, so MAXROADPOS
// determines the probability of a car in a column
// e.g. 3*NCARPOSITIONS gives p=1/3
const int MAXROADPOS = 3*NCARPOSITIONS;
int step_duration;
 
int crash; // true if crashed
unsigned int crashtime; // millis() when crashed
const int CRASHSOUNDDURATION = 250;
 
const char *INTRO1="Trucks ahead,";
const char *INTRO2="Drive carefully";
const int INTRODELAY = 2000;

void getSteeringWheel() {
//  car_pos = map(analogRead(POTPIN),0,1024,0,NCARPOSITIONS);
  bot = botoes();
  if (bot == 1 & car_pos < 3) {car_pos = car_pos + 1;}
  if (bot == 2 & car_pos > 0) {car_pos = car_pos - 1;}
  if (bot == 0) {car_pos = car_pos;}
}
 
void drawRoad() {
  for (int i=0; i<2; i++) {
    if (crash) {
      line_buff[0]=crash2glyphs[car_pos][i];
    } 
    else {
      line_buff[0]=car2glyphs[car_pos][i];
    }
    for (int j=0; j<ROADLEN; j++) {
      int pos = road[(j+road_index)%ROADLEN];
      line_buff[j+1] = pos>=0 && pos<NCARPOSITIONS ? truck2glyphs[pos][i] : BLANK;
    }
    lcd.setCursor(0,i);
    lcd.print(line_buff);
  }
}

//-----------------------------------------------------------------------------

//Prog destinado a escolher um numero - usado no progama da pergnum
int numero=0;
int num1 = 0;
int num2 = 0;
int num3 = 0;

int escolhernumero (int bot) {
  if (bot==1){
    if (num1<9){
      num1++; 
    }
    else if (num1==9){
      num1=0;
    }
  }
  if (bot==2){
    if (num2<9){
      num2++; 
    }
    else if (num2==9){
      num2=0;
    }
  }
  if (bot==3){
    if (num3<9){
      num3++; 
    }
    else if (num3==9){
      num3=0;
    }
  }
  numero=num1*100+num2*10+num3;
  return numero;
}

//Programa para o numero
void pergnumero(int num) {
  if (num<10){
    lcd.setCursor(0,1);
    lcd.print(0);
    lcd.setCursor(1,1);
    lcd.print(0);
    lcd.setCursor(2,1);
    lcd.print(num);  
     
  }
  else if (num>=10 && num<100){
    lcd.setCursor(0,1);
    lcd.print(0);
    lcd.setCursor(1,1);
    lcd.print(num);  
  }
  else if(num>=100){
    lcd.setCursor(0,1);
    lcd.print(num);  
  }
  delay(300);
}

//-----------------------------------------------------------------------------

void musica(int melody[],int noteDurations[], int numnotas, int repeticoes) {
  if (nmusicas<repeticoes){
    for (int thisNote = 0; thisNote < numnotas; thisNote++) {
      int noteDuration = 1500/noteDurations[thisNote];
      tone(5, melody[thisNote],noteDuration);
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(5);
    }
    nmusicas++;
  }
  else if (nmusicas>=repeticoes){
    delay(1000);
    lcd.clear();
    nmusicas=0;
    fase++;
  }
}

//-----------------------------------------------------------------------------

void setup() {
  Serial.begin(112500);
  //LCD
  lcd.begin(16, 2);
  
  Wire.begin(ADDRESS_A);
  Wire.onReceive(receiveEvent);
  
  //Servo
  servoLatch.attach(SERVO_PIN_A);
  servoLatch.write(servoLock);
  
  lcd.setCursor(0,0);
  lcd.print(F("The Art Of Using"));
  lcd.setCursor(4,1);
  lcd.print(F("The Box"));
  delay(5000);
  lcd.clear();
}

//-----------------------------------------------------------------------------

void loop() {
  if (range==-1){
    lcd.clear();
    lcd.print("sem satelites");
    delay(500);
  }
  else{
    if (WP==0){
      if (fase==0){
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        fase++;
      }
      else if (fase==1){
        bot=botoes();
        switch (bot){
          case 1:
            escolherWP=1;
            fase=3;
            break;
          case 2:
            lcd.clear();
            fase=2;
            break;
          default:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("A - Inicio"));
            lcd.setCursor(0,1);
            lcd.print(F("B - Escolher WP"));
            delay(300); 
            break;
        }
      }
      else if (fase==2){
        bot=botoes();
        switch (bot){
        case 1:
          if (escolherWP<14){
            escolherWP++;
            lcd.setCursor(5,1);
            lcd.print(escolherWP);
            delay(300);
          }
          else if (escolherWP==14){
            escolherWP=1;
            lcd.setCursor(5,1);
            lcd.print(escolherWP);
            lcd.setCursor(6,1);
            lcd.print(" ");
            delay(300);
          }
          break;
        case 4:
          fase=3;
          break;
        default:
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(F("Escolher WP"));
          lcd.setCursor(0,1);
          lcd.print(F("WP - "));
          lcd.print(escolherWP);
          delay(300); 
          break;
        }
      }
      else if(fase==3){
        WP=escolherWP;
        fase=0;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("vamos comecar"));
        lcd.setCursor(0,1);
        lcd.print(F("WP - "));
        lcd.print(WP);
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        delay(2000);
        lcd.clear();
      }
    }
    
  //-----------------------------------------------------------------------------
    
    else if (WP==1){
      if (fase==0){
        WP=101;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        enigma();
      }
    }
    else if (WP==101){
      dist(2);
    } 
    
  //-----------------------------------------------------------------------------
  
    else if (WP==2){
      if (fase==0){
        WP=WP+100;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
      else if (fase==1){
        crash = crashtime = road_index = 0;
        step_duration = MAXSTEPDURATION;
        line_buff[1+ROADLEN] = '\0'; // null terminate it
        randomSeed(analogRead(RANDSEEDPIN));
        for (int i=0; i<NGLYPHS; i++) {
          lcd.createChar(i+1,glyphs[i]);
        }
        for (int i=0; i<ROADLEN; i++) {
          road[i]=-1;
        }
        getSteeringWheel();
        drawRoad();
          
        lcd.setCursor(1,0);
        lcd.print(INTRO1);
        lcd.setCursor(1,1);
        lcd.print(INTRO2);
        delay(INTRODELAY);
        prevmillis=millis();
        fase++;
      }
      else if (fase==2){
        unsigned long now = millis()-prevmillis;
        if (!crash) {
          getSteeringWheel();
          crash = (car_pos==road[road_index]);
        }
        if (crash) {
          if (!crashtime) {
            crashtime=now;
            drawRoad();
            tone(5, NOTE_C5, 200);
            delay(260);
            // Game over text
            // (keep first 2 "crash" columns intact)
            lcd.setCursor(2,0);
            lcd.print(F("Crashed after"));
            lcd.setCursor(2,1);
            lcd.print(now/1000);
            lcd.print(F(" seconds."));
            delay(1000);
            if (now>10000){
              WP=3;
              fase=0;
              lcd.clear();
              Wire.beginTransmission(ADDRESS_B);
              Wire.write(WP);
              Wire.endTransmission();
            }
            else{
              fase=1;
            }
          }
          delay(10); // Wait a bit between writes
        } 
        
        else {
       
          int prev_pos = road[(road_index-1)%ROADLEN];
          int this_pos = random(MAXROADPOS);
          while (abs(this_pos-prev_pos)<2) { // don't jam the road
            this_pos = random(MAXROADPOS);
          }
          road[road_index] = this_pos;
          road_index = (road_index+1)%ROADLEN;
          drawRoad();
          delay(step_duration);
          if (step_duration>MINSTEPDURATION) {
            step_duration--; // go faster
          }
        }
      }
      
    }
    else if (WP==102){
      dist(1);
    } 
    //-----------------------------------------------------------------------------
    
    else if (WP==3){
      if (fase==0){
        WP=103;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        lcd.setCursor(0,0);
        lcd.print(F("inserir code"));
        fase=2;
        delay(1000);
      }
      else if (fase==2){
        bot=botoes();
        pnumero=escolhernumero(bot);
        pergnumero(pnumero);
        if (bot==4){
          fase=3;
        }
      }
      else if (fase==3){
        if (pnumero==123){
          lcd.clear();
          lcd.print(F("acertou!"));
          WP++;
          fase=0;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          delay(2000);
          lcd.clear();
        }
        else {
          lcd.clear();
          lcd.print(F("Eroou!"));
          delay(1000);
          lcd.clear();
          fase=1;
        }
        num1 = 0;
        num2 = 0;
        num3 = 0;
        pnumero=0;
      } 
    }
    else if (WP==103){
      dist(2);
    } 
    
  //-----------------------------------------------------------------------------
  
    else if (WP==4){
      if (fase==0){
        WP=104;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        enigma();
      }
    }
    else if (WP==104){
      dist(2);
    }
    
  //-----------------------------------------------------------------------------
  
    else if (WP==5){
      if (fase==0){
        WP=105;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
      else if (fase==1){
        bot = botoes();
        if (bot==4){
          WP++;
          fase=0;
          count=0;
          lcd.clear();
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          delay(1000);
        }
            
        else{
          if (count<=5){
            lcd.setCursor(0,0);
            lcd.print(F("0123456789012345"));
            lcd.setCursor(0,1);
            lcd.print(F("0123456789012345"));
            delay(1000);
            count++;
          }
          else if (count>5 && count<=10){
            lcd.setCursor(0,0);
            lcd.print(F("5432109876543210"));
            lcd.setCursor(0,1);
            lcd.print(F("5432109876543210"));
            delay(1000);
            count++;
          }
          else if (count==11){
            count=0;
          }
        }
      }
    }
    else if (WP==105){
      dist(1);
    }
    
  //-----------------------------------------------------------------------------
    
    else if (WP==6){
      if (fase==0){
        WP=106;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
      else if (fase==1){
        enigma();
      }
    }
    else if (WP==106){
      dist(1);
    }
    
  //-----------------------------------------------------------------------------
  
    else if (WP==7){
      if (fase==0){
        WP=107;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
      else if (fase==1){
        int melody[] = {
          NOTE_E7, NOTE_E7, 0, NOTE_E7, 
          0, NOTE_C7, NOTE_E7, 0,
          NOTE_G7, 0, 0,  0,
          NOTE_G6, 0, 0, 0, 
  
          NOTE_C7, 0, 0, NOTE_G6, 
          0, 0, NOTE_E6, 0, 
          0, NOTE_A6, 0, NOTE_B6, 
          0, NOTE_AS6, NOTE_A6, 0, 
  
          NOTE_G6, NOTE_E7, NOTE_G7, 
          NOTE_A7, 0, NOTE_F7, NOTE_G7, 
          0, NOTE_E7, 0,NOTE_C7, 
          NOTE_D7, NOTE_B6, 0, 0,
  
          NOTE_C7, 0, 0, NOTE_G6, 
          0, 0, NOTE_E6, 0, 
          0, NOTE_A6, 0, NOTE_B6, 
          0, NOTE_AS6, NOTE_A6, 0, 
  
          NOTE_G6, NOTE_E7, NOTE_G7, 
          NOTE_A7, 0, NOTE_F7, NOTE_G7, 
          0, NOTE_E7, 0,NOTE_C7, 
          NOTE_D7, NOTE_B6, 0, 0};
  
        // note durations: 4 = quarter note, 8 = eighth note, etc.:
        int noteDurations[] = {
          12, 12, 12, 12, 
          12, 12, 12, 12,
          12, 12, 12, 12,
          12, 12, 12, 12, 
  
          12, 12, 12, 12,
          12, 12, 12, 12, 
          12, 12, 12, 12, 
          12, 12, 12, 12, 
  
          9, 9, 9,
          12, 12, 12, 12,
          12, 12, 12, 12,
          12, 12, 12, 12,
  
          12, 12, 12, 12,
          12, 12, 12, 12,
          12, 12, 12, 12,
          12, 12, 12, 12,
  
          9, 9, 9,
          12, 12, 12, 12,
          12, 12, 12, 12,
          12, 12, 12, 12};
          
        musica(melody,noteDurations,78,1);
      }
      else if (fase==2){
        int bot = botoes();
        switch (bot)
        {
        case 1:
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Errou A");
          delay(2000);
          fase=1;
          break;
        case 2:
          lcd.clear();
          lcd.print("Acertou");
          WP++;
          fase=0;
          count=0;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          delay(2000);
          lcd.clear();
          break;
        case 3:
          lcd.clear();
          lcd.print("Errou C");
          delay(2000);
          fase=1;
          break;
        default:
          if (count<=5){
            lcd.setCursor(0,0);
            lcd.print("De que jogo e   ");
            lcd.setCursor(0,1);
            lcd.print("a musica        ?");
            delay(1000);
            count++;
          }
          else if (count>5 && count<=10){
            lcd.setCursor(0,0);
            lcd.print(F("A-Sonic B-Super  "));
            lcd.setCursor(0,1);
            lcd.print(F("Mario C-Pacman  "));
            delay(1000);
            count++;
          }
          else if (count==11){
            count=0;
          }  
        }
      }
    }
    else if (WP==107){
      dist(1);
    }
    
  //-----------------------------------------------------------------------------
  
    else if (WP==8){
      if (fase==0){
        WP=108;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        lcd.setCursor(0,0);
        lcd.print(F("inserir code 1"));
        fase=2;
        delay(2000);
      }
      else if (fase==2){
        bot=botoes();
        pnumero=escolhernumero(bot);
        pergnumero(pnumero);
        if (bot==4){
          fase=3;
        }
      }
      else if (fase==3){
        if (pnumero==233){
          lcd.clear();
          lcd.print("acertou!");
          delay(1000);
          fase=4;
        }
        else {
          lcd.clear();
          lcd.print("Eroou!");
          delay(1000);
          lcd.clear();
          fase=1;
        }
        num1 = 0;
        num2 = 0;
        num3 = 0;
        pnumero=0;
      }
      else if (fase==4){
        lcd.setCursor(0,0);
        lcd.print(F("inserir code 2"));
        fase=5;
        delay(2000);
      }
      else if (fase==5){
        bot=botoes();
        pnumero=escolhernumero(bot);
        pergnumero(pnumero);
        if (bot==4){
          fase=6;
        }
      }
      else if (fase==6){
        if (pnumero==123){
          lcd.clear();
          lcd.print("acertou!");
          WP++;
          fase=0;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          delay(2000);
          lcd.clear();
        }
        else {
          lcd.clear();
          lcd.print("Eroou!");
          delay(1000);
          fase=1;
        }
        num1 = 0;
        num2 = 0;
        num3 = 0;
        pnumero=0;
      }
    }
    else if (WP==108){
      dist(2);
    }
    
    //-----------------------------------------------------------------------------
  
    else if (WP==9){
      if (fase==0){
        WP=109;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        lcd.setCursor(0,0);
        lcd.print(F("R G B"));
        fase=2;
        delay(2000);
      }
      else if (fase==2){
        bot=botoes();
        pnumero=escolhernumero(bot);
        pergnumero(pnumero);
        if (bot==4){
          fase=3;
        }
      }
      else if (fase==3){
        if (pnumero==123){
          lcd.clear();
          lcd.print("acertou!");
          WP++;
          fase=0;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          delay(2000);
          lcd.clear();
        }
        else {
          lcd.clear();
          lcd.print("Eroou!");
          delay(1000);
          lcd.clear();
          fase=1;
        }
        num1 = 0;
        num2 = 0;
        num3 = 0;
        pnumero=0;
      }
    }
    else if (WP==109){
      dist(2);
    }
    
  //-----------------------------------------------------------------------------
  
    else if (WP==10){
      if (fase==0){
        WP=120;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        int melody[] = {NOTE_C5,0,0,NOTE_C5,0,NOTE_AS4,0, NOTE_C5,0,NOTE_C5,0,NOTE_AS4,0, NOTE_C5,0,NOTE_C5,0,NOTE_G4,0,NOTE_GS4};
        int noteDurations[] = {8, 4, 8, 8,16, 8,16,8,2,8,16,8,16,8,2,8,16,8,16,2};
        musica(melody,noteDurations, 20, 2);
      }
      else if (fase==2){
        WP=110;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        delay(1000);
      }
      else if (fase==3){
        enigma();
      }
    }
    else if (WP==110){
      dist(2);
    }
    else if (WP==120){
      dist(3);
    }

    
  //-----------------------------------------------------------------------------
  
    else if (WP==11){
      if (fase==0){
        WP=111;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
      }
      else if (fase==1){
        int bot = botoes();
        switch (bot)
        {
        case 1:
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Errou A");
          delay(2000);
          fase=2;
          break;
        case 2:
          lcd.clear();
          lcd.print("Acertou");
          WP++;
          fase=0;
          count=0;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          delay(2000);
          lcd.clear();
          break;
        case 3:
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Errou C");
          delay(2000);
          fase=2;
          break;
        default:
          lcd.setCursor(14,1);
          lcd.print(timeoff);
          timeoff=timeoff-1;
          if (count<5){
            lcd.setCursor(0,0);
            lcd.print(F("Ja sabes a      "));
            lcd.setCursor(0,1);
            lcd.print(F("Pergunta?     "));
            count++;
            delay(1000);
            if (timeoff<10){
              lcd.setCursor(15,1);
              lcd.print(" ");
            }
            
            if (timeoff==0){
              lcd.clear();
              lcd.print("time off");
              delay(2000);
              fase=2;
            }
          }
          else if (count>=5 && count<10){
            lcd.setCursor(0,0);
            lcd.print(F("Opcoes         "));
            lcd.setCursor(0,1);
            lcd.print(F("A B C         "));
            count++;
            delay(1000);
            if (timeoff<10){
              lcd.setCursor(15,1);
              lcd.print(" ");
            }
            if (timeoff==0){
              lcd.clear();
              lcd.print("time off");
              delay(2000);
              fase=2;
            }
          }
          else if(count==10){
            count=0;
          }
        }
      }
      else if (fase==2){
        if (punicao==0){
          //mandar para tras
          WP=121;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          lcd.setCursor(0,0);
          lcd.print(F("anda para tras"));
          lcd.setCursor(0,1);
          lcd.print(F("no trilho 1"));
          delay(2000);
          lcd.clear();
          punicao++;
          timeoff=30;
        }
        else if (punicao==1){
          //mandar para tras
          WP=131;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
          lcd.setCursor(0,0);
          lcd.print(F("anda para tras"));
          lcd.setCursor(0,1);
          lcd.print(F("no trilho 2"));
          delay(2000);
          lcd.clear();
          punicao=0;
          timeoff=30;
        } 
      }
    }
    else if (WP==111){
      dist(1);
    }
    else if (WP==121){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Distancia:");
      lcd.setCursor(0,1);
      lcd.print(range);  
      delay(500);
      if (range<100){
        chegaste();
        WP=11;
        fase=1;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        timeoff=30;
      }
    }
    else if (WP==131){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Distancia:");
      lcd.setCursor(0,1);
      lcd.print(range);  
      delay(500);
      if (range<100){
        chegaste();
        WP=11;
        fase=1;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        timeoff=30;
      }
    }
    
  //-----------------------------------------------------------------------------
  
    else if (WP==12){
      if (fase==0){
        WP=112;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        dirigir("N38 42.883", "W009 09.811");
      }
      else if (fase==1){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("encontra a cache"));
        delay(3000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Tira fotos"));
        lcd.setCursor(0,1);
        lcd.print(F("e diverte-te! :)"));
        delay(3000);
        lcd.clear();
        WP++;
        fase=0;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        delay(1000);   
      }
    }
    else if (WP==112){
      dist(2);
    }
    
  //-----------------------------------------------------------------------------
  
    else if (WP==13){
      if (fase==0){
        WP=113;
        Wire.beginTransmission(ADDRESS_B);
        Wire.write(WP);
        Wire.endTransmission();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Segue o caminho"));
        lcd.setCursor(0,1);
        lcd.print(F("e escuta!"));
        delay(1000);
      }
      else if (fase==1){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Chegaste!"));
        tone(5, NOTE_C4, 30000);
        delay(6000);
        noTone(5);
        servoLatch.write(servoUnlock);
        WP++;
        fase=0;
      }
    }
    else if (WP==113){
      if (range<150.0){
        tone(5, NOTE_C4, range*10);
        delay(range*13);
        noTone(5);
        if (range<120){
          WP=WP-100;
          fase++;
          Wire.beginTransmission(ADDRESS_B);
          Wire.write(WP);
          Wire.endTransmission();
        }
      }
    }  
    
  //-----------------------------------------------------------------------------
  
    else if (WP==14){
      if (fase==0){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Carrega no Enter"));
        lcd.setCursor(0,1);
        lcd.print(F("para fechar"));
        int bot = botoes();
        delay(500);
        if (bot==4){
          servoLatch.write(servoLock);
          fase=1;
        }
      }
      else if (fase==1){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Obrigado! =D"));
        lcd.setCursor(0,1);
        lcd.print(F("Pode desligar e"));
        delay(3000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("colocar a cache"));
        lcd.setCursor(0,1);
        lcd.print(F("no ponto inicial"));
        delay(3000);
      }
    }  
  }
}

//-----------------------------------------------------------------------------
  
void receiveEvent(int howMany){
  while (Wire.available() > 0){
    I2C_readAnything (range);
  } 
}
