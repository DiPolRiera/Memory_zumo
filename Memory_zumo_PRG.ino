
/*
 Al encender podra mover zumo con los botones en las direcciones izquierda, derecha y recto; zumo irá almacenando la trayectoria en un array.
 Tras 5 segundos de inactividad zumo almacenara el recorrido en la memoria EPROM, cargará los datos y los reproducirá automaticamente.
 */
 
#include <EEPROM.h>// EEPROM pre-installed library

#include <Wire.h>
#include <Zumo32U4.h>

Zumo32U4Encoders encoders;
Zumo32U4LCD lcd;
Zumo32U4Buzzer buzzer;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4ButtonC buttonC;

const char encoderErrorLeft[] PROGMEM = "!<c2";
const char encoderErrorRight[] PROGMEM = "!<e2";

char report[80];


/*
 ************Global Variables and Arrays**************
 */
 
unsigned long int value = 0; // almacena los valores de entrada
byte seq = 0; //almacena el numero actual de secuencias ejecutadas
byte seq_Array[50];// matriz para almacenar la secuencia de movimiento en términos de números enteros (1 para FWD, 2 para LEFT y así sucesivamente ..)
//contador para contar el número de veces que el programa pasa a través de una función de movimiento (fwd, lft, etc.)
int fwd_Counter = -1;
int lft_Counter = -1;
int rgt_Counter = -1;
int bwd_Counter = -1;
int stp_Counter = -1;

//Variables globales de "tiempo actual" para diferentes funciones de movimiento (fwd, lft, etc.)
unsigned long int current_Time0 = 0;//  FWD 
unsigned long int current_Time1 = 0;//  LEFT 
unsigned long int current_Time2 = 0;//  RIGHT 
unsigned long int current_Time3 = 0;//  BWD 
unsigned long int current_Time4 = 0;//  STOP

//tiempo total empleado por el zumo en ejecutar el movimiento (fwd, lft, etc.) para un contador de movimiento en particular
unsigned long int total_Fwd_Time[10];
unsigned long int total_Lft_Time[10];
unsigned long int total_Rgt_Time[10];
unsigned long int total_Bwd_Time[10];
unsigned long int total_Stp_Time[10];

bool seleccion = true; 
bool estado = true; // false: almacena  true: movimiento

bool bool_RPEAT = false;
bool bool_DEL = false;
bool bool_PLAYEPROM = false;

unsigned long int tiempo0 = millis();

#define STOP 0
#define LFT 1
#define RGT 2
#define FWD 3
#define BWD 4
#define RPEAT 5
#define DEL 6
#define PERST 7
#define PLAYEPROM 8
 
void setup() {

  // start serial communication
  Serial.begin(9600);
  del_From_Local_Mem();

}

