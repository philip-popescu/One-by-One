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
  cantar = A10;
  for(int i = 0; i < 6; i++){
    CLG[i] = 22+i;
    pinMode(CLG[i],INPUT);
  }
  ds1 = 28; ds2 = 29; req1 = 30; req2 =31; emg = 32; 
  pinMode(ds1,INPUT);
  pinMode(ds2,INPUT);
  pinMode(req1,INPUT);
  pinMode(req2,INPUT);
  pinMode(emg,INPUT);

  do1 = 33; do2 = 34; led1 = 35; led2 = 36; ERR_cantar = 37; ERR_ciclu_fals = 38; activ_cicle = 39; ciclu_in = 40; ciclu_out = 41;
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
  int ok = 1;
  msg[0] = '\0';
  if(digitalRead(ds1) == HIGH){
    digitalWrite(led1,LOW);
    strcat(msg,"Door1 open! ");
    ok = 0;
  }else{
    digitalWrite(led1,HIGH);
  }

  if(digitalRead(ds2) == HIGH){
    digitalWrite(led2,LOW);
    strcat(msg,"Door2 open! ");
    ok = 0;
  }else{
    digitalWrite(led2,HIGH);
  }
  if(millis() % CHECK_MSG_FRQ < MIN_TIME_ERR && !ok){
    ether_print(msg);
    delay(2);
  }
  
  return ok;
}


//Returneaza ce clasa de greutate este activa
int check_class(){
  for(int i = 0; i < 6; i++){
    if(digitalRead(CLG[i]) == LOW){
      strcpy(msg,"Clasa ta este: ");
      add_to_string(msg,i+1);
      ether_print(msg);
      Serial.println(i+1);
      return i + 1;
    }
  }
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
void ciclu(int in, int out, int s_in, int s_out, int type){
  digitalWrite(activ_cicle,LOW);
  strcpy(msg,"Am inceput ciclul.");
  ether_print(msg);
  
  int greutate0 = 0, greutate = 0, ok = 0, max_tries = MAX_TRIES;
  unsigned long t0 = millis();

  for(int i = 0 ; i < 5; i++ ){
    greutate0 += analogRead(cantar);
    delay(10);
  }
  greutate0 /= 5;

  strcpy(msg,"Tara este: ");
  add_to_string(msg,(int)(greutate0/kg));
  ether_print(msg);

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
  delay(200);
  
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

      strcpy(msg, "Greutate cantart:");
      add_to_string(msg,(int)(greutate/kg));
      strcat(msg,"kg.");
      ether_print(msg);
      
      int g = greutate - greutate0; 
      
      strcpy(msg, "Persoana are:");
      add_to_string(msg,(int)(g/kg));
      strcat(msg,"kg.");
      ether_print(msg);
      
      
      if(((clasa*20.0 + 30.0) - 12.5)*kg < g && g < ((clasa*20.0 + 30.0) + 12.5)*kg){ ok = 1; }
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

  delay(1000);

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
}
