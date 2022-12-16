#include <SoftwareSerial.h>
#include <Servo.h>

//Comunicazione seriali con Esp32-Cam tramite i pin digitali 2 e 3
SoftwareSerial Serial2(2,3);

//Dichiarazione del servo e della sua posizione iniziale
Servo servo;
int pos = 0;    

//Utilizzate la ricezione delle stringhe sulla seriale
String data;
char c;

const int led = 13;
const int pulsante = 7;
int val = 0;

void setup() {
  //Impostazione di bound rate
  Serial.begin(9600);
  Serial2.begin(9600);
 
  pinMode(led,OUTPUT);
  digitalWrite(led, LOW);
  pinMode(pulsante, INPUT);
  servo.attach(9);

  delay(10);
  Serial.println("ARDUINO connesso");
  delay(2000);  
}

//Funzione attivazione servo
void servo_on(){
  Serial.println("Servo attivato");
  digitalWrite(led, HIGH);
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15 ms for the servo to reach the position
  }
  digitalWrite(led, LOW);
}

void loop() {
  //Controllo di un nuovo messaggio dall'Esp32-Cam
  //Memorizzo la stringa/comando e a seconda di esso, attivo l'erogazione
  while(Serial2.available()>0){
    delay(10);
    c = Serial2.read();
    data += c;
  } 

  val = digitalRead(pulsante);  //Lettura del pulsante  

  if (val == 1){  //Se il valore del pulsante è 1 = se è stato schiacciato 
    Serial.print("Pulsante rilevato");   
    //Attivo il servo                
    servo_on();
    delay(200);
  }  

  if (data.length()>0) {
    Serial.println(data);
    //Con il comando /confermo_eroga_ORA parte l'erogazione
    if (data.startsWith("/confermo_eroga_ORA")) {
      servo_on();
    }         
    data = "";
  }
}