void loop() {
  static uint8_t lastDisplayTime;
  static uint8_t displayErrorLeftCountdown = 0;
  static uint8_t displayErrorRightCountdown = 0;

// Errores 
  if ((uint8_t)(millis() - lastDisplayTime) >= 100)
  {
    lastDisplayTime = millis();

    int16_t countsLeft = encoders.getCountsLeft();
    int16_t countsRight = encoders.getCountsRight();

    bool errorLeft = encoders.checkErrorLeft();
    bool errorRight = encoders.checkErrorRight();

    if(encoders.checkErrorLeft())
    {
      // An error occurred on the left encoder channel.
      // Display it on the LCD for the next 10 iterations and
      // also beep.
      displayErrorLeftCountdown = 10;
      buzzer.playFromProgramSpace(encoderErrorLeft);
    }

    if(encoders.checkErrorRight())
    {
      // An error occurred on the left encoder channel.
      // Display it on the LCD for the next 10 iterations and
      // also beep.
      displayErrorRightCountdown = 10;
      buzzer.playFromProgramSpace(encoderErrorRight);
    }

    // Update the LCD with encoder counts and error info.
    lcd.clear();
    lcd.print(countsLeft);
    lcd.gotoXY(0, 1);
    lcd.print(countsRight);

    if (displayErrorLeftCountdown)
    {
      // Show an exclamation point on the first line to
      // indicate an error from the left encoder.
      lcd.gotoXY(7, 0);
      lcd.print('!');
      displayErrorLeftCountdown--;
    }

    if (displayErrorRightCountdown)
    {
      // Show an exclamation point on the second line to
      // indicate an error from the left encoder.
      lcd.gotoXY(7, 1);
      lcd.print('!');
      displayErrorRightCountdown--;
    }

    // Send the information to the serial monitor also.
    snprintf_P(report, sizeof(report),
        PSTR("%6d %6d %6d %6d"),
        countsLeft, countsRight, errorLeft, errorRight);
       
        
    Serial.println(report);
  }
  //________________________________________________________________
  if(estado){
    Serial.println("estado movimiento");    
  }
  else  
    Serial.println("estado almacena");

// Activa la pantalla de seleccion en caso de inactividad (10s)
 if (buttonA.isPressed() || buttonB.isPressed() || buttonC.isPressed()){
  tiempo0 = millis();
  estado = true;
 }

 if (millis() - tiempo0 >= 5000){
  tiempo0 = millis();
  estado = false;
  lcd.clear();
  lcd.print("STORE");
 }



  //------------------------------------------------------------

  // Distribucion de botones movimiento

  if (estado)
  {
    if (buttonA.isPressed())
   {
     value = LFT;
   }
    else if (buttonC.isPressed())
   {
      value = RGT;
   }
   else if (buttonB.isPressed())
   {
      value = FWD;
   }
   else
   {
     value = STOP;
   }
  }


// Distribucion de botones de almacenamiento
  else
  {    
    write_To_Permt_Mem();
    Read_Permt_Mem();
    go_In_Seq();   
  }

  delay(100);
  check_Inst(value);
  value=0;

}

void check_Inst(int value) {

  switch (value) {
    case FWD:
      go_Forward();
      delay(10);
      break;
    case LFT:
      go_Left();
      delay(10);
      break;
    case RGT:
      go_Right();
      delay(10);
      break;
    case BWD:
      go_Backward();
      delay(10);
      break;
    case STOP:
      go_Stop();
      delay(10);
      break;
    case RPEAT:
      go_In_Seq();
      delay(10);
      break;
    case DEL:
      del_From_Local_Mem();
      delay(10);
      write_To_Permt_Mem();
      delay(10);
      break;
    case PERST:
      write_To_Permt_Mem();
      delay(10);
      break;  
    case PLAYEPROM:
      Read_Permt_Mem();
      delay(10);
      break;   

                
     default:
      value = 0;
  }
}

void go_Forward() {
  movement_Inst_Fwd();

  current_Time0 = millis();
  int i = seq_Array[(seq - 1)];
  switch (i) {
    case 2:
      // tiempo total desde que se presiona el botón LEFT 
      total_Lft_Time[lft_Counter + 1] = (current_Time0 - current_Time1);
      lft_Counter++;
      break;

    case 3:
      // tiempo total desde que se presiona el botón RGT
      total_Rgt_Time[rgt_Counter + 1] = (current_Time0 - current_Time2);
      rgt_Counter++;
      break;

    case 4:
      total_Bwd_Time[bwd_Counter + 1] = (current_Time0 - current_Time3);
      bwd_Counter++;
      break;

    case 5:
      total_Stp_Time[stp_Counter + 1] = (current_Time0 - current_Time4);
      stp_Counter++;
      break;
  }

  seq_Array[seq] = 1;
  seq++;
}

void go_Left() {
  movement_Inst_Lft();

  current_Time1 = millis();
  int i = seq_Array[(seq - 1)];
  switch (i) {
    case 1:
      total_Fwd_Time[fwd_Counter + 1] = (current_Time1 - current_Time0);
      fwd_Counter++;
      break;

    case 3:
      total_Rgt_Time[rgt_Counter + 1] = (current_Time1 - current_Time2);
      rgt_Counter++;
      break;

    case 4:
      total_Bwd_Time[bwd_Counter + 1] = (current_Time1 - current_Time3);
      bwd_Counter++;
      break;

    case 5:
      total_Stp_Time[stp_Counter + 1] = (current_Time1 - current_Time4);
      stp_Counter++;
      break;
  }

  seq_Array[seq] = 2;
  seq++;
}

