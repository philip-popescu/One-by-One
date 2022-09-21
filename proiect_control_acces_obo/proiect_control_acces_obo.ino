#include "functii_operationale.h"
#include "macro.h"

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10,48,152,221);

void setup() {
  Serial.begin(9600);
  kg = 10.0/4.9;
  setup_IO();
  client_setup();
}


void loop() {
  static unsigned long time_from_last_msg

  // VERIFICARE CANTAR
  if(analogRead(cantar) > MIN_WEIGHT - TAR_MINUS && analogRead(cantar) < MIN_WEIGHT + TAR_PLUS){
      digitalWrite(ERR_cantar,HIGH);
  }else{
    digitalWrite(ERR_cantar,LOW);
    // EDGE CASE "MESE"
    while((analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || check_doors(0) == 0)){
      if(time_from_last_msg - millis() > CHECK_MSG_FRQ){
        msg[0] = 3;
        msg[1] = (unsigned char)((int)(analogRead(cantar)/kg));
        send_message(msg,2);
        time_from_last_msg = millis();
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
    if(time_from_last_msg - millis() > CHECK_MSG_FRQ){
      msg[0] = 0;
      msg[1] = (unsigned char)((int)(analogRead(cantar)/kg));
      send_message(msg,2);
      time_from_last_msg = millis();
    }

    // CHECK IF WE HAVE A REQUEST
    if(digitalRead(req1) == LOW){
      msg[0] = 8;
      msg[1] = 0;
      msg[2] = (unsigned char)((int)(analogRead(cantar)/kg));
      send_message(msg,3);
      unsigned char status = ciclu(do1,do2,ds1,ds2,ciclu_in);
      msg[0] = 9;
      msg[1] = status;
      send_message(msg,2);
    }else if(digitalRead(req2) == LOW){
      msg[0] = 8;
      msg[1] = 1;
      msg[2] = (unsigned char)((int)(analogRead(cantar)/kg));
      send_message(msg,3);
      unsigned char status = ciclu(do2,do1,ds2,ds1,ciclu_out);
      msg[0] = 9;
      msg[1] = status;
      send_message(msg,2);
    }
    delay(2);
  }
}
