#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "time.h"
#include "sntp.h"
#include <string.h>
#include <stdlib.h>


//Credenziali per la rete
const char* ssid = "";     
const char* password = "";  

//Bot Token fornito da Botfather
String BOTtoken = "";  

//ID della persona che usa il bot
String CHAT_ID = "";

//Inizializzazione WiFiClientSecure e UniversalTelegramBot
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

//Per la verifica della presenza di nuovi messaggi ogni secondo
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//Var
String cronologia = "Elenco erogazioni effettuati:\n";
int orari [3] = {0,0,0};
bool rst_cronologia = true;
bool sveglia = true;

//Data e ora
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

//Fuso orario Roma
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  

//Definizione del GPIO della videocamera su ESP32 Cam.
// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


//Led Flash
#define ON HIGH
#define OFF LOW
//LED Flash PIN (GPIO 4)
#define FLASH_LED_PIN 4             
//Per l'attivazione del Led Flash quando viene scattata la foto
bool capturePhotoWithFlash = false; 

bool sendPhoto = false; 

//Questa funzione viene utilizzata per ottenere lo stato dell'invio di foto in json
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//Per l'invio di messaggi di feedback quando le foto vengono inviate con successo o meno a Telegram
void FB_MSG_is_photo_send_successfully (bool state) {
  String send_feedback_message = "";
  if(state == false) {
    send_feedback_message += "IMPOSSIBILE INVIARE FOTO!\n";
    send_feedback_message += "- Reset ESP32-CAM\n\n";
      send_feedback_message += "/start : visualizza i comandi";
    bot.sendMessage(CHAT_ID, send_feedback_message, "");
  } else {
    Serial.println("Successfully sent photo.");
    send_feedback_message += "Foto inviata con successo!\n\n";
      send_feedback_message += "/start : visualizza i comandi";
    bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
  }
}

//Per attivare o disattivare il flash
void LEDFlash_State (bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}

// Configurazione camera
void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  //Specifiche elevate per pre-allocare buffer più grandi
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }
  /*
   * UXGA   = 1600 x 1200 pixels
   * SXGA   = 1280 x 1024 pixels
   * XGA    = 1024 x 768  pixels
   * SVGA   = 800 x 600   pixels
   * VGA    = 640 x 480   pixels
   * CIF    = 352 x 288   pixels
   * QVGA   = 320 x 240   pixels
   * HQVGA  = 240 x 160   pixels
   * QQVGA  = 160 x 120   pixels
   */
   //Inizializzazione della fotocamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Inizializzazione della fotocamera non riuscita con errore 0x%x", err);
    Serial.println();
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
  }
   
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA);  //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  
}

//Restituisce SOLO l'ora 
int hourLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return 0;
  }
  int hour = timeinfo.tm_hour;

  return hour;
}

//Restiruisce l'orario completo
String localTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "";
  }

  String orario_final = "";

  //Controlla che ogni parametro abbia due unità
  //Es. Fa in modo che le "7:0:0" sia trasformato in --> 07:00:00
  int orario[] = {timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec};
  for (int i=0; i<3; i++){
    if (orario[i] < 10){
      orario_final += "0";    
    }
    orario_final += String(orario[i]); 

    if(i!=2){
        orario_final += ":";
      }
  }

  return orario_final;
}


//Funzione callback (viene chiamata quando l'ora si regola tramite NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Ho ottenuto la regolazione dell'ora da NTP!");
  Serial.println(localTime());
}


