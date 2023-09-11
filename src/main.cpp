// ****************************************** Notatki ****************************************
/* Trzy obroty: 
  jeden krok - 1.8stopnia,
  360stopni - 200 krokow,
  3 obroty - 600 krokow,
  najmniejsza rozdzielczosc sterowniak - 1/8 kroku
  wiec 600 * 8 = 4800 kroków = 3 pełne obroty
*/

// ****************************************** Biblioteki ****************************************
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Keypad.h> 
#include <SPIFFS.h>
#include <Preferences.h>
#include <BluetoothSerial.h>

Preferences preferencje;
BluetoothSerial SerialBT;
// ****************************************** KLASY I OBIEKTY ****************************************
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ****************************************** ZMIENNE ****************************************
const char* APssid           = "Zamek inteligentny";
String APpassword;
String HasloAsString;
String input_password; 

String message = "";
String paramFormularzu = "haslo";
String StrOdebraneBT;


const int EmergencyON   = 21;

const int GREENpin      = 18;   
const int Buzzer        = 4;
const int Kontaktron    = 5;
const int potencjometr  = 35;
const int dirPin        = 15; 
const int steppPin      = 2;
const int STNDBYpin     = 19; //Jeżeli OUTPUT = HIGH przechodzi w tryb czuwania (VREF i ENN = 0v);


int stanKontaktronu; //PULLUP na pinie: 1 - drzwi otwarte, 0 - drzwi domknięte
int simpleFlag            = 0;
int FlagaWEB              = 0;
int flagaBT               = 0;
int StartCheckFlag        = 0;
int wartoscPotencjometru  = 0;
int poprzedniaWartoscPot  = 0;
bool pozycjaZamka;
bool ALARM;

unsigned long czasTeraz     = 0;
unsigned long czasPoprzedni = 0;
// ****************************************** KEYPAD ****************************************
const byte ROWS = 4;    
const byte COLS = 4;      
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33, 32};
Keypad mojKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 


// ****************************************** FUNKCJE ****************************************



//Odczytuje wartość podawaną przez kontaktron, potencjometr (wartość napięcia), oraz stan zmiennej 'pozycjaZamka'
void OdczytDanych(){
    stanKontaktronu = digitalRead(Kontaktron);
    wartoscPotencjometru = analogRead(potencjometr);
  //Serial.println("Wartosc potencjometru:  " + String(wartoscPotencjometru));
}


//Przypisuje pozycję silnika krokowego na podstawie wartości zczytanej z potencjometru
void okreslenieStanuSilnika(){

  OdczytDanych();
  //odczytanie z pamieci poprzedniej zapamietanej wartosci potencjometru
  Serial.println("");
  poprzedniaWartoscPot = preferencje.getInt("prevPotVal", 0);
  Serial.println("Poprzednia wartosc potencjometru: " + String(poprzedniaWartoscPot));
  Serial.println("Obecna wartosc potencjometru: " + String(wartoscPotencjometru));

  //Dodanie +- 500 dla przypadkowego przemieszczenia zębatki przy otwieraniu manualnym
  if ((wartoscPotencjometru <  poprzedniaWartoscPot) && ((poprzedniaWartoscPot - wartoscPotencjometru) >= 500)) {
    pozycjaZamka = 1; //Tzn. że zamek jest otwarty
    Serial.println("Pozycja zamka: OTWARTE (1)");
    SerialBT.print("ON");
    SerialBT.print("\n");
    
  }
  else if ((wartoscPotencjometru > poprzedniaWartoscPot) && ((wartoscPotencjometru - poprzedniaWartoscPot) >= 500)) {
    pozycjaZamka = 0; //Tzn. że zamek jest zamknięty
    Serial.println("Pozycja zamka: ZAMKNIETE (0)");
    SerialBT.print("OFF");
    SerialBT.print("\n");

  }
}



