
#define CMDOPEN 2000   //timpul comenzii de deschidere a usii de la controler (ms)
#define INPUTTIME 500  //timpul necesar unui imput valid (ms)
#define ERRTIME 200   //timpul erorii de dublu comanda (ms)

//INPUT-URI
int cmd_door_1, cmd_door_2, isOpen1, isOpen2;

//OUTPUT-URI
int open1, open2, door_1_led, door_2_led, simultaneous_cmd, boath_open;

void setup() {
  cmd_door_1 = 2; cmd_door_2 = 3; isOpen1 = 4; isOpen2 = 5;
  pinMode(cmd_door_1,INPUT);
  pinMode(cmd_door_2,INPUT);
  pinMode(isOpen1,INPUT);
  pinMode(isOpen2,INPUT);

  open1 = 6; open2 = 7; door_1_led = 8; door_2_led = 9; simultaneous_cmd = 10; boath_open = 11; 
  pinMode(open1,OUTPUT);
  pinMode(open2,OUTPUT);
  pinMode(door_1_led,OUTPUT);
  pinMode(door_2_led,OUTPUT);
  pinMode(simultaneous_cmd,OUTPUT);
  pinMode(boath_open,OUTPUT);

  digitalWrite(open1,LOW);
  digitalWrite(open2,LOW);
  digitalWrite(door_1_led,LOW);
  digitalWrite(door_2_led,LOW);
  digitalWrite(simultaneous_cmd,LOW);
  digitalWrite(boath_open,LOW);
}

// Verifica usile daca sunt deschise sau nu
void chech_doors(){
  // Stare usa1
  if(digitalRead(isOpen1) == HIGH){
    digitalWrite(door_1_led,HIGH);
  }else{
    digitalWrite(door_1_led,LOW);
  }
  
  // Stare usa2
  if(digitalRead(isOpen2) == HIGH){
    digitalWrite(door_2_led,HIGH);
  }else{
    digitalWrite(door_2_led,LOW);
  }

  // Vreificare simulnateitate usi
  if(digitalRead(isOpen1) == HIGH && digitalRead(isOpen2) == HIGH){ //daca am ambele usi deschise da eroare
      digitalWrite(boath_open,HIGH);
      delay(500);
  }else{
      digitalWrite(boath_open,LOW);
  }
}

//Eroare de comenzi simultane
void simultaneousERR(){
  int count = 3000 / ERRTIME;
  while(count !=0 ){
    digitalWrite(simultaneous_cmd, HIGH);
    delay(ERRTIME);
    digitalWrite(simultaneous_cmd, LOW);
    delay(ERRTIME);
    count--;
  }
}

//Rezolva procedura de deschidere a usii
void open_door(int out, int cmd, int cmd_err){
  unsigned long start = millis(), finish = INPUTTIME;
  /*
   * Astept timpul necesar unei comenzi corecte si verific sa
   *nu am comenzi simultane.
   */
  while((unsigned long)millis() - start < finish && digitalRead(cmd) == LOW){
    if(digitalRead(cmd_err) == LOW){
      simultaneousERR();
      return;
    }
  }
  /*
   * Verific daca am primit o comanda suficient de lunga
   */
  if((unsigned long)millis() - start < finish){
    delay(200);
    return;
  }
  // dau comanda deschideri usii
  digitalWrite(out,HIGH);
  chech_doors();
  delay(CMDOPEN);
  digitalWrite(out,LOW); 
}

void loop() {
  /*  
   *   Cat timp este o usa deschisa sistemul trebuie sa astepte.
   *  Daca sunt ambele usi deschise se trimite semnal ca ambele usi sunt deschise.
   */
  while(digitalRead(isOpen1) == HIGH || digitalRead(isOpen2) == HIGH){
    chech_doors();
  }
  chech_doors();
  /*
   * Verificam daca avem o comanda de deschidere a unei usi
   */
  if(digitalRead(cmd_door_1) == LOW){
    open_door(open1,cmd_door_1,cmd_door_2); //deschidere usa 1
  }else if(digitalRead(cmd_door_2) == LOW){
    open_door(open2,cmd_door_2,cmd_door_1); //deschidere usa 2
  }
  
}
