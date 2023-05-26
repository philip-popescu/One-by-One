#include "ether_printer.h"
#include "macro.h"

//pasul de cuantizare
double kg;
//mesaj ce va fi printat
unsigned char msg[BUFFER_WIDTH];
//inputs
/*
 * CLG[6] - clasele de greutate
 * ds1/ds2 - senzorii usa 1/2
 * req1/2 - comanda deschidere usa 1/2
 * emg - emergency button
 * cantar - greutatea de pe cantar
*/
int CLG[6], ds1, ds2, req1, req2, emg, cantar;

//outputs
/*
 * do1/2 - comanda de deschidere usa 1/2
 * led1/2 - indicator usa1/2
 * ERR_cantar - eroare de indicare a greutatii cantarului
 * ERR_ciclu_fals - eroare ca s-a efectuat un ciclu necorespunzator
 * ciclu_in/ciclu_out - tipul de ciclu efectuat (in/out)
 */
int do1, do2, led1, led2, ERR_cantar, ERR_ciclu_fals, activ_cicle, ciclu_in, ciclu_out;

//Serup pentru inputuri si outputuri
void setup_IO(){
  //initializare intrari si iesiri
  cantar = A13;
  for(int i = 0; i < 6; i++){
    CLG[i] = 23+2*i;
    pinMode(CLG[i],INPUT_PULLUP);
  }
  ds1 = 35; ds2 = 37; req1 = 39; req2 = 41; emg = 48; 
  pinMode(ds1,INPUT_PULLUP);
  pinMode(ds2,INPUT_PULLUP);
  pinMode(req1,INPUT_PULLUP);
  pinMode(req2,INPUT_PULLUP);
  pinMode(emg,INPUT_PULLUP);

  do1 = 22; do2 = 24; led1 = 45; led2 = 46; ERR_cantar = 32; ERR_ciclu_fals = 47; activ_cicle = 26; ciclu_in = 28; ciclu_out = 30;
  pinMode(do1,OUTPUT);
  pinMode(do2,OUTPUT);
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(ERR_cantar,OUTPUT);
  pinMode(ERR_ciclu_fals,OUTPUT);
  pinMode(activ_cicle,OUTPUT);
  pinMode(ciclu_in,OUTPUT);
  pinMode(ciclu_out,OUTPUT);

  digitalWrite(do1,HIGH);
  digitalWrite(do2,HIGH);
  digitalWrite(led1,HIGH);
  digitalWrite(led2,HIGH);
  digitalWrite(ERR_cantar,HIGH);
  digitalWrite(ERR_ciclu_fals,HIGH);
  digitalWrite(activ_cicle,HIGH);
  digitalWrite(ciclu_in,HIGH);
  digitalWrite(ciclu_out,HIGH);
}



//Functia care verifica usile
int check_doors(int send){
  static unsigned long time_from_last_msg;
  int ok = 1; 
  unsigned char door_no = -1;
  msg[0] = '\0';
  if(digitalRead(ds1) == HIGH){
    digitalWrite(led1,LOW);
    door_no = 1;
    ok = 0;
  }else{
    digitalWrite(led1,HIGH);
  }

  if(digitalRead(ds2) == HIGH){
    digitalWrite(led2,LOW);
    door_no = 2;
    ok = 0;
  }else{
    digitalWrite(led2,HIGH);
  }

  if(millis() - time_from_last_msg > CHECK_MSG_ERROR_FRQ && !ok && send){
    msg[0] = 1;
    msg[1] = door_no;
    send_message(msg, 2);
    time_from_last_msg = millis();
  }
  
  return ok;
}


//Returneaza ce clasa de greutate este activa
int check_class(){
  unsigned long timp = millis();
  for(int i = 0; i < 6; i++){
    if(digitalRead(CLG[i]) == LOW){
      int ok = 1;
      while(millis() - timp < 100) {
        if (digitalRead(CLG[i]) == HIGH) {
          ok = 0;
          break;
        }
      }
      if(digitalRead(CLG[i]) == LOW && ok){
        Serial.println(i+1);
        return i + 1;
      }
    }
  }
  return 0;
}

