#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27,16,2);

#define RX_PIN 16  // RS307S TX -> ESP32 RX (GPIO16)
#define TX_PIN 17  // RS307S RX -> ESP32 TX (GPIO17)
#define BUTTON_PIN1 18
#define BUTTON_PIN2 19
#define BUTTON_PIN3 0
#define BUTTON_PIN4 15
#define EXIT_BUTTON_PIN 5 // Define a pin for the exit button
#define Buzzer 4

const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {25, 33, 32, 13}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {12, 14, 27, 26}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
int candidateArray[3] = {0, 0, 0}; 

HardwareSerial mySerial(2);  // UART2 on ESP32
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
void setup() {
    Serial.begin(115200);
    mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
    finger.begin(57600);
    lcd.init();
    lcd.backlight();

    if (finger.verifyPassword()) {
        Serial.println("FingerPrint sensor Found!");
    } else {
        Serial.println("Fingerprint sensor NOT found!");
        while (1);  // Halt execution if the sensor is not detected
    }

    pinMode(Buzzer, OUTPUT);
    pinMode(BUTTON_PIN1, INPUT_PULLUP);
    pinMode(BUTTON_PIN2, INPUT_PULLUP);
    pinMode(BUTTON_PIN3, INPUT_PULLUP);
    pinMode(BUTTON_PIN4, INPUT_PULLUP);
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

void loop() {
    void printMenu();
    char key = keypad.getKey();
    while(key == NO_KEY && digitalRead(EXIT_BUTTON_PIN) ){ //input ko lagi wait gardai from keypad
      key = keypad.getKey();
    }
    if (!digitalRead(EXIT_BUTTON_PIN)){
        checkForAdmin();
    }
    if(key = '1'){
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Enroll Mode");
                Serial.println("Entering Enroll Mode");
                delay(1000);
                enrollFinger();
    }
    else if(key = '2'){
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
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Match Found");
          if(finger.fingerID != userInputId ){
            lcd.clear();
            lcd.home();
            lcd.print("Your ID doesn't match with fingerprint");
            return;
          }
          if(finger.fingerID == 1){
            lcd.clear();
            lcd.home();
            lcd.print("Admin");
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
     int buttonState4 = 0;
    //wait for button being pressed
    while (noti == 0) {
        buttonState1 = !digitalRead(BUTTON_PIN1);
        buttonState2 = !digitalRead(BUTTON_PIN2);
        buttonState3 = !digitalRead(BUTTON_PIN3);
        buttonState4 = !digitalRead(BUTTON_PIN4);
        int sum  = buttonState1 + buttonState2 + buttonState3 + buttonState4; //to check for multiple buttons
        if ((buttonState1 || buttonState2 || buttonState3) && (sum==1)) {
            noti = 1;
            break;
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
  return;
}


