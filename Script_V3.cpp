#include <iostream>
#include <cmath>

#include <SPI.h>               //
#include <Adafruit_GFX.h>      // biblioteca pro visor   
#include <Adafruit_PCD8544.h>  //

#include "esp32-hal-dac.h"     // biblioteca pro esp32

#define UD_pin 4               //
#define inc_pin 2              // pinos para controlar o pot digital
#define chip 15                //
#define calib 35
#define adc 32  

#include <math.h>
double down_calibration[100];
double sigma_up[100];
double up_calibration[100];
double sigma_down[100];               

int adc_value;
int16_t adc1;
float volts1;

float currently_voltage = 0;

unsigned long agora;
long interval = 100;

//Adafruit_PCD8544(CLK, DIN, D/C, CE, RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(25,12,14,27,26); // pinos do display
int contrastValue = 60; // Default Contrast Value

String comando="";
String parametro="";

bool monit;

int Totalpassos=0;

void increment(){           //acho q eventualmente preciso tirar o delay pra colocar o currentmillis pra manter tudo sendo monitorado até na troca de tensão
  digitalWrite(inc_pin,HIGH);
  delay(interval);
  digitalWrite(inc_pin,LOW);
  delay(interval);
}


void start(){
  pinMode(UD_pin,OUTPUT);
  pinMode(inc_pin,OUTPUT);
  pinMode(chip,OUTPUT);
  pinMode(adc,INPUT);
  digitalWrite(UD_pin,LOW);
  digitalWrite(inc_pin,LOW);
  digitalWrite(chip,LOW);
  display.begin();
  Serial.begin(9600);
  display.clearDisplay();
  display.display();
  delay(1000);
  display.setTextColor(BLACK);
  display.setCursor(25,1);
  display.setTextSize(1);
  display.println("Fonte");
  display.setTextColor(BLACK);
  display.setCursor(33,10);
  display.setTextSize(1);
  display.println("HV");
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(10,30);
  display.println("Prototype 1");
  display.display();
  delay(5000);
  display.clearDisplay();
  VoltOff();

}

void Selfcalibration(){
  double soma_quad;
  double media;
  int amostras = 200;
  int soma;
  int acumulo[amostras]; //acumula os valores lidos no ADC
  //calibracao crescente
  digitalWrite(UD_pin,HIGH);
  for(int i=0;i<100;i++){
    soma = 0;
    soma_quad =0;
    media = 0;
    Totalpassos+=1; 
    tela_display();
    increment();
    for(int u=0; u<amostras; u++){
        acumulo[u] = analogRead(calib);
        delay(3); //adiciona os valores lidos pelo ADC convertidos nesse array
    }
    int tamanho = amostras;
    for(int a=0; a<tamanho;a++){
        soma += acumulo[a];
    }
    media = (soma * 1.0)/(tamanho-1); //tiro a média dos valores do array "acumulo"
    up_calibration[i] = media*3.3/4095; //o array acumulo ainda tem os valores medidos nessa run

    for(int c =0; c<tamanho;c++){
        soma_quad += pow((acumulo[c]*3.3/4095) - media*3.3/4095,2); 
    }
    sigma_up[i] = sqrt((soma_quad * 1.0)/(tamanho-1))/sqrt(tamanho);
  }
  // calibracao decrescente
  delay(1000);
  digitalWrite(UD_pin,LOW);
  for(int s=0;s<100;s++){
    soma = 0;
    soma_quad=0;
    media =0;
    Totalpassos-=1;
    tela_display();
    increment();
    for(int u=0; u<amostras; u++){
        acumulo[u] = analogRead(calib);
        delay(3);
    }

    int tamanho = amostras;
    for(int a=0; a<tamanho;a++){
        soma += acumulo[a];
    }

    media = (soma * 1.0)/(tamanho-1);
    down_calibration[s] = media*3.3/4095;

    for(int c =0; c<tamanho;c++){
        soma_quad += pow((acumulo[c]*3.3/4095) - media*3.3/4095,2); 
    }

    sigma_down[s] = sqrt((soma_quad * 1.0)/(tamanho-1))/sqrt(tamanho);
  }
}

