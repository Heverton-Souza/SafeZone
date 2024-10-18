#include <WiFi.h>      // Biblioteca para controle da conexão WiFi
#include <NTPClient.h> // Biblioteca para sincronização de horário via NTP

// Definição dos pinos de conexão dos componentes
int sensor = 5;              // Pino de entrada para o sensor magnético (MC-38)
int buzzer = 4;              // Pino de saída para o buzzer
const int ledVermelho = 22;   // Pino do LED vermelho
const int ledVerde = 19;      // Pino do LED verde
const int ledAzul = 23;       // Pino do LED azul

// Variáveis para armazenar horários e estados
String horaAtivacao = "";     // Variável para armazenar a hora de ativacao recebida pelo app
String horaDesativacao = "";  // Variável para armazenar a hora de desativacao recebida pelo app
String horario = "";          // Variável para armazenar a hora atual
bool estadoSensor = false;    // Estado do sensor (porta/janela aberta ou fechada)
bool estadoApp = false;       // Estado do alarme controlado pelo app (ativado/desativado)

// Defina o nome e senha da rede Wi-Fi
const char* ssid = "Galaxy";  // Nome da rede Wi-Fi
const char* password = "Fred1234"; // Senha da rede Wi-Fi

// Variável para armazenar os dados recebidos via HTTP
String recebido = "";

// Configuração do servidor HTTP e cliente NTP
WiFiServer server(80);  // Cria um servidor na porta 80 (HTTP)
WiFiUDP ntpUDP;         // Cria uma instância UDP para NTP
NTPClient ntp(ntpUDP, "pool.ntp.org");  // Inicializa o cliente NTP

// Função chamada quando o sensor detecta a porta/janela aberta
void aberto() {
  estadoSensor = true;
}

// Função para atualizar o horário via NTP
void getTime() {
  ntp.update();
  horario = ntp.getFormattedTime();  // Armazena a hora atual   
}

// Função para tratar os comandos recebidos via HTTP do App Inventor
void getMessage() {
  WiFiClient client = server.available();  // Aguarda a conexão de um cliente (app)

  if (client.available()) {
    String comando = client.readString();  // Lê o comando recebido
    client.flush();  // Limpa o buffer de dados

    // Verifica os comandos recebidos para ativar ou desativar o alarme
    if (comando.indexOf("GET /0") >= 0) {
      Serial.println("Comando recebido: 0");
      estadoApp = false;    // Desativa o alarme
      horaAtivacao = "";     // Limpa o horário de ativação
      horaDesativacao = "";  // Limpa o horário de deativação

    } else if (comando.indexOf("GET /1") >= 0) {
      Serial.println("Comando recebido: 1");
      estadoApp = true;   // Ativa o alarme

    } else if (comando.indexOf("GET /horaAtivacao=") >= 0) { // Verifica se uma hora para ser ativado foi agendada
      int pos = comando.indexOf("horaAtivacao=") + 13;  
      horaAtivacao = comando.substring(pos, pos + 5) + ":00";    // Extrai a hora (HH:MM) e adiciona segundos
      Serial.print("Hora de ativação recebida: ");
      Serial.println(horaAtivacao);
  } else if (comando.indexOf("GET /horaDesativacao=") >= 0) { // Verifica se uma hora para ser desativado foi agendada
      int pos = comando.indexOf("horaDesativacao=") + 16; 
      horaDesativacao = comando.substring(pos, pos + 5) + ":00";  // Extrai a hora (HH:MM) e adiciona segundos
      Serial.print("Hora de desativação recebida: ");
      Serial.println(horaDesativacao);
  }

    // Envia uma resposta HTTP simples ao app para confirmar o recebimento
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/plain");
    client.println(); // Fim do cabeçalho HTTP
  }   
}

// Função para conectar o ESP32 à rede Wi-Fi
void setup_wifi() {
  int cont = 0;

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  // Inicia a conexão Wi-Fi

  // Tenta conectar à rede Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (cont == 15) {
      Serial.println();
      cont = 0;
    }
    cont++;
  }

  // Exibe as informações de conexão
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  // Mostra o IP obtido

  server.begin();  // Inicia o servidor HTTP
}

void setup() {
  Serial.begin(115200);  // Inicializa a comunicação serial
  pinMode(sensor, INPUT_PULLUP);  // Configura o pino do sensor como entrada com pull-up
  pinMode(buzzer, OUTPUT);        // Configura o pino do buzzer como saída
  pinMode(ledVermelho, OUTPUT);   // Configura o pino do LED vermelho como saída
  pinMode(ledVerde, OUTPUT);      // Configura o pino do LED verde como saída
  pinMode(ledAzul, OUTPUT);       // Configura o pino do LED azul como saída

  digitalWrite(ledVerde, HIGH);   // Inicializa o LED verde como ligado (alarme desativado)
  
  attachInterrupt(digitalPinToInterrupt(sensor), aberto, RISING);  // Interrupção para quando o sensor for ativado

  setup_wifi();  // Conecta ao Wi-Fi
  ntp.begin();   // Inicia o cliente NTP
  ntp.setTimeOffset(-10800);  // Define o fuso horário (Brasil: GMT-3)
}

void loop() {
  getMessage();  // Verifica se há comandos do app
  getTime();     // Atualiza a hora via NTP 

  // Se a hora atual coincide com a hora agendada para ativação pelo usuário, ativa o alarme
  if (horario == horaAtivacao) {
    estadoApp = true;
  }

  // Se a hora atual coincide com a hora agendada para desativacao pelo usuário, desativa o alarme
  if (horario == horaDesativacao) {
    estadoApp = false;
  }

  // Se o alarme está desativado pelo app, o sistema permanece inativo
  if(estadoApp == false) {
    estadoSensor = false;
    digitalWrite(ledVerde, LOW);  // LED vermelho desligado 
    digitalWrite(ledVermelho, LOW);  // LED vermelho desligado 
    digitalWrite(ledAzul, HIGH);    // LED verde ligado 
    tone(buzzer, 0);                 // Buzzer silencioso
  }

  // Se o alarme está ativado pelo app
  if(estadoApp == true) {
    digitalWrite(ledAzul, LOW); // Apaga o LED azul
    digitalWrite(ledVerde, HIGH); // Acende o LED verde
    if (estadoSensor == true) {  // Se o sensor detecta a porta/janela aberta
      digitalWrite(ledVerde, LOW); // Apaga o LED verde
      digitalWrite(ledAzul, LOW); // Apaga o LED azul
      digitalWrite(ledVermelho, HIGH);  // Acende o LED vermelho
      // Emite um som de sirene com o buzzer, aumentando e diminuindo a frequência
      for (int frequency = 1000; frequency <= 4000; frequency += 10) {
        tone(buzzer, frequency);  // Aumenta a frequência
        delay(5);
      }
      for (int frequency = 4000; frequency >= 1000; frequency -= 10) {
        tone(buzzer, frequency);  // Diminui a frequência
        delay(5);
      }
    }
  }
}
