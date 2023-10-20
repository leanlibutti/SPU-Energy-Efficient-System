#include <Servo.h>
#include <IRremote.h>
//#include <IRrecv.h>
//#include <IRutils.h>
// an IR detector/demodulator is connected to GPIO pin 2
uint16_t RECV_PIN = 5;

IRrecv irrecv(RECV_PIN,220);

decode_results results;

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

/*unsigned int rawOn[100] = {59506, 
  4300, 4350, 550, 1550, 550, 550, 550, 1600, 500, 1600, 
  550, 500, 550, 550, 550, 1550, 550, 550, 550, 500, 
  550, 1600, 550, 500, 550, 550, 500, 1600, 550, 1600, 
  500, 550, 550, 1600, 500, 550, 550, 500, 550, 1600, 
  500, 1600, 550, 1600, 550, 1600, 500, 1600, 550, 1600, 
  500, 1600, 550, 1600, 550, 500, 550, 550, 500, 550, 
  550, 500, 550, 550, 550, 500, 550, 1600, 550, 500, 
  550, 550, 550, 500, 550, 550, 500, 550, 550, 500, 
  550, 500, 550, 550, 500, 1600, 550, 1600, 550, 1600, 
  500, 1600, 550, 1600, 500, 1600, 550, 1600, 500};

unsigned int rawOff[100] = {15580, 
  4350, 4300, 550, 1600, 550, 500, 550, 1600, 500, 1650, 
  500, 550, 550, 500, 550, 1600, 500, 550, 550, 500, 
  550, 1600, 550, 500, 550, 550, 550, 1550, 550, 1600, 
  550, 500, 550, 1600, 550, 500, 550, 1600, 550, 1550, 
  550, 1600, 550, 1600, 500, 550, 550, 1600, 500, 1600, 
  550, 1600, 550, 500, 550, 550, 550, 500, 550, 550, 
  550, 1550, 550, 550, 550, 500, 550, 1600, 500, 1600, 
  550, 1600, 500, 550, 550, 550, 550, 500, 550, 550, 
  500, 550, 550, 500, 550, 550, 500, 550, 550, 1600, 
  500, 1600, 550, 1600, 500, 1650, 500, 1600, 500};*/

uint16_t rawOn[100] = {61606, 
  4350, 4300, 600, 1500, 600, 500, 600, 1500, 600, 1550, 
  600, 450, 650, 450, 600, 1500, 600, 500, 650, 400, 
  600, 1550, 600, 450, 600, 450, 600, 1550, 600, 1550, 
  550, 500, 600, 1500, 650, 450, 600, 450, 650, 1500, 
  600, 1500, 650, 1500, 600, 1550, 600, 1500, 600, 1550, 
  650, 1450, 650, 1500, 600, 450, 650, 450, 600, 450, 
  650, 400, 650, 450, 600, 450, 600, 1550, 600, 1500, 
  650, 450, 600, 450, 600, 500, 600, 450, 600, 450, 
  650, 450, 600, 450, 600, 450, 650, 1500, 600, 1750, 
  400, 1500, 650, 1500, 600, 1500, 600, 1550, 600};

uint16_t rawOff[100] = {11868, 
  4350, 4350, 550, 1550, 550, 550, 500, 1600, 550, 1600, 
  550, 500, 550, 750, 300, 1600, 550, 500, 550, 550, 
  500, 1650, 500, 550, 500, 550, 550, 1600, 500, 1600, 
  550, 550, 500, 1600, 550, 500, 550, 1600, 550, 1600, 
  500, 1600, 550, 1600, 550, 500, 550, 1600, 550, 1550, 
  550, 1600, 550, 500, 550, 550, 550, 500, 550, 550, 
  550, 1550, 550, 550, 550, 500, 550, 1600, 550, 1550, 
  550, 1600, 550, 500, 550, 550, 500, 550, 550, 500, 
  550, 550, 500, 550, 550, 500, 550, 550, 550, 1550, 
  550, 1600, 550, 1550, 550, 1600, 550, 1600, 500};

int pos = 0;    // variable to store the servo position
int off;
boolean sound = false;
int buttonState;
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

double ms_sound = 0;
double ms_servo = 0;
boolean change = true;
int count = 0;

void setup() {
  Serial.begin(115200);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object

  pinMode(7, OUTPUT); //led verde
  digitalWrite(7, LOW);

  pinMode(8, OUTPUT); //led rojo
  digitalWrite(8, HIGH);
  
  pinMode(6, OUTPUT); //ruido
  digitalWrite(6, LOW);
  off=true;

  pos = 40;
  myservo.write(pos); //posicion inicial

  count=40;

  sound = false;

  irrecv.enableIRIn();
  Serial.println("Receiving...");

}