void print(){
    int tamanhoprint = 100;
    for(int i=0;i<tamanhoprint;i++){
        Serial.print(up_calibration[i]);Serial.print(" "); Serial.print(sigma_up[i]); Serial.print(" ");
        Serial.print(down_calibration[i]); Serial.print(" "); Serial.println(sigma_down[i]);
    }

}


void VoltUp(int passos){
    digitalWrite(UD_pin,HIGH);
  for(int i=0; i< passos;i++){
    Totalpassos+=1; 
    tela_display();
    increment();
  }
}
void VoltDown(int passos){
    digitalWrite(UD_pin,LOW);
    for(int i=0; i< passos;i++){
    Totalpassos-=1;
    tela_display();
    increment();
  }
}
void VoltOff(){
    digitalWrite(UD_pin,LOW);
    for(int i=0;i<100;i++){
        digitalWrite(inc_pin,HIGH);
        delay(20);
        digitalWrite(inc_pin,LOW);
        delay(20);
    }
}

void lerserial() { //funçao que transforma o que e recebido pela serial em uma string
  comando = "";//reset do valor
  parametro="";//reset do valor
  boolean isCommand=true;//determina aonde salvar o caracter
  char carac; //armazena cada caracter
  while(Serial.available()>0) { // enquando commandeber caracteres
    carac=Serial.read(); //grava-lo na variavel carac
    if(carac ==' ') {//se o caracter for espaco
      isCommand=false;//gravar o restante na var parametro
    }
    if((carac !='\n') && (isCommand==true)) {//se o caracter NAO for quebra de linha e for um comando
      comando.concat(carac);//concatena o valor na var comando
    }
    if((carac !='\n') && (isCommand==false)) {//se o caracter NAO for quebra de linha e for um parametro
      parametro.concat(carac);//concatena o valor na var parametro
    }
    delay(50); //delay para nova leitura
  }
}
/*    unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
    // Salva o último tempo que o evento ocorreu
      previousMillis = currentMillis;
      adc_value = analogRead(adc);
      Serial.println(adc_value);
      display.setTextColor(BLACK);
      display.setCursor(10,22);
      display.setTextSize(1);
      display.print("ADC: ");
      display.print(adc_value);
      }*/


float calibracao[100] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 19.849679358717434, 29.77451903807615, 32.98591297993956, 95.70735547365138, 158.42879796736318, 221.15024046107513, 293.7965226341459, 386.29248416593384, 478.788445697722, 483.54933132765314, 513.3335817098642, 572.8936017241028, 612.6053537477915, 672.168377949002, 701.9586807180864, 751.6006915779324, 751.6207936269507, 801.2676780655453, 831.0677994153436, 890.6457310937795, 890.6783392512694, 950.2641777573783, 940.3808699582312, 1019.8264592720395, 1019.8793579974649, 1069.56325719145, 1109.3299937565803, 1129.2557155865006, 1169.0408960602958, 1179.06259489244, 1248.6457892260878, 1248.7691660227333, 1298.5326105236286, 1328.4642869205065, 1368.3410190409766, 1418.1654083632366, 1418.3913531844805, 1458.345725015915, 1518.1825856042444, 1528.4322799650804, 1558.5734523289043, 1618.5363225856167, 1648.7779256608467, 1698.9293149872717, 1729.298869660738, 1759.7450033569862, 1839.9017736767846, 1890.3815922059694, 1921.1218610711173, 2001.61063859988, 2012.765855843364, 2073.7036581729517, 2114.97056479648, 2156.439279766435, 2217.985473527623, 2269.8639668463256, 2302.1830401876687, 2394.378798747538, 2477.0195702580295, 2500.5288005858342, 2574.133510383377, 2628.420537554278, 2693.2329419288703, 2768.6480983478923, 2854.7533547500652, 2911.94795943031, 3039.5916271296874, 3088.937430694961, 3199.0749943825685, 3270.9160802268057, 3334.416015121188, 3449.3279573349223, 3556.333308284445, 3675.5419010943997, 3817.172276557642, 3922.006972790876, 4099.593563790043, 4241.181428814552, 4347.247690737002};

