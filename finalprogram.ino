#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27,16,2);
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
const char* ssid = "ESP32";  // Enter SSID here
const char* password = "12345678";  //Enter Password here
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Electronic Voting Machine</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        .middle {
            background-image: url(election-nepal.webp);
            height: 80vh;
            background-size: cover;
            padding: 10px;

        }

        .title {
            border: 0.5px solid #dd2255;
            /* border-radius: 45px; */
            text-align: center;
            font-size: 4rem;
            width: 900px;
            box-shadow: 2px 4px 0px rgb(218, 218, 125);
            margin: 0 auto;
            margin-top: 10px;
            transition: ease-in-out 0.2s;
            margin-bottom: 20px;
        }

        .title:hover {
            background-color: rgb(193, 162, 162);
        }

        ul {
            list-style: none;
            display: flex;
            justify-content: center;
            gap: 50px;
            font-size: 50px;
        }

        button {
            font: inherit;
            background-color: inherit;
            border: 0.5px solid #dd2255;
            text-align: center;
            font-size: 4rem;
            width: 200px;
            box-shadow: 2px 4px 0px rgb(218, 218, 125);
            margin: 0 auto;
            transition: ease-in-out 0.2s;
            cursor: pointer;
        }

        button:active {
            background-color: rgb(187, 218, 244);
        }

        .candidates {
            display: flex;
            justify-content: space-evenly;
            color: white;
            margin-top: 50px;
            backdrop-filter: blur(2px)
        }

        .candidates>* {
            width: 200px;
            height: 200px;
            display: flex;
            border: 2px solid black;
            align-items: center;
            justify-content: center;
            background-color: rgb(230, 129, 129);
            transition: .4s ease-in;
            box-shadow: 2px 4px 0px rgb(218, 218, 125);
        }
        .candidates>*:hover{
            background-color: rgb(203, 196, 187);
        }
    </style>
</head>

<body>
    <div class="title">Electronic Voting Machine</div>
    <div class="middle">
        <nav>
            <ul>
                <li> <a href="/results"><button id="enroll">Results</button></a></button>
                </li>
                <li><a href="/enroll"><button id="enroll">Enroll</button></a></li>
            </ul>
        </nav>
        <div class="candidates">
            <div id="one">Candidate One:<span> %C1% votes</span></div>

            <div id="two">Candidate Two: <span>%C2% votes</span></div>
            <div id="three">Candidate Three:<span>%C3% votes</span></div>
        </div>

    </div>
    <div class="footer">
        <p style="text-align: center; font-size: 2rem;">&copy Developed by BEI078</p>
    </div>
</body>

</html>
)rawliteral";


#define RX_PIN 16  // RS307S TX -> ESP32 RX (GPIO16)
#define TX_PIN 17  // RS307S RX -> ESP32 TX (GPIO17)
#define BUTTON_PIN1 18
#define BUTTON_PIN2 19
#define BUTTON_PIN3 15
#define EXIT_BUTTON_PIN 5 // Define a pin for the exit button
#define Buzzer 4
int votedArr[10]={0};
int votedArrPointer=0;
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {32, 33, 25, 13}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {27, 14, 12, 26}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
int candidateArray[3] = {0, 0, 0}; 

HardwareSerial mySerial(2);  // UART2 on ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
void setup() {
    Serial.begin(115200);
    mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
    finger.begin(57600);
    lcd.init();
    //lcd.backlighto();
    lcd.backlight();
    if (finger.verifyPassword()) {
        Serial.println("FingerPrint sensor Found!");
    } else {
        Serial.println("Fingerprint sensor NOT found!");
        while (1);  // Halt execution if the sensor is not detected
    }
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
  server.on("/", HTTP_GET, root);
  server.on("/results", HTTP_GET, handleResults);

    server.begin();
    pinMode(Buzzer, OUTPUT);
    pinMode(BUTTON_PIN1, INPUT_PULLUP);
    pinMode(BUTTON_PIN2, INPUT_PULLUP);
    pinMode(BUTTON_PIN3, INPUT_PULLUP);
    pinMode(EXIT_BUTTON_PIN, INPUT_PULLUP); // Set up the exit button pin
}

