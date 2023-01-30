#include <LiquidCrystal.h>
#include <SPI.h>
#include <Wire.h>
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define EEPROM_I2C_ADDRESS 0x50

//JOYSTICK
int JS_X= 0;
int JS_Y= 0;
int JS_BTN= 12;
int btn;
int pause= 0;
bool joystick= false;

//INITIAL SETUP
int MAP[16][2];
int PACMAN_POSX= 0;
int PACMAN_POSY= 0;
int GHOST_POSX= 15;
int GHOST_POSY= 0;
int x;
int y;

//GAME CONDITIONS
int PLAYING= false;
bool LEVEL_COMPLETED= false;
bool VICTORY= false;
int SCORE= 0;
int HIGHSCORE= 0;

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

int delay_gcd;
const unsigned short tasksNum = 5;
task tasks[tasksNum];

enum SM1_states {off, on}; //ON AND OFF STATE
int SM1_Tick(int state1) {
  switch(state1) {
    case off:
      btn= digitalRead(JS_BTN);
      if(btn == HIGH) {
        state1= off;
      }
      else {
        state1= on;
      }
      break;
    case on:
      btn= digitalRead(JS_BTN);
      if(btn == HIGH) {
        state1= on;
      }
      else {
        state1= off;
      }
      break;
  }
  switch(state1) {
    case off:
      btn= digitalRead(JS_BTN);
      if(btn == LOW) {
        VICTORY= false;
        RESETGAME();
      }
      break;
    case on:
      PLAYING= true;
      lcd.display();
      break;
  }
  return state1;
}

enum SM3_States {SM3_INIT, LEFT, RIGHT, UP, DOWN, PAUSE}; //JOYSTICK
int SM3_Tick(int state3) {
  switch(state3) {
    case SM3_INIT:
      JS_X= analogRead(A2);
      JS_Y= analogRead(A1);
      if(JS_Y > 600) {
        state3= UP;
      }
      else if(JS_Y < 400) {
        state3= DOWN;
      }
      else if(JS_X < 400) {
        state3= LEFT;
      }
      else if(JS_X > 600) {
        state3= RIGHT;
      }
      else {
        state3= SM3_INIT;
      }
      break;
    case LEFT:
      JS_X= analogRead(A2);
      if(JS_X < 400) {
        state3= LEFT;
      }
      else {
        state3= SM3_INIT;
      }
      break;
    case RIGHT:
      JS_X= analogRead(A2);
      if(JS_X > 600) {
        state3= RIGHT;
      }
      else {
        state3= SM3_INIT;
      }
      break;
    case UP:
      JS_Y= analogRead(A1);
      if(JS_Y > 600) {
        state3= UP;
      }
      else {
        state3= SM3_INIT;
      }
      break;
    case DOWN:
      JS_Y= analogRead(A1);
      if(JS_Y < 400) {
        state3= DOWN;
      }
      else {
        state3= SM3_INIT;
      }
      break;
    case PAUSE:
      if(pause < 500) {
        state3= PAUSE;
      }
      break;
  }
  switch(state3) {
    case SM3_INIT:
      break;
    case UP:
      joystick= true;
      x= 0;
      y= -1;
      break;
    case DOWN:
      joystick= true;
      x= 0;
      y= 1;
      break;
    case LEFT:
      joystick= true;
      x= -1;
      y= 0;
      break;
    case RIGHT:
      joystick= true;
      x= 1;
      y= 0;
      break;
    case PAUSE:
      break;
  }
  return state3;
}