void setvoltage(float volt){
  if((volt - currently_voltage)>0){
    digitalWrite(UD_pin,HIGH);
    for(int i = Totalpassos; i<100; i++){
      if(abs(volt - calibracao[i]) < abs(calibracao[i-1] - calibracao[i+1])/2 ){
        Totalpassos += abs(Totalpassos - i);
        currently_voltage = calibracao[Totalpassos];
        break;
      }
      increment();
    }
  }
  else{
    digitalWrite(UD_pin,LOW);
    for(int i = Totalpassos; i>0; i--){
      if(abs(volt - calibracao[i]) < abs(calibracao[i-1] - calibracao[i+1])/2 ){
        Totalpassos -= abs(Totalpassos - i);
        currently_voltage = calibracao[Totalpassos];
        break;
      }
      increment();
    }
  }
}

 void monitorar(bool monit){
    if(monit==true){
      adc_value = analogRead(adc);
      display.setTextColor(BLACK);
      display.setCursor(10,18);
      display.setTextSize(1);
      display.print("ADC: ");
      display.print(adc_value * 3.3 / 4095);
      display.print(" V");
    }
    if(monit==false){
      display.setTextColor(BLACK);
      display.setCursor(0,18);
      display.setTextSize(1);
      display.print("Nao monitorando");
      display.display();

    }
 }

void interpreter(){
    if(Serial.available()) { // se estiver recebendo algo pela serial
    Serial.println("Estou recebendo");
    lerserial(); //executar a funçao que transforma os caracteres em string
    Serial.print("Comando:"); Serial.println(comando);
    // tratar
    if(comando=="voltup") {
        Serial.print("Subindo a tensao em "); Serial.print(parametro); Serial.println(" passos.");
        VoltUp(parametro.toInt());
        Serial.println("terminou!");
        //Totalpassos += parametro.toInt();
        Serial.print("Passo atual: "); Serial.println(Totalpassos);


    }
    if(comando=="off"){ 
        Serial.println("Desligando a fonte!");
        VoltOff();
        Totalpassos = 0;
     }
    if(comando == "voltdown" ){
        Serial.print("Abaixando a tensao em"); Serial.print(parametro); Serial.println(" passos.");
        VoltDown(parametro.toInt());
        Serial.println("terminou!");
        //Totalpassos -= parametro.toInt();
        Serial.print("Passo atual: "); Serial.println(Totalpassos);
        
    }
    if(comando == "startmonitor"){
      Serial.println("Monitorando");   
      monit = true;

    }
    if(comando == "stopmonitor"){
      Serial.println("Nao monitorando");   
      monit = false; 
    }
    if(comando == "setvoltage"){
      Serial.print("Tensão setada para: "); Serial.print(parametro.toFloat());Serial.println("V");
      setvoltage(parametro.toFloat());

    }
    if(comando == "settime"){
      Serial.print("Tempo setado para:"); Serial.print(parametro.toInt());Serial.println("ms");
      iTempo(parametro.toInt());
    }
    if(comando == "selfcalibration"){
      Serial.println("Calibração em andamento.");
      Selfcalibration();
      Serial.println("Calibração Feita. Para printar digite 'printcalibration'");
    }
    if(comando == "printcalibration"){
      print();
    }

  }//receber algo serial
}

void tela_display(){
  display.clearDisplay();
  display.setTextColor(BLACK);
  display.setCursor(1,1);
  display.setTextSize(1);
  display.print("Volt: ");
  display.print(calibracao[Totalpassos]);
  monitorar(monit);
  display.display();
}

void iTempo(int time){
  interval = time;
}

void setup() {
  // put your setup code here, to run once:
  start();

}

void loop() {
  // put your main code here, to run repeatedly:
  interpreter();
  tela_display();


}
