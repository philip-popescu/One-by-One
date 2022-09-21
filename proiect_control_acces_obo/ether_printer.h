#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <SD.h>

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

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

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

  // start UDP
  Udp.begin(localPort);
}

void send_message(const char* buff,unsigned long buff_size){
  Udp.beginPacket(remote_ip, remote_port);
  Udp.write(buff, buff_size);
  Udp.endPacket();
}