//Gestione post arrivo di un messaggio
void handleNewMessages(int numNewMessages) {
  //Controllo del contenuto del messaggio appena arrivato
  for (int i = 0; i < numNewMessages; i++) {
    //Verifica ID
    //Se il chat_id è diverso dall' ID impostato (CHAT_ID), significa che qualcuno non autorizzato ha inviato un messaggio al tuo bot.
    //In tal caso, il messaggio viene ignorato
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Utente non autorizzato", "");
      Serial.println("Utente non autorizzato");
      Serial.println("------------");
      continue;
    }
    
    //Stampa il messaggio ricevuto
    String text = bot.messages[i].text;
    Serial.println(text);

    //Controlla le condizioni in base ai comandi inviati dal bot
    String send_feedback_message = "";
    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      send_feedback_message += "Benvenvenuta, " + from_name + "\n";
      send_feedback_message += "Scegli le opzioni da attivare:\n\n";
      send_feedback_message += "/scatta_foto : invio della foto del dispenser\n";
      send_feedback_message += "/scatta_foto_con_flash : invio della foto del dispenser con LED Flash\n\n";
      send_feedback_message += "/eroga_ORA : erogazione istantanea dei croccantini\n";
      send_feedback_message += "/cronologia : tiene traccia delle erogazioni effettuate\n\n";
      send_feedback_message += "/imposta_orario : programma l'erogazione fino a un massimo di 3 orari giornalieri\n";
      send_feedback_message += "/visualizza_orario_impostato : visualizza gli orari già programmati\n";
      send_feedback_message += "/annulla_impostazione : cancella tutta la programmazione\n\n\n";
      
      bot.sendMessage(CHAT_ID, send_feedback_message, "");

    }

    if (text == "/scatta_foto") {
      bot.sendMessage(CHAT_ID, "Attendi pochi secondi...");
      sendPhoto = true;
    }
    
    if (text == "/scatta_foto_con_flash") {
      bot.sendMessage(CHAT_ID, "Attendi pochi secondi...");
      capturePhotoWithFlash = true;
      sendPhoto = true;
    }

    if (text == "/cronologia") {
      bot.sendMessage(CHAT_ID, cronologia +"\n\n/start : torna al menù principale");
    }
    
    if (text == "/eroga_ORA") {      
      send_feedback_message += "Confermi l'erogazione instantanea?\n";
      send_feedback_message += "/confermo_eroga_ORA\n\n\n";      
      send_feedback_message += "/start : visualizza i comandi";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    if (text == "/confermo_eroga_ORA"){
      bot.sendMessage(CHAT_ID, "Attendere qualche secondo . . .");
      cronologia += ("Alle ");
      cronologia += localTime();
      cronologia += "\n";

      delay(7000);
      send_feedback_message += "Erogazione istantanea effettuata!";
      send_feedback_message += "\n\n/start : torna al menù principale";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
      Serial.println("Erogazione istantanea effettuata");

    }

    if (text == "/visualizza_orario_impostato"){
      int zeri = 0;
       send_feedback_message += "Erogazione programmata:\n";
      for(int i=0; i<3; i++){
        if(orari[i]!=0){
          send_feedback_message += ("Alle ");
          send_feedback_message += orari[i];
          send_feedback_message += (":00\n"); 
        }
        else zeri++;        
      }
      if(zeri==3){
         send_feedback_message += "\n\n\nNon hai impostato nessun orario!";
      }
      
      send_feedback_message += "\n\n/start : torna al menù principale";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");  
    }

    if (text == "/annulla_impostazione") {      
      send_feedback_message += "Sei sicuro di voler eliminare gli orari impostati?\n";
      send_feedback_message += "/confermo_annullamento\n\n\n";      
      send_feedback_message += "/start : visualizza i comandi";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
    

    if (text == "/confermo_annullamento"){
      for(int i=0; i<3; i++){
        orari[i]=0;
      }
      send_feedback_message += "La programmazione è stata annullata!";
      send_feedback_message += "\n\n/start : torna al menù principale";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
      
    }

    if (text == "/imposta_orario"){
      String keyboardJson;
      keyboardJson += "[[{ \"text\" : \"07:00\", \"callback_data\" : \"07\" },{ \"text\" : \"08:00\", \"callback_data\" : \"08\" },";
      keyboardJson += "{ \"text\" : \"09:00\", \"callback_data\" : \"09\" },{ \"text\" : \"10:00\", \"callback_data\" : \"10\" }]";
      keyboardJson += ",[{ \"text\" : \"12:00\", \"callback_data\" : \"12\" },{ \"text\" : \"13:00\", \"callback_data\" : \"13\" },";
      keyboardJson += "{ \"text\" : \"14:00\", \"callback_data\" : \"14\" },{ \"text\" : \"15:00\", \"callback_data\" : \"15\" }]";
      keyboardJson += ",[{ \"text\" : \"16:00\", \"callback_data\" : \"16\" },{ \"text\" : \"17:00\", \"callback_data\" : \"17\" },";
      keyboardJson += "{ \"text\" : \"18:00\", \"callback_data\" : \"18\" },{ \"text\" : \"19:00\", \"callback_data\" : \"19\" }]";
      keyboardJson += ",[{ \"text\" : \"20:00\", \"callback_data\" : \"20\" },{ \"text\" : \"21:00\", \"callback_data\" : \"21\" },";
      keyboardJson += "{ \"text\" : \"22:00\", \"callback_data\" : \"22\" },{ \"text\" : \"23:00\", \"callback_data\" : \"23\" }]]";
      send_feedback_message += "\n\n\n/start : torna al menù principale";
      bot.sendMessageWithInlineKeyboard(CHAT_ID, "Scegli l'orario di attivazione:", "", keyboardJson);
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    if (bot.messages[i].type == "callback_query"){
      String s = bot.messages[i].text; 
      int ora = s.toInt();
      int zeri = 3;

      //A seguire, ci saranno una serie di controlli 
      //Conta gli zeri per capire quante impostazioni sono possibili
      for(int i=0; i<3; i++){     
        if(orari[i]!=0){
          zeri--;
        }
      }
      //Significa che sono già stati impostati 3 orari
      if (zeri == 0){
        send_feedback_message += "Non puoi impostare un orario, poichè ne hai programmati già 3!";
      }
      else{
        //Ricordare che la programmazione è giornaliera 
        //per cui non è possibile selezionare un orario nell'arco della giornata che sia già trascorso
        if (ora < hourLocalTime()){ 
          send_feedback_message += "Impossibile! L'orario selezionato è già trascorso, scegliere un altro orario";
        }
        else{
          for(int i=0; i<3; i++){     
            if(orari[i]==0 && hourLocalTime()){
            orari[i] = ora;
            send_feedback_message += "Orario impostato per le "+s+":00";
            break;
            }         
          }
        }
      }

      //Stampa molto utile per verificare gli elementi dell'array
      Serial.print("\nArray: ");
      for(int i=0; i<3; i++){
        Serial.print(orari[i]);
        Serial.print(" ");
      }
      Serial.println();

      send_feedback_message += "\n\n/start : torna al menù principale";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");

    }

        
  }
  
}


//Processo di scatto e invio di foto
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  //Accende il LED Flash se il comando ricevuto è "/scatta_foto_con_Flash"
  if(capturePhotoWithFlash == true) {
    LEDFlash_State(ON);
  }
  delay(1000);
  
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Acquisizione della fotocamera non riuscita");
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  //Spegne il led post scatto
  if(capturePhotoWithFlash == true) {
    LEDFlash_State(OFF);
    capturePhotoWithFlash = false;
  }
  
  //The process of sending photos
  if (clientTCP.connect(myDomain, 443)) {
    String head = "--Esp32Cam\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n";
    head += CHAT_ID; 
    head += "\r\n--Esp32Cam\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Esp32Cam--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32Cam");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    //timeout 10 secondi (per l'invio della foto)
    int waitTime = 10000;   
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);

    //La condizione per verificare se la foto è stata inviata correttamente o meno
    // Se l'invio della foto ha esito positivo o negativo, verrà inviato un messaggio di feedback a Telegram
    if(getBody.length() > 0) {
      String send_status = "";
      send_status = getValue(getBody, ',', 0);
      send_status = send_status.substring(6);
      
      if(send_status == "true") {
        FB_MSG_is_photo_send_successfully(true);  
      }
      if(send_status == "false") {
        FB_MSG_is_photo_send_successfully(false); 
      }
    }
    if(getBody.length() == 0) FB_MSG_is_photo_send_successfully(false); 
    
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connessione a api.telegram.org non riuscita.");
  }
  return getBody;
}