void go_Right() {
  movement_Inst_Rgt();

  current_Time2 = millis();
  int i = seq_Array[(seq - 1)];
  switch (i) {
    case 1:
      total_Fwd_Time[fwd_Counter + 1] = (current_Time2 - current_Time0);
      fwd_Counter++;
      break;

    case 2:
      total_Lft_Time[lft_Counter + 1] = (current_Time2 - current_Time1);
      lft_Counter++;
      break;

    case 4:
      total_Bwd_Time[bwd_Counter + 1] = (current_Time2 - current_Time3);
      bwd_Counter++;
      break;

    case 5:
      total_Stp_Time[stp_Counter + 1] = (current_Time2 - current_Time4);
      stp_Counter++;
      break;
  }

  seq_Array[seq] = 3;
  seq++;
}

void go_Backward() {
  movement_Inst_Bwd();

  current_Time3 = millis();
  int i = seq_Array[(seq - 1)];
  switch (i) {
    case 1:
      total_Fwd_Time[fwd_Counter + 1] = (current_Time3 - current_Time0);
      fwd_Counter++;
      break;

    case 2:
      total_Lft_Time[lft_Counter + 1] = (current_Time3 - current_Time1);
      lft_Counter++;
      break;

    case 3:
      total_Rgt_Time[rgt_Counter + 1] = (current_Time3 - current_Time2);
      rgt_Counter++;
      break;

    case 5:
      total_Stp_Time[stp_Counter + 1] = (current_Time3 - current_Time4);
      stp_Counter++;
      break;
  }

  seq_Array[seq] = 4;
  seq++;
}

void go_Stop() {
  movement_Inst_Stp();

  current_Time4 = millis();
  int i = seq_Array[(seq - 1)];
  switch (i) {
    case 1:
      total_Fwd_Time[fwd_Counter + 1] = (current_Time4 - current_Time0);
      fwd_Counter++;
      break;

    case 2:
      total_Lft_Time[lft_Counter + 1] = (current_Time4 - current_Time1);
      lft_Counter++;
      break;

    case 3:
      total_Rgt_Time[rgt_Counter + 1] = (current_Time4 - current_Time2);
      rgt_Counter++;
      break;

    case 4:
      total_Bwd_Time[bwd_Counter + 1] = (current_Time4 - current_Time3);
      bwd_Counter++;
      break;
  }

  seq_Array[seq] = 5;
  seq++;
}

void go_In_Seq() {

  lcd.clear();
  lcd.print("REPEAT");
    
  value = 0;
  for (int i = 0; i < (seq + 1); i++) {
    int value1 = 0;
    value1 = seq_Array[i];
    switch (value1) {
      case 1:
        static int j = 0;
        go_Forward_Seq(j);
        j++;
        break;
      case 2:
        static int k = 0;
        go_Left_Seq(k);
        k++;
        break;
      case 3:
        static int l = 0;
        go_Right_Seq(l);
        l++;
        break;
      case 4:
        static int m = 0;
        go_Backward_Seq(m);
        m++;
        break;
      case 5:
        static int n = 0;
        go_Stop_Seq(n);
        n++;
        break;
      default:
        j = 0; k = 0; l = 0; m = 0; n = 0;
    }
  }
}

void del_From_Local_Mem() {
  // Pone los contadores de movimiento a sus valores iniciales
  fwd_Counter = -1;
  lft_Counter = -1;
  rgt_Counter = -1;
  bwd_Counter = - 1;
  stp_Counter = - 1;

  //pone los tiempos totales de movimiento en su valores iniciales
  for (int i = 0; i < 10; i++) {
    total_Fwd_Time[i] = 0;
    total_Lft_Time[i] = 0;
    total_Rgt_Time[i] = 0;
    total_Bwd_Time[i] = 0;
    total_Stp_Time[i] = 0;
  }

  // Resetea el array de las secuencias de los movimientos 
  for (int i = 0; i < 50; i++) {
    seq_Array[i] = 0;
  }

  seq = 0;
  
}