enum SM4_states {SM4_INIT, PLAYER}; //PACMAN
int SM4_Tick(int state4) {
  switch(state4) {
    case SM4_INIT:
      if(joystick == true) {
        state4= PLAYER;
      }
      else {
        state4= SM4_INIT;
      }
      break;
  }
  switch(state4) {
    case SM4_INIT:
      break;
    case PLAYER:
      int INITX= PACMAN_POSX;
      int INITY= PACMAN_POSY;
      LEVEL_COMPLETED= true;
      //LIMITATIONS
      if((PACMAN_POSX + x > -1) && (PACMAN_POSX + x < 16)) {
        PACMAN_POSX= PACMAN_POSX + x;
      }
      if((PACMAN_POSY + y > -1) && (PACMAN_POSY + y < 2)) {
        PACMAN_POSY= PACMAN_POSY + y;
      }
      x= 0;
      y= 0;
      //MOVEMENT
      lcd.setCursor(PACMAN_POSX, PACMAN_POSY);
      lcd.write(byte(0));
      lcd.setCursor(INITX, INITY);

      if((PACMAN_POSX != INITX) || (PACMAN_POSY != INITY)) {
        lcd.print(" "); //EAT PELLET
      }

      if(MAP[PACMAN_POSX][PACMAN_POSY]) {
        MAP[PACMAN_POSX][PACMAN_POSY]= false;
      }
      for(int i= 0; i < 16; i++) {
        for(int j= 0; j < 2; j++) {
          if(MAP[i][j]) {
            LEVEL_COMPLETED= false;
          }
        }
      }
      break;
  }
  return state4;
}

enum SM5_states {SM5_INIT, GHOST_MOVEMENT}; //GHOST
int SM5_Tick(int state5) {
  switch(state5) {
    case SM5_INIT:
      if(PLAYING == true) {
        state5= GHOST_MOVEMENT;
      }
      else {
        state5= SM5_INIT;
      }
      break;
  }
  switch(state5) {
    case SM5_INIT:
      break;
    case GHOST_MOVEMENT:
      int INITX= GHOST_POSX;
      int INITY= GHOST_POSY;
      //GHOST MOVE
      if(GHOST_POSY < PACMAN_POSY) {
        GHOST_POSY++;
      }
      else if(GHOST_POSY > PACMAN_POSY) {
        GHOST_POSY--;
      }
      else if(GHOST_POSX < PACMAN_POSX) {
        GHOST_POSX++;
      }
      else if(GHOST_POSX > PACMAN_POSX) {
        GHOST_POSX--;
      }
      //SPRITE
      lcd.setCursor(GHOST_POSX, GHOST_POSY);
      lcd.write(1);
      lcd.setCursor(INITX, INITY);
      //TRAIL
      if((INITX != GHOST_POSX) || (INITY != GHOST_POSY)) {
        if(MAP[INITX][INITY]) {
          lcd.write(2);
        }
        else {
          lcd.print(" ");
        }
      }
      break;
  }
  return state5;
}

enum SM6_states {WIN, LOSE, WAITING}; //WIN OR LOSE
int SM6_Tick(int state6) {
  switch(state6) {
    case WAITING:
      if((LEVEL_COMPLETED == true) && (PACMAN_POSX != GHOST_POSX) && (PACMAN_POSY != GHOST_POSY)) {
        state6= WIN;
      }
      else if((LEVEL_COMPLETED == false) && (PACMAN_POSX == GHOST_POSX) && (PACMAN_POSY == GHOST_POSY)) {
        state6= LOSE;
      }
      else {
        state6= WAITING;
      }
      break;
    case WIN:
      if((LEVEL_COMPLETED == true) && (PACMAN_POSX != GHOST_POSX) && (PACMAN_POSY != GHOST_POSY)) {
        state6= WIN;
      }
      else if((LEVEL_COMPLETED == false) && (PACMAN_POSX == GHOST_POSX) && (PACMAN_POSY == GHOST_POSY)) {
        state6= LOSE;
      }
      else {
        state6= WAITING;
      }
    case LOSE:
      if((LEVEL_COMPLETED == true) && (PACMAN_POSX != GHOST_POSX) && (PACMAN_POSY != GHOST_POSY)) {
        state6= WIN;
      }
      else if((LEVEL_COMPLETED == false) && (PACMAN_POSX == GHOST_POSX) && (PACMAN_POSY == GHOST_POSY)) {
        state6= LOSE;
      }
      else {
        state6= WAITING;
      }
      break;
  }
  switch(state6) {
    case WAITING:
      break;
    case WIN:
      VICTORY= true;
      lcd.setCursor(0, 0);
      lcd.print("YOU WON!");
      delay(2000);
      RESETGAME();
      break;
    case LOSE:
      VICTORY= false;
      lcd.setCursor(0, 0);
      lcd.print("GAME OVER");
      delay(2000);
      RESETGAME();
      break;
  }
  return state6;
}

