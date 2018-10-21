//fixed constatnts
#define MAX_X 8
#define MAX_Y 5
#define MAX_Z 8
//in miliseconds
#define LAYER_TIMEOUT 4
#define MOVE_SNAKE_DELAY 750 

//pins for the led cube
const int LED_PIN_X[MAX_Y][MAX_X] = {
    {2,3,4,5,6,7,8,9},
    {22,23,24,25,26,27,28,29},
    {30,31,32,33,34,35,36,37},
    {38,39,40,41,42,43,44,45},
    {46,47,48,49,50,51,52,53}
};

const int LED_PIN_Z[MAX_Z] = {
  10,11,12,13,14,15,16,17
};

enum SnakeDirection {
  x_up,
  x_down,
  y_up,
  y_down,
  z_up,
  z_down
};
SnakeDirection snakeDirection;

enum GameState {
  on,
  win,
  ate_body,
  hit_wall
};
GameState gamestate;

struct CubeElement {
  unsigned char x;
  unsigned char y;
  unsigned char z;
  bool operator==(const CubeElement &ce) const;
  void operator=(const CubeElement &ce);
};

bool CubeElement::operator==(const CubeElement &ce) const {
  return (x == ce.x) && (y == ce.y) && (z == ce.z);
}

void CubeElement::operator=(const CubeElement &ce) {
  x = ce.x;
  y = ce.y;
  z = ce.z;
}

int player_score = 0;
int best_score = 0;
int snakeSize = 320;
CubeElement snake[320] = {0};
CubeElement apple;

//ledState[z][y][x]
bool ledState[8][5][8] = {0};



void setup() {
  // put your setup code here, to run once:
  snakeDirection = z_up;

  for (int i = 0; i < snakeSize; i++) {
    snake[i].x = 0;
    snake[i].y = 0;
    snake[i].z = 0;
  }

  for (int z = 0; z < 8; z++) {
    for (int y = 0; y < 5; y++) {
      for (int x = 0; x < 8; x++) {
        ledState[z][y][x] = false;
      }
    }
  }

  //set the led cube pins as outputs
  for(int x=0; x<MAX_X; x++){
    for(int y=0; y<MAX_Y; y++){
      pinMode(LED_PIN_X[y][x], OUTPUT);
    }
  }
  //set the led cube z pins output
  for(int z=0; z<MAX_Z; z++){
    pinMode(LED_PIN_Z[z], OUTPUT);
  }

  //Ann

  //Jackie
  //init snake at (0,0,0)
  //init apple randomly
  //init gamestate = on
  randomSeed(analogRead(0));
  apple.x = random(8);
  apple.y = random(1,5);
  apple.z = random(8);
  
  gamestate = on;
  ledState[snake[0].z][snake[0].y][snake[0].x] = true;
  ledState[apple.z][apple.y][apple.x] = true;

  //Gio
  pinMode(LED_BUILTIN, OUTPUT);

}


bool upd_ledmtx() {
  //storing snake head
  //for the later body update
  CubeElement prev_head;
  prev_head = snake[0];

  //updating the snake head
  gamestate = hit_wall;
  switch (snakeDirection) {
  case x_up:
    if (snake[0].x == 7) return false;
    snake[0].x++;
    break;

  case x_down:
    if (snake[0].x == 0) return false;
    snake[0].x--;
    break;

  case y_up:
    if (snake[0].y == 4) return false;
    snake[0].y++;
    break;

  case y_down:
    if (snake[0].y == 0) return false;
    snake[0].y--;
    break;

  case z_up:
    if (snake[0].z == 7) return false;
    snake[0].z++;
    break;

  case z_down:
    if (snake[0].z == 0) return false;
    snake[0].z--;
    break;

  default: break;
  }
  gamestate = on;

  //ate its body
  for (int i = 1; i < snakeSize; i++) {
    if (snake[0] == snake[i]) {
      gamestate = ate_body;
      return false;
    }
  }

  //updating the led mtx
  //lighting up head's new position
  ledState[snake[0].z][snake[0].y][snake[0].x] = true;

  //got apple, length+1
  if (snake[0] == apple) {
    player_score++;
    snakeSize++;
  }
  //erasing previous tail's position
  else {
    if (snakeSize ==  1)
      ledState[prev_head.z][prev_head.y][prev_head.x] = false;
    else
      ledState[snake[snakeSize - 1].z][snake[snakeSize - 1].y][snake[snakeSize - 1].x] = false;
  }

  //updating the snake body 
  for (int i = snakeSize - 1; i > 0; i--) {
    if (i == 1) snake[i] = prev_head;
    else snake[i] = snake[i - 1];
  }

  //generate new apple 
  if (snake[0] == apple) {
    int cnt = random(320 - snakeSize);
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 5; j++) {
        for (int k = 0; k < 8; k++) {
          //find the cnt'th spot in led that is free
          if (ledState[i][j][k] == false) cnt--;
          if (cnt < 0) {
            apple.x = k;
            apple.y = j;
            apple.z = i;

            ledState[apple.z][apple.y][apple.x] = true;
            return true;
          }
        }
      }
    }
    
  }
  else
    ledState[apple.z][apple.y][apple.x] = true;
  return true;
}

/**
 * Reads the current values of ledState. If the value is true, then it will turn the led on
 * It loops through all the layers, turning only 1 layer on at a time.
 * The delay is set to LAYER_TIMEOUT
 */
void setLedStateToCube(){
  for(int z=0; z<MAX_Z; z++){
    for(int y=0; y<MAX_Y; y++){
      for(int x=0; x<MAX_X; x++){
        if(ledState[z][y][x]){
          digitalWrite(LED_PIN_X[y][x], HIGH);   
        }
        else{
          digitalWrite(LED_PIN_X[y][x], LOW);
        }
      }
    }
    digitalWrite(LED_PIN_Z[z], HIGH);
    //show the current layer before clearing to the next one
    delay(LAYER_TIMEOUT);
    //turn the layer off before starting the next one
    digitalWrite(LED_PIN_Z[z], LOW);
  }
  return;
}

/**
 * We want to keep the appearance that all the leds are on at the same time,
 * but once we loop through the MAX_Z layers, if we return to update the snake location,
 * we will move the snake once every (8*LAYER_TIMEOUT) (once every 32 milisecons). To 
 * prevent this, we will draw the led cube until MOVE_SNAKE_DELAY has been reached. At
 * which point, we will return to allow the snake position to update.
 */
void drawLedCube(){
  for(int i=0; i<(MOVE_SNAKE_DELAY/LAYER_TIMEOUT*MAX_Z); i++)
    setLedStateToCube();
}


void loop() {
  // put your main code here, to run repeatedly:

  //Ann
  getButtonInput();

  //Jackie
  upd_ledmtx();

  //Gio
  drawLedCube();
}

void getButtonInput() {

}
