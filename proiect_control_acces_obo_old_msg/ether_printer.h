#include <SPI.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include "macro.h"

char **buff;
int index = 0;
EthernetServer server(80);

//Dezalocarea memoriei pentru buffer
void clean_buffer() {
  for (int i = 0; i < BUFFER_LEN; i++) {
    free(buff[i]);
  }
  free(buff);
  buff = NULL;
  index = 0;
}

//Alocarea memoriei pentru buffer
void make_buffer() {
  buff = (char **)malloc(BUFFER_LEN * sizeof(char *));
  for (int i = 0; i < BUFFER_LEN; i++) {
    buff[i] = (char *)malloc(BUFFER_WIDTH * sizeof(char));
  }
  index = 0;
}

//Adaugarea la bufferul de afisare
void add_buffer(char *msg) {

  if (buff == NULL) {
    make_buffer();
  }
  if (index < BUFFER_LEN) {
    strcpy(buff[index], msg);
    index++;
  } else {
    char *aux = buff[index - 1], *aux2;

    for (int i = index - 1; i > 0; i--) {
      aux2 = buff[i - 1];
      buff[i - 1] = aux;
      aux = aux2;
    }
    free(aux);
    buff[index - 1] = (char *)malloc(BUFFER_WIDTH);
    strcpy(buff[index - 1], msg);
  }
}

//Functie de printare la portul serial cu buffer de 20 de stringuri
void ether_print(char *msg) {
  if (msg != NULL) {
    add_buffer(msg);
  }

  EthernetClient client = server.available();
  if (client && index != 0) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the
          //client.println("Refresh: 5");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin

          for (int i = 0; i < index; i++) {
            client.print(buff[i]);
            client.println("<br />");
          }
          client.println("</html>");
          clean_buffer();
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    delay(2);
    client.stop();
    Serial.println("client disconnected");
  }
}

//Adaugare a unui numar la un string
char *toString(int nr) {
  char *message = (char *)malloc(BUFFER_WIDTH);
  int k = 0;
  do {
    int x = nr % 10;
    message[k] = '0' + x;
    nr /= 10;
    k++;
  } while (nr);
  message[k] = '\0';
  for (int i = 0; i < k / 2; i++) {
    char x = message[i];
    message[i] = message[k - 1 - i];
    message[k - 1 - i] = x;
  }
  return message;
}
void add_to_string(char *msg, int nr) {
  char *aux = toString(nr);
  strcat(msg, aux);
  free(aux);
}

//Initializare WebServer
void server_setup(byte mac[], IPAddress ip) {
    Serial.print("Initializing SD card...");

    if (!SD.begin(4)) {
        Serial.println("initialization failed!");
        while (1);
    }
    Serial.println("initialization done.");

    File data = SD.open("data.txt", FILE_READ);

    unsigned char buff[50];
    ip.fromString(data.readStringUntil('\n'));
    Serial.print("Local IP: ");
    Serial.println(ip);

    data.readStringUntil('\n').getBytes(buff, 50);
    unsigned int localPort = atoi(buff);
    Serial.print("Local PORT: ");
    Serial.println(localPort);

    data.close();

  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}