//Generuje nowe hasło do Keypada
void GenHasla(){

  srand(time(0));
  const char znaki[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D'};
  int ilosc_znakow = sizeof(znaki);
  char Haslo[8];
  
  for (int i=0; i < sizeof(Haslo); i++){
    Haslo[i] = znaki[rand() % ilosc_znakow];
  }
  HasloAsString = String(Haslo, sizeof(Haslo));
}



//Ruch silnika krokowego 
void Otwieranie(){
  //Ruch w kierunku przeciwnym do ruchu wskazówek zegara
    delay(100);
    digitalWrite(STNDBYpin, LOW);
    delay(100);
    digitalWrite(dirPin, LOW);
    Serial.println("Otwieranie zamka!");
    tone(Buzzer, 1000, 3000); 

  for(int x = 0; x < (600*8); x++){
    digitalWrite(GREENpin, HIGH);

    digitalWrite(steppPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(steppPin, LOW);
    delayMicroseconds(500);
  }
  delay(100);
  digitalWrite(STNDBYpin, HIGH);
}

//Ruch silnika krokowego 
void Zamykanie(){
  //Ruch w kierunku zgodnym z kierunkiem ruchu wskazówek zegara
  delay(100);
  digitalWrite(STNDBYpin, LOW);
  delay(100);
  digitalWrite(dirPin, HIGH);
  Serial.println("Zamykanie zamka!");

  for(int x = 0; x < (600*8); x++){  
    digitalWrite(steppPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(steppPin, LOW);
    delayMicroseconds(500);
  }
  delay(100);
  digitalWrite(STNDBYpin, HIGH);
}



//Wysyla informacje o do klientow o stanie zmiennych
void notifyClients() {
 String wysylanaWiadomosc = String(pozycjaZamka) + "," + String(stanKontaktronu) + "," + HasloAsString + "," + String(APpassword) ;
  ws.textAll(wysylanaWiadomosc);

  if (pozycjaZamka == 1){
    //Serial.println("Pozycja zamka: OTWARTE (1)");
    SerialBT.print("ON");
    SerialBT.print("\n");
  } else{
    //Serial.println("Pozycja zamka: ZAMKNIETE (0)");
    SerialBT.print("OFF");
    SerialBT.print("\n");
  }
  delay(200);
}



//Obsługuje dane/wiadomości przychodzące z servera WebSocket (ze strony HTML)
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      //Serial.println(String((char*)data)); //debug
      FlagaWEB= 1;
    } 
    return;
  }
}



//Obsługa zdarzeń servera WebSocket - Zalogowanie/wylogowanie klienta, ERRORy, odebranie wiadomości/danych
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      okreslenieStanuSilnika();
      notifyClients();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}



//Działania przy rozpoczęciu działania servera WebSocket
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}



// ****************************************** SETUP ****************************************
void setup() {
  Serial.begin(115200);
  SerialBT.begin("Zamek inteligentny"); //Interfejs Bluetooth
  preferencje.begin("pamiec", false);
  
  pinMode(GREENpin, OUTPUT);
  pinMode(Buzzer, OUTPUT);  
  pinMode(Kontaktron, INPUT_PULLUP);
  pinMode(dirPin, OUTPUT);
  pinMode(steppPin, OUTPUT);
  pinMode(STNDBYpin, OUTPUT);
  pinMode(EmergencyON, OUTPUT);

  digitalWrite(GREENpin, LOW);
  digitalWrite(STNDBYpin, HIGH);  //Domyślnie w trybie czuwania
  

  delay(1000);
  APpassword = preferencje.getString("haslo", String("12345678"));
  if (APpassword.isEmpty()){
    preferencje.putString("haslo", String("12345678"));
    Serial.println("Ustawiono standardowe haslo: " + APpassword);
  }

  // LOGOWANIE DO STRONY WEB
  Serial.println("");
  WiFi.mode(WIFI_AP);
  Serial.println(" Tworzenie Access Point'u…");
  Serial.println(" SSID: " + String(APssid));
  Serial.println(" Haslo do AP: " + APpassword);
  WiFi.softAP(APssid, APpassword.c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.println(" Adres IP strony WEB: ");
  Serial.println(IP);
  Serial.println(" Hasło do Keypadu: ");
    GenHasla();
  Serial.println(HasloAsString);

  //Rozpoczęcie działania funkcji servera WebSocket

  initWebSocket();

  // Sprawdzenie działania systemu plików SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println(" Problem - System plików SPPIFS nie zostal zainicjowany!");

  } else {
    Serial.println("Pomyslnie zainicjowano system plikow SPIFFS!");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/index.html", String());
});
  
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    String StrOdebraneWEB;
    if (request->hasParam(paramFormularzu)){
      StrOdebraneWEB = request->getParam(paramFormularzu)->value();
      Serial.println("Odebrana waidomosc WEB: " + StrOdebraneWEB);
      preferencje.putString("haslo", StrOdebraneWEB);
      server.reset();
      WiFi.disconnect();
      delay(1000);
      APpassword = preferencje.getString("haslo", String("12345678"));
      WiFi.mode(WIFI_AP);
      WiFi.softAP(APssid, APpassword.c_str());
    } else {
      StrOdebraneWEB = "Wiadomosc nie zostala odebrana";
    }
  });

  // Rozpoczęcie servera Web
  server.begin();
  delay(1000);
  StartCheckFlag = 1;
}

