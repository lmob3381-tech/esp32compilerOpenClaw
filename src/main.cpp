/*
  MULTI-GAME: BREAKOUT + SNAKE
  Long Press SW 4 detik = ganti game
  OLED 128x64, ESP32, Joystick VRX=34 VRY=35 SW=32
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define VRX_PIN 34
#define VRY_PIN 35
#define SW_PIN 32

// ===== PILIH GAME =====
enum CurrentGame { GAME_BREAKOUT, GAME_SNAKE };
CurrentGame currentGame = GAME_BREAKOUT;

// ===== GLOBAL UI =====
enum AppState { MENU, SETTINGS, ABOUT, PLAYING, GAME_OVER, WIN };
AppState state = MENU;
int menuIndex = 0;
unsigned long btnPressTime = 0;
bool btnHeld = false;

// ========== BREAKOUT CODE ==========
#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 3
#define PADDLE_Y (SCREEN_HEIGHT - 6)
#define BALL_SIZE 2
#define BRICK_ROWS 3
#define BRICK_COLS 10
#define BRICK_WIDTH 12
#define BRICK_HEIGHT 5
#define BRICK_GAP 1
#define BRICK_TOP 14

bool bricks[BRICK_ROWS][BRICK_COLS];
float paddleX, ballX, ballY, ballVX, ballVY;
int b_score = 0, b_lives = 3;

void resetBricks(){ for(int r=0;r<BRICK_ROWS;r++)for(int c=0;c<BRICK_COLS;c++)bricks[r][c]=true; }
int countBricks(){ int cnt=0; for(int r=0;r<BRICK_ROWS;r++)for(int c=0;c<BRICK_COLS;c++)if(bricks[r][c])cnt++; return cnt; }

void startBreakout(){
  b_score=0; b_lives=3; resetBricks();
  paddleX=(SCREEN_WIDTH-PADDLE_WIDTH)/2.0; ballX=SCREEN_WIDTH/2.0; ballY=PADDLE_Y-6; ballVX=1.4; ballVY=-1.6;
  state=PLAYING;
}

void updateBreakout(){
  int raw=analogRead(VRX_PIN); float delta=(raw-2048)/2048.0; if(abs(delta)<0.15)delta=0; paddleX+=delta*3.2;
  if(paddleX<0)paddleX=0; if(paddleX>SCREEN_WIDTH-PADDLE_WIDTH)paddleX=SCREEN_WIDTH-PADDLE_WIDTH;

  ballX+=ballVX; ballY+=ballVY;
  if(ballX<=0||ballX>=SCREEN_WIDTH-BALL_SIZE)ballVX=-ballVX;
  if(ballY<=BRICK_TOP-4)ballVY=-ballVY;
  if(ballY+BALL_SIZE>=PADDLE_Y && ballX+BALL_SIZE>=paddleX && ballX<=paddleX+PADDLE_WIDTH && ballVY>0){ ballVY=-abs(ballVY); }

  for(int r=0;r<BRICK_ROWS;r++)for(int c=0;c<BRICK_COLS;c++)if(bricks[r][c]){
    int bx=c*(BRICK_WIDTH+BRICK_GAP); int by=BRICK_TOP+r*(BRICK_HEIGHT+BRICK_GAP);
    if(ballX+BALL_SIZE>=bx && ballX<=bx+BRICK_WIDTH && ballY+BALL_SIZE>=by && ballY<=by+BRICK_HEIGHT){
      bricks[r][c]=false; ballVY=-ballVY; b_score+=10;
    }
  }
  if(ballY>SCREEN_HEIGHT){ b_lives--; if(b_lives<=0)state=GAME_OVER; else{ballX=SCREEN_WIDTH/2.0; ballY=PADDLE_Y-6;} }
  if(countBricks()==0)state=WIN;
}

void drawBreakout(){
  display.clearDisplay();
  display.fillRect(0,0,SCREEN_WIDTH,10,SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK); display.setCursor(2,1); display.print("BOUT S:");display.print(b_score); display.print(" L:");display.print(b_lives);
  display.setTextColor(SSD1306_WHITE);
  for(int r=0;r<BRICK_ROWS;r++)for(int c=0;c<BRICK_COLS;c++)if(bricks[r][c]){
    int bx=c*(BRICK_WIDTH+BRICK_GAP); int by=BRICK_TOP+r*(BRICK_HEIGHT+BRICK_GAP);
    display.drawRect(bx,by,BRICK_WIDTH,BRICK_HEIGHT,SSD1306_WHITE);
  }
  display.fillRect(paddleX,PADDLE_Y,PADDLE_WIDTH,PADDLE_HEIGHT,SSD1306_WHITE);
  display.fillRect(ballX,ballY,BALL_SIZE,BALL_SIZE,SSD1306_WHITE);
  display.display();
}

// ========== SNAKE CODE ==========
#define GRID_SIZE 4
int snakeX[100], snakeY[100], snakeLen=3;
int foodX, foodY; int dir=1; // 0=up 1=right 2=down 3=left
int s_score=0; unsigned long lastMove=0;

void spawnFood(){ foodX=random(0,SCREEN_WIDTH/GRID_SIZE); foodY=random(3,SCREEN_HEIGHT/GRID_SIZE); }
void startSnake(){
  snakeLen=3; dir=1; s_score=0;
  for(int i=0;i<snakeLen;i++){ snakeX[i]=5-i; snakeY[i]=5; }
  spawnFood(); state=PLAYING; lastMove=millis();
}

void updateSnake(){
  if(millis()-lastMove<120) return; // kecepatan snake
  lastMove=millis();

  int rawX=analogRead(VRX_PIN); int rawY=analogRead(VRY_PIN);
  if(rawX<1500 && dir!=1)dir=3; if(rawX>2500 && dir!=3)dir=1;
  if(rawY<1500 && dir!=2)dir=0; if(rawY>2500 && dir!=0)dir=2;

  for(int i=snakeLen-1;i>0;i--){ snakeX[i]=snakeX[i-1]; snakeY[i]=snakeY[i-1]; }
  if(dir==0)snakeY[0]--; if(dir==1)snakeX[0]++; if(dir==2)snakeY[0]++; if(dir==3)snakeX[0]--;

  if(snakeX[0]<0||snakeX[0]>=SCREEN_WIDTH/GRID_SIZE||snakeY[0]<2||snakeY[0]>=SCREEN_HEIGHT/GRID_SIZE)state=GAME_OVER;
  for(int i=1;i<snakeLen;i++)if(snakeX[0]==snakeX[i]&&snakeY[0]==snakeY[i])state=GAME_OVER;

  if(snakeX[0]==foodX && snakeY[0]==foodY){ snakeLen++; s_score+=10; spawnFood(); }
}

void drawSnake(){
  display.clearDisplay();
  display.fillRect(0,0,SCREEN_WIDTH,10,SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK); display.setCursor(2,1); display.print("SNAKE S:");display.print(s_score);
  display.setTextColor(SSD1306_WHITE);
  for(int i=0;i<snakeLen;i++) display.fillRect(snakeX[i]*GRID_SIZE, snakeY[i]*GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  display.fillRect(foodX*GRID_SIZE, foodY*GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  display.display();
}

// ========== UI & SWITCHER ==========
void drawMenu(){
  display.clearDisplay();
  display.setCursor(35,0); display.print(currentGame==GAME_BREAKOUT?"BREAKOUT":"SNAKE");
  display.drawLine(0,9,SCREEN_WIDTH,9,SSD1306_WHITE);
  String items[3]={"[>] PLAY","[s] SETTINGS","[i] GANTI GAME"};
  for(int i=0;i<3;i++){
    if(i==menuIndex){ display.fillRect(0,14+i*11,SCREEN_WIDTH,9,SSD1306_WHITE); display.setTextColor(SSD1306_BLACK); }
    else display.setTextColor(SSD1306_WHITE);
    display.setCursor(4,15+i*11); display.print(items[i]);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2,55); display.print("Tahan 4s utk ganti game");
  display.display();
}

void drawGameOver(){
  display.clearDisplay(); display.setCursor(30,20); display.print("GAME OVER");
  display.setCursor(25,35); display.print("SW= Menu"); display.display();
}
void drawWin(){
  display.clearDisplay(); display.setCursor(38,20); display.print("MENANG!");
  display.setCursor(25,35); display.print("SW= Menu"); display.display();
}

void switchGame(){
  if(currentGame==GAME_BREAKOUT) currentGame=GAME_SNAKE;
  else currentGame=GAME_BREAKOUT;
  state=MENU; drawMenu();
}

void setup(){
  Wire.begin(21,22);
  pinMode(SW_PIN,INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  randomSeed(analogRead(33));
  drawMenu();
}

void loop(){
  bool btn = digitalRead(SW_PIN)==LOW;

  // --- Deteksi Long Press 4 detik ---
  if(btn &&!btnHeld){ btnPressTime=millis(); btnHeld=true; }
  if(!btn && btnHeld){
    unsigned long holdTime = millis()-btnPressTime;
    btnHeld=false;
    if(holdTime>=4000){ // LONG PRESS
      switchGame();
      delay(300); // debounce
    }else if(holdTime>50){ // SHORT PRESS
      if(state==MENU){
        if(menuIndex==0){ if(currentGame==GAME_BREAKOUT)startBreakout(); else startSnake(); }
        if(menuIndex==2)switchGame();
      }
      if(state==GAME_OVER||state==WIN)state=MENU;
    }
  }

  // --- Navigasi Menu ---
  int joyY=analogRead(VRY_PIN);
  if(state==MENU && millis()-lastMove>200){
    if(joyY<1500)menuIndex=(menuIndex+2)%3;
    if(joyY>2500)menuIndex=(menuIndex+1)%3;
    if(joyY<1500||joyY>2500){drawMenu(); lastMove=millis();}
  }

  // --- Update Game ---
  if(state==PLAYING){
    if(currentGame==GAME_BREAKOUT)updateBreakout();
    else updateSnake();
  }

  // --- Draw ---
  if(state==PLAYING){
    if(currentGame==GAME_BREAKOUT)drawBreakout();
    else drawSnake();
  }
  if(state==GAME_OVER)drawGameOver();
  if(state==WIN)drawWin();

  delay(20);
}
