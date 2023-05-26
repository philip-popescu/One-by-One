#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <SD.h>
#include "macro.h"

#define MODE_TCP true

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE0
};
IPAddress ip;
unsigned int localPort;      // local port to listen on
IPAddress remote_ip;
unsigned int remote_port;    // server aplication port

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";        // a string to send back


#if MODE_TCP
    char **buff;
    int index = 0;
    EthernetServer server(80);

    //Dezalocarea memoriei pentru buffer
    void clean_buffer(){
        for(int i=0; i < BUFFER_LEN;i++){
            free(buff[i]);
        }
        free(buff);
        buff = NULL;
        index = 0;
    }

    //Alocarea memoriei pentru buffer
    void make_buffer(){
        buff = (char**)malloc(BUFFER_LEN*sizeof(char*));
        for(int i=0; i < BUFFER_LEN;i++){
            buff[i] = (char*)malloc(BUFFER_WIDTH*sizeof(char));
        }
        index = 0;
    }

    //Adaugarea la bufferul de afisare
    void add_buffer(const char *msg){
        if(buff == NULL){
            make_buffer();
        }
        if(index < BUFFER_LEN){
            strcpy(buff[index],msg);
            index++;
        }else{
            char *aux = buff[index-1],*aux2;
            
            for(int i = index-1; i > 0; i--){
            aux2 = buff[i-1];
            buff[i-1] = aux;
            aux = aux2;
            }
            free(aux);
            buff[index-1] = (char*)malloc(BUFFER_WIDTH);
            strcpy(buff[index-1],msg);
        }
    }

    ISR(TIMER1_OVF_vect){
        EthernetClient client = server.available();
        if (client && index != 0) {
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
                        
                        for(int i = 0 ; i < index; i++){
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
            client.stop();
        }
    }
#else
    // An EthernetUDP instance to let us send and receive packets over UDP
    EthernetUDP Udp;
#endif

File data;

void client_setup() {
    Serial.print("Initializing SD card...");

    if (!SD.begin(4)) {
        Serial.println("initialization failed!");
        while (1);
    }
    Serial.println("initialization done.");

    data = SD.open("data.txt", FILE_READ);

    unsigned char buff[50];
    ip.fromString(data.readStringUntil('\n'));
    Serial.print("Local IP: ");
    Serial.println(ip);

    data.readStringUntil('\n').getBytes(buff, 50);
    localPort = atoi(buff);
    Serial.print("Local PORT: ");
    Serial.println(localPort);

    remote_ip.fromString(data.readStringUntil('\n'));
    Serial.print("Remote IP: ");
    Serial.println(remote_ip);

    data.readStringUntil('\n').getBytes(buff, 50);
    remote_port = atoi(buff);
    Serial.print("Remote PORT: ");
    Serial.println(remote_port);

    data.close();
    // start the Ethernet
    Ethernet.begin(mac, ip);
    
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
        }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
    }

    #if MODE_TCP
        server.begin();
        Serial.print("server is at ");
        Serial.println(Ethernet.localIP());

        TCCR1B = 0;
        TCCR1A = 0;

        TCNT1 = 0; //	set value of the counter to zero
        //TCCR1B |= (1 << CS10);	// choose no prescaler
        TCCR1B |= (1 << CS12);	// choose 256 prescaler
        
        TIMSK1 |= (1 << TOIE1); // enable timer overflow interrupt.
    #else
        // start UDP
        Udp.begin(localPort);
    #endif
}

char * translate(const char* buffer,unsigned long buff_size) {

    char *msg = calloc(BUFFER_WIDTH, sizeof(char));

    switch (buffer[1]) {
        default:
            break;
    }

    return msg;

}

void send_message(const char* buffer,unsigned long buff_size){
    #if MODE_TCP
        if(buffer != NULL){
            char * msg = translate(buffer, buff_size);
            add_buffer(msg);
            free(msg);
        }
    #else
        Udp.beginPacket(remote_ip, remote_port);
        Udp.write(buffer, buff_size);
        Udp.endPacket();
    #endif
}
