#include <GyverPower.h>
#include <SoftwareSerial.h>

/*
 * D6 - RX Arduino (TX SIM800L), D5 - TX Arduino (RX SIM800L)
 */
SoftwareSerial SIM800(5, 6);

String response = "";
String phone = "phone number input";
String phrase = "start";

void setup() {
  Serial.begin(9600);
  Serial.println("Start!");
  SIM800.begin(9600);
  sendATCommand("AT",true);
  sendATCommand("AT+CMGF=1",true);
  sendATCommand("AT+CSCLK=1",true);
  sendATCommand("ATE0",true);

  /*
   * Подключаем прерывание на пин D2 , к RING пину GSM модуля.
   * Когда придет вызов или смс то на пин D2 придет 0 с RING и ардуина проснется
  */
  attachInterrupt(0, wakeUP, FALLING);
  pinMode(2, INPUT);
  digitalWrite(2,HIGH);

  /*
   * После того как проснется ардуина, проснется GSM модуль.
   * Пин D4 к DTR пину GSM модуля. 1 - пробуждаем GSM модуль.
  */
  pinMode(4, OUTPUT);
  digitalWrite(4,HIGH);

  /*
   * led off
  */
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  /*
   * sleep mode
  */
  power.setSleepMode(POWERDOWN_SLEEP);
  power.setSystemPrescaler(PRESCALER_2); 
}

void loop() {  
  power.sleep(SLEEP_FOREVER);
  if (digitalRead(2) == 0) {
     power.setSystemPrescaler(PRESCALER_1);
     delay(100);
     checkInputSMS();
     delay(100);
     digitalWrite(4,HIGH);
     power.setSystemPrescaler(PRESCALER_2);
  }
}

/*
 * Отправка команды в модем и получение результата по команде.
 */
String sendATCommand(String cmd, bool waiting) {
  String resp = "";   
  SIM800.println(cmd);
  if (waiting) {
    resp = SIM800.readString();
    Serial.println(resp);
  }
  return resp;
}

/*
 * Проверка что пришло смс, чтение смс + парсинг на номер и сообщение + удаление.
 */
void checkInputSMS(){
    if (SIM800.available()) {
    response = SIM800.readString();
      if (response.startsWith("\r\n+CMTI:")) {
        int index = response.lastIndexOf(",");
        String result = response.substring(index + 1, response.length());
        result.trim();
        response=sendATCommand("AT+CMGR="+result, true);
        parseSMS(response);
        sendATCommand("AT+CMGDA=\"DEL ALL\"", true);
      }
    }
}

void parseSMS(String msg) {
  String msgHeader  = "";
  String msgBody    = "";
  String msgPhone    = "";

  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgHeader = msg.substring(0, msg.indexOf("\r"));

  msgBody = msg.substring(msgHeader.length() + 2);
  msgBody = msgBody.substring(0, msgBody.lastIndexOf("OK"));
  msgBody.trim();

  int firstIndex = msgHeader.indexOf("\",\"") + 3;
  int secondIndex = msgHeader.indexOf("\",\"", firstIndex);
  msgPhone = msgHeader.substring(firstIndex, secondIndex);
  if (phone == msgPhone && phrase == msgBody){
    
    //Тут включаем брелок. Пока для теста включаем диод.
    digitalWrite(13, HIGH);
    delay(5000);
    digitalWrite(13, LOW);
    Serial.println("!");
  }
}

void sendSMS(String phone, String message) {
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);
  sendATCommand(message + "\r\n" + (String)((char)26), true);
}

/*
 * Дёргаем за функцию "проснуться".
 * Без неё проснёмся чуть позже (через 0-8 секунд).
 */
void wakeUP() {
  digitalWrite(4,LOW);
  power.wakeUp();
}