void printMenu(){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("1: Enroll");
    lcd.setCursor(0,1);
    lcd.print("2: Vote");
    Serial.println("Press 1 to Enroll or 2 to Vote");
}
void root(AsyncWebServerRequest *request){
    request->send(200, "text/html", processHTML());
}


String processHTML() {
    String html = index_html;
    html.replace("%C1%", String(candidateArray[0]));
    html.replace("%C2%", String(candidateArray[1]));
    html.replace("%C3%", String(candidateArray[2]));
    return html;
}

void handleResults(AsyncWebServerRequest *request){
      request->send(200, "text/html", processHTML());

}


void loop() {
    printMenu();
    char key = keypad.getKey();
    while(key == NO_KEY && (digitalRead(EXIT_BUTTON_PIN) == HIGH) ){ //input ko lagi wait gardai from keypad
      key = keypad.getKey();
    }
    Serial.println(key);
    if (!digitalRead(EXIT_BUTTON_PIN)){
        checkForAdmin();
        return;
    }
    if(key == '1'){
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Enroll Mode");
                Serial.println("Entering Enroll Mode");
                delay(1000);
                enrollFinger();
    }
    else if(key == '2'){
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Vote Mode");
                Serial.println("Entering Vote Mode");
                delay(2000);
                startVoting();
    }
    else{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Invalid Input");
    }
}

void enrollFinger(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter your ID: ");
    char key = keypad.getKey();
    while(key == NO_KEY){
      key = keypad.getKey();
    }
   // Get the key pressed from keypad
    lcd.setCursor(0,1);
      lcd.print(key);
      delay(1000);
   // Display the pressed key on the LCD
    int id = key - '0'; // Convert char to int

    if (id >= 1 && id <= 120) {
      Serial.print("Enrolling ID: ");
      Serial.println(id);
      enrollFingerprint(id);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Invalid ID!");
}
}

void enrollFingerprint(int id) {
    int p = -1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Place your finger");
    lcd.setCursor(0, 1);
    lcd.print("on the sensor");
    Serial.println("Place your finger on the sensor...");

    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) {
            Serial.print(".");
        } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
            Serial.println(" Communication error!");
            return;
        } else if (p == FINGERPRINT_IMAGEFAIL) {
            Serial.println(" Image capture failed.");
            return;
        }
        delay(500);
    }

    Serial.println("Finger detected! Processing...");
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
        Serial.println("Fingerprint conversion failed!");
        return;
    }
     lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Remove your finger");
  

    Serial.println("Remove your finger...");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Place your same");
    lcd.setCursor(0, 1);
    lcd.print("finger on the sensor");
   
    Serial.println("Place the same finger again...");
    p = -1;
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) {
            Serial.print(".");
        } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
            Serial.println("Communication error!");
            return;
        } else if (p == FINGERPRINT_IMAGEFAIL) {
            Serial.println("Image capture failed.");
            return;
        }
         
        delay(500);
    }

    Serial.println("Second scan successful! Processing...");
    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
        Serial.println("Second fingerprint conversion failed!");
        return;
    }

    Serial.println("Merging fingerprints...");
    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
        Serial.println("Fingerprint merge failed!");
        return;
    }

    Serial.print("Storing fingerprint at ID ");
    Serial.println(id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
        Serial.println("Fingerprint enrolled successfully!");
        lcd.home();
        lcd.clear();
        lcd.print("Enrolled");
        lcd.setCursor(0, 1);
        lcd.print("Successfully");
        delay(2000);
    } else {
        Serial.println("Failed to store fingerprint!");
    }
}

