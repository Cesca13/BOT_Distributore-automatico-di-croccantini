# BOT_Distributore-automatico-di-croccantini
## Introduzione e finalità del progetto
Il progetto che ho realizzato è un distributore automatico di croccantini il cui controllo è possibile tramite bot di Telegram. La scelta di usare un bot è data sicuramente dalla comodità e facilità d’uso; le impostazioni presenti consentono di padroneggiare al meglio il dispenser a secondo delle proprie esigenze. Inoltre, l’utilizzo di una Cam permette una maggiore sicurezza delle funzionalità proposte, in particolare sull’erogazione.
## Materiale utilizzato
-	Arduino UNO R3;
-	ESP32-Cam;
-	Micro servo motore SG90;
-	Pulsante;
-	LED;
-	Resistenza x2;
-	Breadboard 830 pin MB102;
-	Cavi Jumper DuPont Maschio-Maschio;
-	Cavi Jumper DuPont Maschio-Femmina;
-	Cavo maschio-maschio da USB A a USB B;
-	Cavo USB da maschio USB Type-C a USB-A.
## Telegram
Telegram è un servizio di messaggistica istantanea. Si può installare facilmente sul smartphone (Android e iPhone) o computer (PC, Mac e Linux). È gratuito e senza pubblicità. Telegram consente di creare bot con cui interagire.
_“I bot sono applicazioni di terze parti che vengono eseguite all'interno di Telegram. Gli utenti possono interagire con i bot inviando loro messaggi, comandi e richieste in linea. Tu controlli i tuoi bot usando le richieste HTTPS all'API Bot di Telegram”._
ESP32-CAM interagisce con il bot di Telegram per ricevere, gestire i messaggi e inviare risposte.
### BotFather
Per creare un Bot di Telegram bisogna cercare e cliccare "BotFather”. Digitare "/newbot" e seguire le istruzioni: assegnare un nome e un nome utente per poi avere l’accesso a un token. 
### IDBot
Per essere sicuri di ignorare i messaggi che non provengono dal nostro account Telegram (o da qualsiasi utente non autorizzato), bisogna ottenere il proprio ID utente. IDBot ti permette di scoprirlo.
Di conseguenza, quando il bot del dispencer riceve un messaggio, l'ESP verifica se l'ID del mittente corrisponde all’ID del proprietario, così da gestire il messaggio o ignorarlo.
## IDE di Arduino
Per installare la scheda ESP32 nell’IDE Arduino, ho seguito queste istruzioni:
1.	Nell’IDE Arduino, andare su File > Preferenze;
2.	Inserire quanto segue nel campo "Ulteriori URL di Board Manager": https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json;
3.	Aprire il Gestore delle bacheche. Andare su Strumenti > Bacheca > Gestore bacheche
4.	Cercare ESP32 e premiere il pulsante di installazione per "ESP32 di Espressif Systems".
### Libreria bot universale di Telegram
Per interagire con il bot di Telegram, ho utilizzato la “Universal Arduino Telegram Bot”  che fornisce un'interfaccia semplice per l'API di Telegram Bot. Ho seguito i passaggi a seguire per installazione:
1.	Scaricare la libreria in questione;
2.	Andare su Sketch > Includi libreria > Aggiungi libreria ZIP;
3.	Aggiungi la libreria che hai appena scaricato.
### Libreria ArduinoJson
Inoltre, è necessario installare anche la libreria “ArduinoJson” seguendo i pochi passaggi:
1.  Andare su Sketch > Includi libreria > Gestisci librerie;
2.	Cerca "ArduinoJson";
3.	Installa la libreria.
## Comandi
### Avvio e /start
![image](https://user-images.githubusercontent.com/68649843/208135399-cc399470-2744-4429-9ea0-b0576ba0169f.png)
### /scatta_foto e /scatta_foto_con_flash   
![image](https://user-images.githubusercontent.com/68649843/208135890-feef8690-b9e7-4cca-8994-1764de1f8cd1.png)
### /eroga_ORA 
![image](https://user-images.githubusercontent.com/68649843/208137023-e36ccb79-3e93-46b7-9a0f-d868446233f2.png)
### /cronologia
![image](https://user-images.githubusercontent.com/68649843/208137062-fcf6ffd6-2645-4560-9b5d-9fadfcead640.png)
### /imposta_orario
![image](https://user-images.githubusercontent.com/68649843/208136171-5622cc9c-8589-4172-ae9f-923ddc96a5eb.png)
### /visualizza_orario_impostato    
![image](https://user-images.githubusercontent.com/68649843/208136202-f89856b5-51ed-4cec-ab3d-2ae3603c6bf6.png)
### /annulla_impostazione
![image](https://user-images.githubusercontent.com/68649843/208136237-66088961-6570-4c18-85d0-51c1957c1e93.png)
## Schema circuito
![image](https://user-images.githubusercontent.com/68649843/208137164-e34e19c5-2e0a-4a66-b0ca-d650c104c741.png)
## Schema elettrico 
![image](https://user-images.githubusercontent.com/68649843/208137207-7b34407f-1b1d-4e49-89c2-c7e543974176.png)




