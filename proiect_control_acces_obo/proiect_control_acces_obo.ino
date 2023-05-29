#include "functii_operationale.h"
#include "macro.h"

void setup() {
  Serial.begin(9600);
  kg = 10.0/4.9;
  setup_IO();
  client_setup();
}


void loop() {
  static unsigned long time_from_last_msg = 0;
  static unsigned long time_from_last_error_msg = 0;

  // VERIFICARE CANTAR
  if(analogRead(cantar) > MIN_WEIGHT - TAR_MINUS && analogRead(cantar) < MIN_WEIGHT + TAR_PLUS){
      digitalWrite(ERR_cantar,HIGH);
  }else{
    digitalWrite(ERR_cantar,LOW);
    // EDGE CASE "MESE"
    while((analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || check_doors(0) == 0)){

  Serial.print("Cantar W1: ");
  Serial.println((int)(analogRead(cantar)/kg));

      if(millis() - time_from_last_error_msg  > CHECK_MSG_ERROR_FRQ){

  Serial.print("Cantar: ");
  Serial.println((int)(analogRead(cantar)/kg));

        msg[0] = 2;
        msg[1] = (unsigned char)((int)(analogRead(cantar)/kg));
        send_message(msg,2);
        time_from_last_error_msg = millis();
      }
      digitalWrite(do2, LOW);
      delay(CMD_FOR_LOCK);
      digitalWrite(do2, HIGH);
      delay(W8_TIME_LOCK);
    }
    return;
  }
  
  // VERIFICA CLOCK OVERFLOW EDGE CASE
  unsigned long maxL = 4294967295 ;
  if(millis() > maxL - 50000) delay(50000);
  
  if(check_doors(1)){
    // SENT NORMAL MESSAGE
    if(millis() - time_from_last_msg > CHECK_MSG_FRQ){
      msg[0] = 0;
      msg[1] = (unsigned char)((int)(analogRead(cantar)/kg));
      Serial.print("Cantar: ");
      Serial.println((int)(analogRead(cantar)/kg));
      send_message(msg,2);
      time_from_last_msg = millis();
    }

    // CHECK IF WE HAVE A REQUEST
    if(digitalRead(req1) == LOW){
      unsigned char status = ciclu(do1,do2,ds1,ds2,ciclu_in);
      msg[0] = 9;
      msg[1] = status;
      send_message(msg,2);
    }else if(digitalRead(req2) == LOW){
      unsigned char status = ciclu(do2,do1,ds2,ds1,ciclu_out);
      msg[0] = 9;
      msg[1] = status;
      send_message(msg,2);
    }
    delay(2);
  } else {
    int door = (digitalRead(ds1) == HIGH) ? do1 : do2;
    int door_s = (digitalRead(ds1) == HIGH) ? ds1 : ds2;
    while((analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || !check_doors(1))){

  Serial.print("Cantar W2: ");
  Serial.println((int)(analogRead(cantar)/kg));

      digitalWrite(door, LOW);
      delay(CMD_FOR_LOCK);
      digitalWrite(door, HIGH);
      delay(W8_TIME_LOCK);
    }
  }
}