//Subroutine 
void orari_erog(){
  for(int i=0; i<3; i++){
    if(orari[i]!=0 && hourLocalTime() == orari[i]){
      orari[i] = 0;
      //Comando seriale di erogazione ad Arduino
      Serial.println("/confermo_eroga_ORA");
      delay(7000);
      String send_feedback_message;
      send_feedback_message += "L'erogazione programmata per le ";
      send_feedback_message += String (hourLocalTime()), ":00,";
      send_feedback_message += " è stata appena effettuata!";
      send_feedback_message += "\n\n/start : torna al menù principale";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");

      cronologia += ("Alle ");
      cronologia += localTime();
      cronologia += "\n";
              
    }
  }
}  



void setup(){
  //Disabilita il rilevatore di brownout  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  //Inizializzazione velocità di comunicazione seriale (baud rate)
  Serial.begin(9600);
  delay(1000);
  
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);

  configInitCamera();

  //Funzione di notifica del tempo
  sntp_set_time_sync_notification_cb(timeavailable);

  //L'indirizzo del server NTP può essere acquisito tramite DHCP
  sntp_servermode_dhcp(1); 

  //Questo imposterà i server ntp configurati e TimeZone/daylightOffset costante
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  //Connessione al WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("Connessione a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); 

  //Il timeout del processo di connessione della CAM ESP32 con l'hotspot WiFi / router WiFi è di 20 secondi.
  //Se entro 20 secondi la CAM ESP32 non è stata collegata correttamente al WiFi, la CAM ESP32 si riavvierà.
  int connecting_process_timed_out = 20; 
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  LEDFlash_State(OFF);
}



void loop() {
  //Per scattare e inviare foto
  if (sendPhoto) {
    sendPhotoTelegram(); 
    sendPhoto = false; 
  } 

  //Condizione per controllare se arrivano nuovi messaggi
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1); 
    }
    lastTimeBotRan = millis();
  }

  delay(500);

  orari_erog();

  delay(500);

  //Resetta "cronologia" a mezzanotte solo una volta (grazie alla var booleana)
  if (hourLocalTime()==0 && rst_cronologia){
    cronologia = "Elenco erogazioni effettuate:\n";
    rst_cronologia = false;
  }
  else if (hourLocalTime()>0){
    rst_cronologia = true;
  }
  
  delay(500);

  //Messaggio delle 07:00 che ricorda giornalmente di impostare i nuovi orari
  if(hourLocalTime()==7 && sveglia){
    String message;
    message += "Ciao,\n";
    message += "E' una nuova giornata! Ti va di programmare i nuovi orari?\n";
    message += "\n\n Clicca su /start";
    bot.sendMessage(CHAT_ID, message,"");
    sveglia = false;
  }
  else if (hourLocalTime()>7){
    sveglia = true;
  }

}