// ****************************************** LOOP() ****************************************
void loop() {

  while(simpleFlag == 0){
  char key = mojKeypad.getKey();
  czasTeraz = millis();


  if (pozycjaZamka == 1){
     digitalWrite(GREENpin, HIGH);  
  } else {
    digitalWrite(GREENpin, LOW);  
  }


  if (czasTeraz - czasPoprzedni > 5000UL){
    OdczytDanych();
    notifyClients();
    czasPoprzedni = czasTeraz;
  }  


  //Bezpiecznik
  while (EmergencyON == 1)
  {
    Otwieranie();
    tone(Buzzer, 3000);
    digitalWrite(GREENpin, HIGH);
  }
  

  if (StartCheckFlag == 1){
    okreslenieStanuSilnika();
    notifyClients();

    StartCheckFlag = 0;
  }

  //Pobieranie danych z bluetooth;
  //znaki inkrementowane w celu otrzymania pełnych słów
  if (SerialBT.available()){
    char incomingChar = SerialBT.read();
    StrOdebraneBT += incomingChar;
    Serial.println(StrOdebraneBT);  
  }


  //Jeżeli wiadomość BT > 0 i porównana zostanie treść
  if (StrOdebraneBT.length() > 0 ){
    if (StrOdebraneBT == "SPRAWDZ"){
      flagaBT = 1;
    }
    else if (StrOdebraneBT == "ON"){
      flagaBT = 2;
    } 
    else if (StrOdebraneBT == "OFF"){
      flagaBT = 3;
    }
  }

  if(flagaBT == 1){

    okreslenieStanuSilnika();
    if (pozycjaZamka = 1){
      SerialBT.print("ON");
      SerialBT.print("\n");
      delay(200);
    } else if (pozycjaZamka = 0){
      SerialBT.print("OFF");
      SerialBT.print("\n");
      delay(200);
    }

    SerialBT.print("#"+HasloAsString);
    SerialBT.print("\n");
    StrOdebraneBT = "";
    flagaBT = 0;

  } else if (flagaBT == 2) {
    Otwieranie();
    SerialBT.print("ON");
    SerialBT.print("\n");
    Serial.println("Drzwi otwarte");
    StrOdebraneBT = "";
    pozycjaZamka = 1;

    //Preferencje:
    //zapis wartosci potencjometru do pamieci
    OdczytDanych();
    Serial.println("");
    preferencje.putInt("prevPotVal", wartoscPotencjometru);
    notifyClients();
     flagaBT = 0;
    delay(4000);
  } else if (flagaBT == 3) {
    Zamykanie();
    GenHasla();
    pozycjaZamka = 0;

    SerialBT.print("OFF");
    SerialBT.print("\n");
    delay(200);
    SerialBT.print("#"+HasloAsString);
    SerialBT.print("\n");
    Serial.println("Drzwi zamkniete");
    StrOdebraneBT = "";

    //Preferencje:
    //zapis wartosci potencjometru do pamieci
    OdczytDanych();
    Serial.println("");
    preferencje.putInt("prevPotVal", wartoscPotencjometru);
    notifyClients();
    flagaBT = 0;
  }


  //Jeżeli wiadomość z servera WEB == "toggle"
  if (FlagaWEB == 1){
     okreslenieStanuSilnika();
     if (pozycjaZamka == 1){
      Zamykanie();
      GenHasla();
      pozycjaZamka = 0;

      SerialBT.print("OFF");
      SerialBT.print("\n");
      delay(200);
      SerialBT.print("#"+HasloAsString);
      SerialBT.print("\n");
      Serial.println("Drzwi zamkniete");
      StrOdebraneBT = "";

     } else if (pozycjaZamka == 0) {
      Otwieranie();
      pozycjaZamka = 1;

      SerialBT.print("ON");
      SerialBT.print("\n");
      delay(200);
      Serial.println("Drzwi otwarte");
      StrOdebraneBT = "";
      delay(4000);
     }

    //Preferencje:
    //zapis wartosci potencjometru do pamieci
    OdczytDanych();
    Serial.println("");
    preferencje.putInt("prevPotVal", wartoscPotencjometru);
    notifyClients();
    FlagaWEB = 0;
    delay(200);
  }

//Automatyczne zamykanie po 4 sekundach, po zamknięciu drzwi
  while (pozycjaZamka == 1 && stanKontaktronu == 0 ){
    stanKontaktronu = digitalRead(Kontaktron);
    delay(4000);
    Zamykanie();
    GenHasla();

    Serial.println("Autozamykanie...");

    SerialBT.print("OFF");
    SerialBT.print("\n");
    delay(200);
    SerialBT.print("#"+HasloAsString);
    SerialBT.print("\n");
    Serial.println("Drzwi zamkniete");
    StrOdebraneBT = "";

    //Preferencje:
    //zapis wartosci potencjometru do pamieci
    OdczytDanych();
    Serial.println("");
    preferencje.putInt("prevPotVal", wartoscPotencjometru);
    notifyClients();
    pozycjaZamka = 0;
    
  }

  //Jeżeli przycisk keypada zostanie naciśnięty
  if (key){
    Serial.println(key);
    tone(Buzzer, 2000, 300); // 1KHz
    if (key == '*') {
      input_password= "";                 // Czyści wpisane znaki jeżeli wciśnięty został znak gwiazdki *
    } else if (key == '#') {              // Jeżeli wciśnięty # to porówanie wpisanego hasła z ustawionym kluczem i odpowiedź
      if (HasloAsString == input_password) {  //Jeżeli prawda to zamek się otwiera 
        okreslenieStanuSilnika();
        tone(Buzzer, 3000, 3000);
        if (pozycjaZamka == 0) {
          Serial.println("\n Prawidłowe hasło - DOSTĘP UZYSKANY!");
          Otwieranie();
          pozycjaZamka = 1;

          SerialBT.print("ON");
          SerialBT.print("\n");
          delay(200);         

          //zapis wartosci potencjometru do pamieci
          OdczytDanych();
          Serial.println("");
          preferencje.putInt("prevPotVal", wartoscPotencjometru);
          notifyClients();
          simpleFlag = 1;
          delay(4000);
        } else if (pozycjaZamka == 1) {                       
          Serial.println("\n DRZWI SA OTWARTE!");
        }
      } else {
       Serial.println("\n Hasło nieprawidłowe - DOSTĘP ZABLOKOWANY!");
        tone(Buzzer, 1000, 3000);
        digitalWrite(GREENpin, LOW);
      }
      input_password = ""; // Wyczyszczenie wpisywanego hasła z pamięci
    } else {
     input_password += key; // Dodanie nowych znaków to zmiennej typu string wpisywanego hasła
    }
  }
  }

  while(simpleFlag == 1){
    //Jeżeli drzwi zostaną zamknięte po ich pierwotnym otworzeniu
    OdczytDanych();

    if(stanKontaktronu == 0){
      delay(4000);
      Zamykanie();
      GenHasla();
      pozycjaZamka = 0;

      SerialBT.print("OFF");
      SerialBT.print("\n");
      delay(200);
      SerialBT.print("#"+HasloAsString);
      SerialBT.print("\n");

      //Preferencje:
      //zapis wartosci potencjometru do pamieci
      OdczytDanych();
      Serial.println("");
      preferencje.putInt("prevPotVal", wartoscPotencjometru);
      notifyClients();
      simpleFlag = 0;
    }
  delay(200);
  }
}