void loop() {

  // MOVIMIENTO DE SERVO
  if (((millis() - ms_servo) >= 90) && (off==false)){
    if ((count >= 0) && (change==true)){
      myservo.write(pos);
      pos--;
      count--;

      if (count <= 0){
        change = false;
      }

    }else{
      if ((change==false) && (count <= 41) ){
        myservo.write(pos);
        pos++;
        count++;
      
        if (count >= 40){
          change = true;
        }
      }
    }  
    ms_servo = millis();
  }
  
  Serial.println(millis() - ms_sound);

  // CONTROLA EL SONIDO CUANDO SE PRENDE O SE APAGA
  if ((sound == true) && ((millis() - ms_sound) >= 150)){
    Serial.println(".......................................");
    sound = false;
    digitalWrite(6, LOW);
    ms_sound = millis();
  }


  if (irrecv.decode(&results)) {
    //serialPrintUint64(results.value, 16);
    Serial.println("\n Received.\n");
    //bool signal = storeCode(&results);
    if(compareCode(rawOn, &results)){
      // PRENDER AIRE
      Serial.println("TURN ON");
      if (off == true){
        digitalWrite(8, LOW); // apago led rojo
        digitalWrite(7, HIGH); // prendo led verde
        digitalWrite(6, HIGH); //ruido
        sound = true;
        off=false;
        ms_sound = millis();  
      }      

    } else if(compareCode(rawOff, &results)){
      // APAGAR AIRE
      Serial.println("TURN OFF");
      if (off == false){
        digitalWrite(8, HIGH); // prendo led rojo
        digitalWrite(7, LOW); // apago led verde
        pos = 40;
        myservo.write(pos); // vuelve a la pos inicial
        sound = true;
        digitalWrite(6, HIGH); // ruido
        off=true;
        count = 40;
        change = true;
        ms_sound = millis();  
      }     

    } else {
      Serial.println("UNKNOWN");
    }
    irrecv.resume();  // Receive the next value
  }


  //buttonState = digitalRead(5);
  //myservo.write(40);

  
  /*while (verificarBoton() || (off==false)){
    off=false;
    digitalWrite(7, HIGH); //PRENDO LED VERDE
    
    if (sound){  
      digitalWrite(6, HIGH); //APAGO EL SONIDO
      delay(150);
      digitalWrite(6, LOW); //APAGO EL SONIDO  
      sound=false;
    }
    
    for (pos = 40; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(90);                       // waits 15ms for the servo to reach the position
      if (verificarBoton()){
        off=true;
        break;
      }
    }
    delay(500);
    
    if(off==false){
      for (pos = 0; pos <= 40; pos += 1) { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(90);                       // waits 15ms for the servo to reach the position
        if (verificarBoton()){
          off=true;
          break;
        }
      }  
    }
    
    if ((verificarBoton()==true) || (off==true)){
      off=true;
      digitalWrite(6, HIGH); //APAGO EL SONIDO
      delay(150);
      digitalWrite(6, LOW); //APAGO EL SONIDO  
    }else{
      off=false;
    }
    
    delay(500);
  }
  
  digitalWrite(7, LOW);
  digitalWrite(8, HIGH);
  off=true;
  sound=true; */
 
}

boolean verificarBoton(){
  //int buttonState = digitalRead(5);
  if (buttonState==HIGH){
    return true;
  }else
    return false;
}

void turnOn(){
  digitalWrite(7, HIGH); //PRENDO LED VERDE
}

void turnOff(){
  digitalWrite(7, LOW);  // LUZ ROJA
  digitalWrite(8, HIGH);
}

bool compareCode(unsigned int* original, decode_results *results){
  bool eq = true;
  for(int i = 0; i < 100; i++){
    if(!((results->rawbuf[i]*USECPERTICK >= (original[i]-300)) && (results->rawbuf[i]*USECPERTICK <= (original[i]+300))) && (i > 0)){
      eq = false;
      /*Serial.println("El valor ");*/
      Serial.println();
      Serial.print(i);
      Serial.print(" - ");
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
      Serial.print(" - ");
      Serial.print(original[i]);
    }
  }
  Serial.println();
  Serial.println(results->rawlen);
  for(int i = 0; i < results->rawlen; i++){
    Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    Serial.print(", ");
    if(i % 10 == 0) Serial.println();
  }
  Serial.println();
  return eq;
}

/*bool storeCode(decode_results *results) {
  Serial.println("RAW: ");
  Serial.println(results->rawlen);
  bool eq = true;
  for(int i = 0; i < results->rawlen; i++){
    Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    Serial.print(", ");
    if(i % 10 == 0) Serial.println();
  }
  codeType = results->decode_type;
  int count = results->rawlen;
  if (codeType == UNKNOWN) {
    Serial.println("Received unknown code, saving as raw");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) {
      if (i % 2) {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
        Serial.print(" m");
      } 
      else {
        // Space
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
        Serial.print(" s");
      }
      Serial.print(rawCodes[i - 1], DEC);
    }
    Serial.println("");
  }
  else {
    if (codeType == NEC) {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        return;
      }
    } 
    else if (codeType == SONY) {
      Serial.print("Received SONY: ");
    } 
    else if (codeType == PANASONIC) {
      Serial.print("Received PANASONIC: ");
    }
    else if (codeType == JVC) {
      Serial.print("Received JVC: ");
    }
    else if (codeType == RC5) {
      Serial.print("Received RC5: ");
    } 
    else if (codeType == RC6) {
      Serial.print("Received RC6: ");
    } 
    else {
      Serial.print("Unexpected codeType ");
      Serial.print(codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX);
    codeValue = results->value;
    codeLen = results->bits;
  }
  return eq;
}*/