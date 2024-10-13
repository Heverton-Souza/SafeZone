#include <WiFi.h>
#include <NTPClient.h>

//Definição dos pinos 
int sensor = 5;
int buzzer = 4;
const int ledVermelho = 22;
const int ledVerde = 19;
const int ledAzul = 23;
String horaRecebida = "";
String horario = "";
bool estadoSensor = false;
bool estadoApp = false;

// Defina o nome da rede Wi-Fi e a senha
const char* ssid = "Galaxy";
//const char* ssid = "Mengo";
const char* password = "Fred1234";

// Variável para armazenar o dado recebido
String recebido = "";

WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

void aberto(){
  estadoSensor = true;
}

void getTime(){
  if (ntp.update()) {

      Serial.print("HORARIO: ");
      Serial.println(ntp.getFormattedTime());
      horario = ntp.getFormattedTime();
      
    } else {
      Serial.println("!Erro ao atualizar NTP!\n");
    }
}
void getMessage(){
  WiFiClient client = server.available(); 

  if (client.available()) {
    String comando = client.readString();  // Lê a linha de comando
    client.flush();  // Limpa qualquer dado restante no buffer

    // Verifica se o comando é um GET com "/0" ou "/1"
    if (comando.indexOf("GET /0") >= 0) {
      Serial.println("Comando recebido: 0");
      estadoApp = false;
    } else if (comando.indexOf("GET /1") >= 0) {
      Serial.println("Comando recebido: 1");
      estadoApp = true;
    } else if (comando.indexOf("GET /hora=") >= 0) {
      // Pega a substring que contém a hora
      int pos = comando.indexOf("hora=") + 5;
      horaRecebida = comando.substring(pos, pos + 5);  // Supondo que a hora está no formato HH:MM
      Serial.print("Hora recebida: ");
      Serial.println(horaRecebida);
      // Aqui você pode fazer algo com a hora, como armazenar ou acionar algo
    }

    // Envia uma resposta HTTP para o cliente (App Inventor)
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/plain");
    client.println(); // Linha em branco necessária para finalizar o cabeçalho HTTP
  }   
}

void setup_wifi() {
  int cont = 0;

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (cont == 15) {
      Serial.println();
      cont = 0;
    }
    cont++;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(sensor, INPUT_PULLUP); //We are using INPUT_PULLUP instead of connecting a 10kΩ resistor between the reed switch and GND.
  pinMode(buzzer, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  digitalWrite(ledVerde, HIGH);
  attachInterrupt(digitalPinToInterrupt(sensor), aberto, RISING);
  setup_wifi();
  ntp.begin();
  //GMT em segundos
  // +1 = 3600
  // +8 = 28800
  // -1 = -3600
  // -3 = -10800 (BRASIL)
  ntp.setTimeOffset(-10800);
}

void loop() 
{
  getMessage();
  getTime();
  if (horario == horaRecebida){
    estadoApp = true;
    horaRecebida = "";
  }
  if(estadoApp == false)
  {
    estadoSensor = false;
    digitalWrite(ledVermelho, LOW);
    digitalWrite(ledVerde, HIGH);
    tone (buzzer, 0);
  }
  
  if(estadoApp == true){
    if (estadoSensor == true) //Quando o sensor está separado, ele vai a 1
    {
      digitalWrite(ledVermelho, HIGH); 
      // PWM para aumentar o brilho do LED vermelho
      for (int frequency = 1000; frequency <= 4000; frequency += 10) {
        tone(buzzer, frequency);  // Toca o som com a frequência atual
        delay(5);  // Aguarda um tempo para criar o efeito de sirene
      }
    for (int frequency = 4000; frequency >= 1000; frequency -= 10) {
        tone(buzzer, frequency);  // Toca o som com a frequência atual
        delay(5);  // Aguarda um tempo para criar o efeito de sirene
      }
    }
  }
}