//Functia de efectoare a ciclului de intrare/iesire
unsigned char ciclu(int in, int out, int s_in, int s_out, int type){

  unsigned char status = 0;

  digitalWrite(activ_cicle,LOW);
  
  int greutate0 = 0, greutate = 0, ok = 0, max_tries = MAX_TRIES;
  unsigned long t0 = millis();

  // READ REFFERENCE WEIGHT
  for(int i = 0 ; i < 5; i++ ){
    greutate0 += analogRead(cantar);
    delay(10);
  }
  greutate0 /= 5;

  // OPEN AND CLOSE THE ACCESS DOOR
  digitalWrite(in,LOW);
  delay(CMD_LEN);
  digitalWrite(in,HIGH);
  delay(200);
  while(!check_doors(0)){
    digitalWrite(in,LOW);
    delay(CMD_FOR_LOCK);
    digitalWrite(in,HIGH);
    delay(W8_TIME_LOCK);
  }
  delay(200);

  // Send cycle start message
  msg[0] = 8;
  msg[1] = (type - ciclu_in) / 2;
  msg[2] = (unsigned char)(greutate0/kg);
  send_message(msg, 3);

  // Send the weight after the door closes
  msg[0] = 10;
  msg[1] = (unsigned char)((int)(analogRead(cantar)/kg));
  send_message(msg,2);
  
  // If no person enters
  if(analogRead(cantar)*1.0 - greutate0*1.0 < TAR_PLUS){
    while(digitalRead(req1) == LOW || digitalRead(req2) == LOW){}
    digitalWrite(activ_cicle,HIGH);
    return 3;
  }

  for(int i = 0 ; i < 5; i++ ){
    greutate += analogRead(cantar);
    delay(10);
  }
  greutate /= 5;
  
  t0 = millis();
  while(millis() - t0 < CICLE_TIME && max_tries != 0 && !ok){
    int clasa = check_class();
    if(clasa != 0){
      max_tries--;
      greutate = 0;
      for(int i = 0 ; i < 5; i++ ){
        greutate += analogRead(cantar);
        delay(10);
      }
      greutate /= 5;
      
      int g = greutate - greutate0; 

      if (clasa == 6) {
        ok = 1;
        break;
      }
      
      if(((clasa*20.0 + 30.0) - 12.5)*kg < g && g < ((clasa*20.0 + 30.0) + 12.5)*kg){ 
        ok = 1; 
      } else {
        msg[0] = 11;
        msg[1] = clasa;
        msg[2] = (unsigned char)g;
        send_message(msg, 3);
      }
    }
  }

  if(!ok) { status = 2; }

  if(digitalRead(req1) == HIGH &&  digitalRead(req2) == HIGH){ 
    msg[0] = 12;
    send_message(msg, 1);
    ok = 0; 
    status = 4;
  }
  
  //SUCCESFULLY FINISH THE CYCLE
  if(ok){
    status = 1;
    digitalWrite(type,LOW);
    delay(ACK_LEN);
    digitalWrite(type,HIGH);
    delay(10);
  }

  delay(1000);

  // open the exit door
  while(ok && (analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || check_doors(0) == 0)){
    digitalWrite(out,LOW);
    delay(CMD_FOR_LOCK);
    digitalWrite(out,HIGH);
    delay(W8_TIME_LOCK);
  }

  // YOU SHALL NOT PASS!
  while(!ok && (analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || check_doors(0) == 0)){
    digitalWrite(in,LOW);
    delay(CMD_FOR_LOCK);
    digitalWrite(in,HIGH);
    delay(W8_TIME_LOCK);
  }

  while(digitalRead(req1) == LOW || digitalRead(req2) == LOW){}
  digitalWrite(activ_cicle,HIGH);

  return status;
}