void startVoting() {
    lcd.setCursor(0,0);
    char key = keypad.getKey();
    while(key == NO_KEY){
      key = keypad.getKey();
    }
    int userInputId = key - '0'; //convert char from keypad into integer;
    if(doubleVoting(userInputId)){
      lcd.clear();
      lcd.home();
      lcd.print("Already Voted");
      delay(2000);
      return;
    }

      lcd.setCursor(0,1);
      lcd.print(key);
      Serial.println(key);
      delay(1000);
      digitalWrite(Buzzer, HIGH);
      delay(1000);
      digitalWrite(Buzzer, LOW);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Place your finger");
      lcd.setCursor(0,1);
      lcd.print("Waiting");
      Serial.println("Waiting for a fingerprint...");
      int p = -1;

      while (p != FINGERPRINT_OK) {
          p = finger.getImage();
          if (p == FINGERPRINT_NOFINGER) {
              Serial.print(".");
          } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
              Serial.println("Communication error!");
              return;
          } else if (p == FINGERPRINT_IMAGEFAIL) {
              Serial.println("Image capture failed.");
              return;
          }
          delay(500);
      }
  
      Serial.println("Finger detected! Processing...");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Fingerprint detected");
      delay(2000);
      p = finger.image2Tz(1);
      if (p != FINGERPRINT_OK) {
          Serial.println("Fingerprint conversion failed!");
          return;
      }

      Serial.println("Searching for a match...");
      lcd.setCursor(0,1);
      lcd.print("Searching");
      p = finger.fingerFastSearch();
      if (p == FINGERPRINT_OK) {
          Serial.print("Match found! Fingerprint ID: ");
          Serial.println(finger.fingerID);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Match Found");
          if(finger.fingerID != userInputId ){
            lcd.clear();
            lcd.home();
            lcd.print("Your ID doesn't match with fingerprint");
            delay(2000);
            return;
          }
          if(finger.fingerID == 1){
            lcd.clear();
            lcd.home();

            lcd.print("Admin Verified");
            delay(2000);
            displayVoteCount();
            return;
          }
          Serial.print(finger.fingerID);
          Serial.print(" | Confidence: ");
          Serial.println(finger.confidence);
          lcd.clear();
          lcd.setCursor(0,0);
          
          lcd.print("Please vote");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("c1");
          lcd.setCursor(5,0);
          lcd.print("c2");
          lcd.setCursor(10,0);
          lcd.print("c3");
          push_button();
          votedArr[votedArrPointer] = userInputId;
          votedArrPointer++;
         
      } else if (p == FINGERPRINT_NOTFOUND) {
          Serial.println("No match found in the database.");
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("No match found");
          delay(2000);
      } else {
          Serial.println("Unknown error!");
      }
      
      delay(2000); 
}

void push_button() {
    int noti = 0;
    int buttonState1 = 0;
    int buttonState2 = 0;
    int buttonState3 = 0;
    //wait for button being pressed
    while (noti == 0) {
        buttonState1 = !digitalRead(BUTTON_PIN1);
        buttonState2 = !digitalRead(BUTTON_PIN2);
        buttonState3 = !digitalRead(BUTTON_PIN3);
        int sum  = buttonState1 + buttonState2 + buttonState3; //to check for multiple buttons
        if ((buttonState1 || buttonState2 || buttonState3) && (sum==1)) {
            noti = 1;
        }
    }

    if (buttonState1) {
        candidateArray[0]++;
    } else if (buttonState2) {
        candidateArray[1]++;
    } else if (buttonState3) {
        candidateArray[2]++;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Vote Recorded");
    Serial.println("Vote Recorded");
    delay(2000);
}

void checkForAdmin(){
  lcd.clear();
  lcd.home();
  lcd.print("Admin Mode");//convert from char to int
  startVoting();
  return;
}

void displayVoteCount(){
  lcd.clear();
  lcd.home();
  for(int i=0; i<3; i++){
    lcd.print(candidateArray[i]);
  }
  delay(5000);
  return;
}

bool doubleVoting(int userId){
  //search array
  for(int i=0; i<10; i++){
    if(votedArr[i] == userId) return true;
}
 return false;
}