/**********************************************************************************************************
     Copia los datos de los arrays a la memoria EPROM (es una memoria no volatil)
************************************************************************************************************/

void write_To_Permt_Mem(){
  // direccion de la memoria EPROM
  EEPROM.write(100,seq);
    
  //guarda la secuencia de movimiento
  for(int i=0; i<seq; i++){ 
  EEPROM.write(2*i,seq_Array[i]);
  }

  //almacena el tiempo entre dos movimientos sucesivos
  for(int i=1; i<seq+1; i++){           
  if(seq_Array[i-1]==1){
    static byte a=0;
    EEPROM.write(2*i-1,(total_Fwd_Time[a])/1000);// Note: One location can store maximum value of 255, hence the time is divided by 1000 here. And then multiplied by 1000 while retreiving the data from EEPROM location
    a++;
    }
  else if(seq_Array[i-1]==2){
    static byte b=0;
    EEPROM.write(2*i-1,(total_Lft_Time[b])/1000);
    b++;
    }
  else if(seq_Array[i-1]==3){
    static byte c=0;
    EEPROM.write(2*i-1,(total_Rgt_Time[c])/1000);
    c++;
    }
  else if(seq_Array[i-1]==4){
    static byte d=0;
    EEPROM.write(2*i-1,(total_Bwd_Time[d])/1000);  
    d++;
    }
  else if(seq_Array[i-1]==5){
    static byte e=0;
    EEPROM.write(2*i-1,(total_Stp_Time[e])/1000);  
    e++;
    }             
  }
 } 

 
/**********************************************************************************************************
     Lee la secuencia almacenada en la EPROM
************************************************************************************************************/

void Read_Permt_Mem(){
   byte x = EEPROM.read(100);
   for(int i=0; i<x+1; i++){
    byte r = EEPROM.read(2*i);
    switch(r){
      case 1:
        movement_Inst_Fwd();
        break;
      case 2:
        movement_Inst_Lft();
        break;
      case 3:
        movement_Inst_Rgt();
        break;
      case 4:
        movement_Inst_Bwd();
        break; 
      case 5:
        movement_Inst_Stp();
        break;                          
      }
     delay((EEPROM.read(i+1))*1000);    // multiplied by thousand because the original time was divided by 1000 while storing in EEPROM.
    }
  }
 
/**********************************************************************************************************
     Mueve el zumo en una dirección durante el tiempo especificado 
************************************************************************************************************/
void go_Forward_Seq(int j) {
  movement_Inst_Fwd();
  delay(total_Fwd_Time[j]);
}

void go_Left_Seq(int k) {
  movement_Inst_Lft();
  delay(total_Lft_Time[k]);
}

void go_Right_Seq(int l) {
  movement_Inst_Rgt();
  delay(total_Rgt_Time[l]);
}

void go_Backward_Seq(int m) {
  movement_Inst_Bwd();
  delay(total_Bwd_Time[m]);
}

void go_Stop_Seq(int n) {
  movement_Inst_Stp();
  delay(total_Stp_Time[n]);
}

/*********************************************************************************************
          Instrucciones básicas de movimiento
**********************************************************************************************/
void movement_Inst_Fwd(void) {
  Serial.println("Going_Forward");
  motors.setSpeeds(100, 100);
}

void movement_Inst_Lft(void) {
  Serial.println("Going_Left");
  motors.setSpeeds(0, 100);
  delay(500);
}

void movement_Inst_Rgt(void) {
  Serial.println("Going_Right"); 
  motors.setSpeeds(100, 0);
  delay(500);

}

void movement_Inst_Bwd(void) {
  Serial.println("Going_Backward"); 
  motors.setSpeeds(-100, -100);
}

void movement_Inst_Stp(void) {
  Serial.println("Stopping");
  motors.setSpeeds(0, 0);
}