void RESETGAME() {
  //SCORE
  if(VICTORY == true) {
  SCORE++;
  }
  if(VICTORY == false) {
  SCORE= 0;
  }
  HIGHSCORE= readEEPROM(0, EEPROM_I2C_ADDRESS);
  if(SCORE > HIGHSCORE) {
    writeEEPROM(0, SCORE, EEPROM_I2C_ADDRESS);
  }
  //RESET
  PACMAN_POSX= 0;
  PACMAN_POSY= 0;
  GHOST_POSX= 15;
  GHOST_POSY= 0;
  PLAYING= false;
  LEVEL_COMPLETED= false;
  //STARTUP
  lcd.setCursor(0, 0);
  lcd.write(byte(0));
  lcd.print("   PAC-MAN!   ");
  lcd.write(byte(3));
  lcd.setCursor(0, 1);
  lcd.print("HIGH: ");
  lcd.print(HIGHSCORE);
  lcd.print(" SCORE: ");
  lcd.print(SCORE);
  delay(5000);
  lcd.clear();
  //SETUP
  for(int i= 0; i < 17; i++) {
    for(int j= 0; j < 2; j++) {
      MAP[i][j]= true;
      lcd.setCursor(i-1, j-1);
      lcd.write(2);
    }
  }
}

void writeEEPROM(int addr, int value, int i2c_addr) {
  Wire.beginTransmission(i2c_addr);
  Wire.write((int)(addr >> 8));
  Wire.write((int)(addr & 0xFF));
  Wire.write(value);
  Wire.endTransmission();
}

int readEEPROM(int addr, int i2c_addr) {
  int rcv= 0xFF;
  Wire.beginTransmission(i2c_addr);
  Wire.write((int)(addr >> 8));
  Wire.write((int)(addr & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(i2c_addr, 1);
  rcv= Wire.read();
  return rcv;
}

void setup() {
  SPI.begin();
  Wire.begin();
  lcd.begin(16, 2);
  pinMode(JS_BTN, INPUT_PULLUP);
  
  unsigned char i= 0;
  tasks[i].state= off;
  tasks[i].period= 100;
  tasks[i].elapsedTime= 0;
  tasks[i].TickFct= &SM1_Tick;
  i++;
  tasks[i].state= SM3_INIT;
  tasks[i].period= 100;
  tasks[i].elapsedTime= 0;
  tasks[i].TickFct= &SM3_Tick;
  i++;
  tasks[i].state= SM4_INIT;
  tasks[i].period= 100;
  tasks[i].elapsedTime= 0;
  tasks[i].TickFct= &SM4_Tick;
  i++;
  tasks[i].state= SM5_INIT;
  tasks[i].period= 1000;
  tasks[i].elapsedTime= 0;
  tasks[i].TickFct= &SM5_Tick;
  i++;
  tasks[i].state= WAITING;
  tasks[i].period= 100;
  tasks[i].elapsedTime= 0;
  tasks[i].TickFct= &SM6_Tick;

  //SPRITES
  byte PACMAN[8]= {
    B00000,
    B01110,
    B11111,
    B11110,
    B11000,
    B11111,
    B01110,
    B00000
  };
  byte PACMAN2[8]= {
    B00000,
    B01110,
    B11111,
    B01111,
    B00011,
    B11111,
    B01110,
    B00000
  };
  byte GHOST[8]= {
    B00000,
    B01110,
    B11111,
    B10101,
    B11111,
    B11111,
    B10101,
    B00000
  };
  byte PELLET[8]= {
    B00000,
    B00000,
    B00000,
    B00000,
    B00100,
    B00000,
    B00000,
    B00000
  };
  lcd.createChar(0, PACMAN);
  lcd.createChar(3, PACMAN2);
  lcd.createChar(1, GHOST);
  lcd.createChar(2, PELLET);

  RESETGAME();
  Serial.begin(9600);
}

void loop() {
  unsigned char i;
  //Serial.print(HIGHSCORE);
  for (i = 0; i < tasksNum; ++i) {
    if ( (millis() - tasks[i].elapsedTime) >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis(); // Last time this task was ran
    }
  }
}