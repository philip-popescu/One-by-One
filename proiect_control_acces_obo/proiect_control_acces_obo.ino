#include "functii_operationale.h"
#include "macro.h"

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10,48,152,221);

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  kg = 10.0/4.9;
  setup_IO();
  server_setup(mac,ip);
}


void loop() {
  
  ether_print(NULL);
  if(millis()%CHECK_MSG_FRQ < MIN_TIME_ERR){
    strcpy(msg, "La cantar am: ");
    add_to_string(msg,(int)(analogRead(cantar)/kg));
    strcat(msg,"kg");
    ether_print(msg);
    delay(2);
  }else{
    ether_print(NULL);
  }
  
  unsigned long maxL = 4294967295 ;
  if(millis() > maxL - 50000)
    delay(50000);
  
  if(check_doors()){
    if(analogRead(cantar) > MIN_WEIGHT - TAR_MINUS && analogRead(cantar) < MIN_WEIGHT + TAR_PLUS){
      digitalWrite(ERR_cantar,HIGH);
      delay(1);
      if(digitalRead(req1) == LOW){
        strcpy(msg,"Incep ciclu intrare!");
        ether_print(msg);
        ciclu(do1,do2,ds1,ds2,ciclu_in);
      }
      delay(1);
      if(digitalRead(req2) == LOW){
        strcpy(msg,"Incep ciclu iesire!");
        ether_print(msg);
        ciclu(do2,do1,ds2,ds1,ciclu_out);
      }
    }else{
      digitalWrite(ERR_cantar,LOW);
      strcpy(msg,"WARNING! CANTAR DECALIBRAT CE INDICA: ");
      add_to_string(msg, (int)analogRead(cantar)/kg);
      strcat(msg,"kg");
      ether_print(msg);
      delay(200);
    }
  }else{
    if(analogRead(cantar) > MIN_WEIGHT - TAR_MINUS && analogRead(cantar) < MIN_WEIGHT + TAR_PLUS){
      digitalWrite(ERR_cantar,HIGH);
    }else{
      digitalWrite(ERR_cantar,LOW);
      strcpy(msg,"WARNING! CANTAR DECALIBRAT CE INDICA: ");
      add_to_string(msg, (int)analogRead(cantar)/kg);
      strcat(msg,"kg");
      ether_print(msg);
      delay(200);
    }
  }
}
