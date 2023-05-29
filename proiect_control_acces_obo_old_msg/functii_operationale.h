#include "ether_printer.h"
#include "macro.h"

//pasul de cuantizare
double kg;
//mesaj ce va fi printat
char msg[BUFFER_WIDTH];
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
  ds1 = 35; ds2 = 37; req1 = 39; req2 =41; emg = 48; 
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
int check_doors(){

#ifdef DEBUG
    static unsigned long long t = 0;
#endif
  int ok = 1;
  msg[0] = '\0';
  if(digitalRead(ds1) == HIGH){
    digitalWrite(led1,LOW);
    strcat(msg,"Door 1 open! ");
    ok = 0;
  }else{
    digitalWrite(led1,HIGH);
  }

  if(digitalRead(ds2) == HIGH){
    digitalWrite(led2,LOW);
    strcat(msg,"Door 2 open! ");
    ok = 0;
  }else{
    digitalWrite(led2,HIGH);
  }
  if(millis() % CHECK_MSG_FRQ < MIN_TIME_ERR && !ok){
    ether_print(msg);
    delay(2);
  }

#ifdef DEBUG
    if (millis() - t > 500 && !ok) {
        Serial.println("WARNING: DOOR OPEN!");
        t = millis();
    }
#endif
  
  return ok;
}


//Returneaza ce clasa de greutate este activa
int check_class(){
  unsigned long timp = millis();
  for(int i = 0; i < 6; i++){
    if(digitalRead(CLG[i]) == LOW){
      while(millis() - timp < 100) {
        if (digitalRead(CLG[i]) == HIGH) {
          goto ret;
        }
      }
      if(digitalRead(CLG[i]) == LOW){
        strcpy(msg,"Recived weight class: ");
        add_to_string(msg,i+1);
        ether_print(msg);
        Serial.print("Clasa citita: ");
        Serial.println(i+1);
        return i + 1;
      }
    }
  }
ret:
  return 0;
}

//Da semnal de ciclu fals
void false_cicle(){
  strcpy(msg,"FALSE CICLE!");
  ether_print(msg);
  digitalWrite(ERR_ciclu_fals, LOW);
  delay(1000);
  digitalWrite(ERR_ciclu_fals, HIGH);
  delay(50);
}

//Functia de efectoare a ciclului de intrare/iesire
//Functia de efectoare a ciclului de intrare/iesire
void ciclu(int in, int out, int s_in, int s_out, int type){
  digitalWrite(activ_cicle,LOW);
  strcpy(msg,"Cicle starting...");
  ether_print(msg);
  
#ifdef DEBUG
    Serial.println("Am inceput ciclul.");
#endif

  int greutate0 = 0, greutate = 0, ok = 0, max_tries = MAX_TRIES;
  unsigned long t0 = millis();

  for(int i = 0 ; i < 5; i++ ){
    greutate0 += analogRead(cantar);
    delay(10);
  }
  greutate0 /= 5;

  strcpy(msg,"Base weight: ");
  add_to_string(msg,(int)(greutate0/kg));
  ether_print(msg);

#ifdef DEBUG
    Serial.println("GET IN THERE LEWIS!");
#endif

  digitalWrite(in,LOW);
  t0 = millis();
  while(millis() - t0 < CMD_LEN){ether_print(NULL);check_doors();}
  digitalWrite(in,HIGH);
  delay(200);
  while(!check_doors()){
    digitalWrite(in,LOW);
    t0 = millis();
    while(millis() - t0 < CMD_FOR_LOCK){check_doors();}
    digitalWrite(in,HIGH);
    t0 = millis();
    while(millis() - t0 < W8_TIME_LOCK){check_doors();}
  }
  
  if(analogRead(cantar)*1.0 - greutate0*1.0 < TAR_PLUS){
    false_cicle();
    while(digitalRead(req1) == LOW || digitalRead(req2) == LOW){ether_print(NULL); check_doors();}
    digitalWrite(activ_cicle,HIGH);
    return;
  }

  for(int i = 0 ; i < 5; i++ ){
    greutate += analogRead(cantar);
    delay(10);
  }
  greutate /= 5;

  
#ifdef DEBUG
    Serial.println("DA-I CU CARDUL MAI VASILE, MAI!");
#endif
  
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

      strcpy(msg, "Total weight:");
      add_to_string(msg,(int)(greutate/kg));
      strcat(msg,"kg.");
      ether_print(msg);
      
      int g = greutate - greutate0; 
      
      strcpy(msg, "You have");
      add_to_string(msg,(int)(g/kg));
      strcat(msg,"kg.");
      ether_print(msg);
      
      if ( clasa == 6) {
        ok = 1;
      }else if(((clasa*20.0 + 30.0) - 12.5)*kg < g && g < ((clasa*20.0 + 30.0) + 12.5)*kg){ ok = 1; }
    }
  }

  if(digitalRead(req1) == HIGH &&  digitalRead(req2) == HIGH){ ok = 0; }
  
  if(!ok){ false_cicle(); }else{
    digitalWrite(type,LOW);
    t0 = millis();
    while(millis() - t0 < ACK_LEN){check_doors();}
    digitalWrite(type,HIGH);
    delay(10);
  }

  while(ok && (analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || check_doors() == 0)){
    digitalWrite(out,LOW);
    t0 = millis();
    while(millis() - t0 < CMD_FOR_LOCK){check_doors();}
    digitalWrite(out,HIGH);
    t0 = millis();
    while(millis() - t0 < W8_TIME_LOCK){check_doors();}
  }

  
  while(!ok && (analogRead(cantar)*1.0 > MIN_WEIGHT + TAR_PLUS || check_doors() == 0)){
    digitalWrite(in,LOW);
    t0 = millis();
    while(millis() - t0 < CMD_FOR_LOCK){check_doors();}
    digitalWrite(in,HIGH);
    t0 = millis();
    while(millis() - t0 < W8_TIME_LOCK){check_doors();}
  }

  while(digitalRead(req1) == LOW || digitalRead(req2) == LOW){ether_print(NULL); check_doors();}
  digitalWrite(activ_cicle,HIGH);

    strcpy(msg, "Exiting cicle with: ");
    strcat(msg, (ok)? "SUCCESS" : "FAILURE");
    ether_print(msg);
  
#ifdef DEBUG
    Serial.println("Exiting cicle...");
#endif
}
