//Backup 2/12/2019 LineAlertBot

#include <WiFi.h>
#include <TridentTD_EasyFreeRTOS32.h>
#include <time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <MicroGear.h>
#include <ArduinoJson.h>

#define TRIAC_PIN     15
#define SCL_PIN       23  //SCL
#define SDA_PIN       22  //SDA
#define FLOW_PIN      21  //Input Water Rate
#define DHT22_PIN     19  //Input Temperature and humidity
//#define DHTTYPE DHT11   //DHT 11
#define DHTTYPE DHT22     //DHT 22 (AM2302), AM2321
//#define DHTTYPE DHT21   //DHT 21 (AM2301)
#define GLED_PIN       4  //Output LED Green
#define RLED_PIN       2  //Output LED Red
#define US100_Trig_PIN 5   //Trigger
#define US100_Echo_PIN 18  //Echo

#define TIMEZONE      7
#define FLOW_R        1.9  //5000.0 / 2805.0 = 1.783
#define BH1750_ADDR   0x23

// Config connect WiFi
#define WIFI_SSID "TPST_2.4G"
#define WIFI_PASSWORD "A123456789"
//#define WIFI_SSID "Papawarin"
//#define WIFI_PASSWORD "0934280211"
//#define WIFI_SSID "D-Link_DIR-612"
//#define WIFI_PASSWORD "A123456789"

#define APPID   "ESP32LineAlert"     //change this to your APPID
#define KEY     "fnlNqnV39yvVZcI"     //change this to your KEY
#define SECRET  "LoLGJN2dkZDYneEreW6yaQLes"     //change this to your SECRET
#define ALIAS   "LineAlert" //set name of drvice
#define TargetWeb "switch" //set target name of web

//change this to your linebot server ex.http://numpapick-linebot.herokuapp.com/bot.php
const char* host     = "https://esp32linealert.herokuapp.com/bot.php";

WiFiClient client;
TridentOS Sensor_Task, Ultrasonic_Task, DHT22_Task, SetTime_Task;
LiquidCrystal_I2C lcd(0x27, 16, 2); //ถ้าจอไม่แสดงผล ให้ลองเปลี่ยน 0x3F เป็น 0x27
DHT dht(DHT22_PIN, DHTTYPE);
MicroGear microgear(client);
void SensorTask(void*), UltrasonicTask(void*), DHT22Task(void*), SetTimeTask(void*); //ชื่อฟังกชั่นที่ taskจะเรียกทำงาน
volatile int NbTopsFan; //measuring the rising edges of the signal
uint16_t lux;
long duration, cm;
int dst = 0, count_wait = 0, data_timein[8], page = 0, weather_now = 0, weather_old = 0, water_now = 0, water_old = 0, timer = 0;
float calc = 0, rate_flow = 0, rain_flow = 0, water_level = 0, water_fix = 205, hum = 0, tmpc = 0, tmpf = 0;
String tmpNow = "", uid = "", msg_st = "";
/*0*/ String message[40] = {"เริ่มต้นระบบเตือนภัยน้ำท่วม",
                            /*1*/"วันเวลาปัจจุบัน " + String(tmpNow),
                            /*2*/"สภาพอากาศปกติ อุณหภูมิ : " + String(tmpc, 0) +
                            " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                            " % ความเข้มแสง : " + String(lux) + " lux",
                            /*3*/"สภาพอากาศร้อน อุณหภูมิ : " + String(tmpc, 0) +
                            " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                            " % ความเข้มแสง : " + String(lux) + " lux",
                            /*4*/"สภาพอากาศหนาว อุณหภูมิ : " + String(tmpc, 0) +
                            " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                            " % ความเข้มแสง : " + String(lux) + " lux",
                            /*5*/"สภาพอากาศฝนตก อุณหภูมิ : " + String(tmpc, 0) +
                            " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                            " % ความเข้มแสง : " + String(lux) + " lux",
                            /*6*/"สภาพอากาศฝนตกหนัก อุณหภูมิ : " + String(tmpc, 0) +
                            " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                            " % ความเข้มแสง : " + String(lux) + " lux",
                            /*7*/"ระดับน้ำปกติ ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*8*/"น้ำท่วมเท้า ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*9*/"น้ำท่วมหน้าแข้ง ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*10*/"น้ำท่วมเข่า ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*11*/"น้ำท่วมเอว ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*12*/"น้ำท่วมอก ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*13*/"น้ำท่วมคอ ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*14*/"น้ำท่วมหัว ความสูงของน้ำ : " + String(water_level / 100, 2) + " เมตร",
                            /*15*/"คำแนะนำ : ควรเฝ้าระวังระดับน้ำ และ ขนของขึ้นที่สูง",
                            /*16*/"คำแนะนำ : ควรเตรียมพร้อมสำหรับการอพยบขึ้นที่สูง",
                            /*17*/"คำแนะนำ : ควรเริ่มการอพยบขึ้นที่สูง และติดต่อเจ้าหน้าที่ โทร. 1111 กด 5",
                            /*18*/"คำแนะนำ : คู่มือรับภัยน้ำท่วม > http://www.rd.go.th/region1/fileadmin/pdf/393-54.pdf",
                            /*19*/"คำแนะนำ : เบอร์รับภัยน้ำท่วม โทรสายด่วน 1111 กด 5 ฟรีตลอด 24 ชั่วโมง. หรือ เบอร์อื่น ๆ > http://ilovekamikaze.com/news/GU8NK2KP",
                            /*20*/"อุณหภูมิขณะนี้ : " + String(tmpc, 0) + " องศาเซลเซียส",
                            /*21*/"ความชื้นในบรรยากาศ : " + String(hum, 0) + " %",
                            /*22*/"ความเข้มแสง : " + String(lux) + " lux",
                            ""
                           }; //ArduinoIDE เวอร์ชั่นใหม่ ๆ ใส่ภาษาไทยลงไปได้เลย
/*---------------------------------"rpm"---------------------------------------*/
/*---------------------------------"rpm"---------------------------------------*/
void rpm () {     //This is the function that the interupt calls

  NbTopsFan++;    //This function measures the rising and falling edge of the hall effect sensors signal
}
/*---------------------------------"Sensor"---------------------------------------*/
/*---------------------------------"Sensor"---------------------------------------*/
void SensorTask(void*) {
  VOID SETUP() {            //  เหมือน setup() แต่ใส่ชื่อเป็น SETUP() พิมพ์ใหญ่แทน
    Serial.println("SensorTask Start");
  }

  VOID LOOP() {             // เหมือน loop() แต่ใส่ชื่อเป็น LOOP() พิมพ์ใหญ่แทน

    NbTopsFan = 0;          //Set NbTops to 0 ready for calculations
    sei();                  //Enables interrupts
    DELAY (1000);           //Wait 1 second
    cli();                  //Disable interrupts

    calc = NbTopsFan;               //calc = (NbTopsFan * 60 / 7.5); //(Pulse frequency x 60) / 7.5Q, = flow rate in L/hour * 0.0167 = L/min
    rate_flow = (calc * FLOW_R);    //1 ml/sec * 0.06 = L/min   //มิลลิลิตร(ml) x 0.0308 = มิลลิเมตร(mm)
    if (rate_flow > 500) {          //30L/min = 500 ml/sec * 0.0308 = 15.4 mm/sec
      rate_flow = 500;
    }
    rain_flow += (rate_flow * 0.0308);

    if (rate_flow == 0) {
      page = 0;
      count_wait++;
      if (count_wait == 60) {
        rain_flow = 0;
      }
    }
    else {
      page = 1;
      count_wait = 0;
    }

    lux = readBH1750(BH1750_ADDR);  //อ่านค่าความเข้มแสง หน่วยเป็น LUX
    if (lux > 60000) {              //เซ็นเซอร์วัดความเข้มแสง Model: GY-302 ช่วง 0-65535 lux Power supply :3-5 V
      lux = 60000;
    }

    Serial.println(tmpNow);
    Serial.println("Rate : " +
                   String(rate_flow, 2) + " ml/sec | Total water : " +
                   String(rain_flow, 2) + " mm | Light : " +
                   String(lux) + " lux | Water level : " +
                   String(water_level, 2) + " cm");
    Serial.println("Humidity : " + String(hum, 2) + " % | Temperature : " +
                   String(tmpc, 2) + " *C | Temperature : " +
                   String(tmpf, 2) + " *F");
  }
}
/*---------------------------------"Lux BH1750"---------------------------------------*/
/*---------------------------------"Lux BH1750"---------------------------------------*/
int readBH1750(byte dev_addr) {
  Wire.beginTransmission(dev_addr);
  Wire.write(0x10);
  if (Wire.endTransmission() != 0) return -1;
  Wire.requestFrom((int)dev_addr, 2);
  return (Wire.available() >= 2) ? (Wire.read() << 8) | Wire.read() : -1;
}
/*---------------------------------"Ultrasonic"---------------------------------------*/
/*---------------------------------"Ultrasonic"---------------------------------------*/
void UltrasonicTask(void*) {  //เซ็นเซอร์วัดระยะทาง Model: Hcsr04 Detection range:2 cm to 4 m Detecting precision: 0.3 cm + 1% Power supply :5 V
  VOID SETUP() {              //  เหมือน setup() แต่ใส่ชื่อเป็น SETUP() พิมพ์ใหญ่แทน
    Serial.println("UltrasonicTask Start");
  }

  VOID LOOP() {               // เหมือน loop() แต่ใส่ชื่อเป็น LOOP() พิมพ์ใหญ่แทน

    digitalWrite(US100_Trig_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(US100_Trig_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(US100_Trig_PIN, LOW);
    duration = pulseIn(US100_Echo_PIN, HIGH);
    cm = microsecondsToCentimeters(duration);

    if (cm > 205) {                 //กำหนดความสูงของเซ็นเซอร์ 2 เมตร
      cm = 205;
    }
    water_level = water_fix - cm;   //คำนวณระดับน้ำ = ระดับความสูงของเซ็นเซอร์ กำหนด 2 เมตร - ระยะที่เซ็นเซอร์วัดได้
    DELAY(1000);
  }
}

long microsecondsToCentimeters(long microseconds)
{
  //ความเร็วเสียงในอากาศประมาณ 340 เมตร/วินาที หรือ 29 ไมโครวินาที/เซนติเมตร
  //ระยะทางที่ส่งเสียงออกไปจนเสียงสะท้อนกลับมาสามารถใช้หาระยะทางของวัตถุได้
  //เวลาที่ใช้คือ ระยะทางไปกลับ ดังนั้นระยะทางคือ ครึ่งหนึ่งของที่วัดได้
  return microseconds / 29 / 2;
}
/*---------------------------------"DHT22"---------------------------------------*/
/*---------------------------------"DHT22"---------------------------------------*/
void DHT22Task(void*) {       //เซนเซอร์วัดความชื้นและอุณหภูมิ Model: DHT22 Humidity 0 - 100% RH+-2% RH | -40 - 80 +-0.5 degrees C Power supply :3-5 V
  VOID SETUP() {              //  เหมือน setup() แต่ใส่ชื่อเป็น SETUP() พิมพ์ใหญ่แทน
    Serial.println("DHT22Task Start");
  }

  VOID LOOP() {               // เหมือน loop() แต่ใส่ชื่อเป็น LOOP() พิมพ์ใหญ่แทน

    if (!isnan(dht.readHumidity()) || !isnan(dht.readTemperature()) || !isnan(dht.readTemperature(true))) {
      hum  = dht.readHumidity();
      tmpc = dht.readTemperature();
      tmpf = dht.readTemperature(true);
    }
    // เช็คถ้าอ่านค่าไม่สำเร็จให้เริ่มอ่านใหม่
    /*if (isnan(hum) || isnan(tmpc) || isnan(tmpf)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
      }*/

    weatherlevel();

    DELAY(1000);
  }
}
/*---------------------------------"Time"---------------------------------------*/
/*---------------------------------"Time"---------------------------------------*/
void SetTimeTask(void*) {
  VOID SETUP() {              //  เหมือน setup() แต่ใส่ชื่อเป็น SETUP() พิมพ์ใหญ่แทน
    Serial.println("SetTimeTask Start");
  }

  VOID LOOP() {               // เหมือน loop() แต่ใส่ชื่อเป็น LOOP() พิมพ์ใหญ่แทน
    time_t now = time(nullptr);                   //set time internet
    struct tm* newtime = localtime(&now);

    tmpNow = "";
    if ((newtime->tm_mday >= 0) && (newtime->tm_mday <= 9)) {
      tmpNow += "0";
      tmpNow += String(newtime->tm_mday);
    }
    else {
      tmpNow += String(newtime->tm_mday);
    }
    tmpNow += "/";
    if ((newtime->tm_mon + 1 >= 0) && (newtime->tm_mon + 1 <= 9)) {
      tmpNow += "0";
      tmpNow += String(newtime->tm_mon + 1);
    }
    else {
      tmpNow += String(newtime->tm_mon + 1);
    }
    tmpNow += "/";
    tmpNow += String(newtime->tm_year + 1900);
    if ((newtime->tm_hour >= 0) && (newtime->tm_hour <= 9)) {
      tmpNow += " 0";
      tmpNow += String(newtime->tm_hour);
    }
    else {
      tmpNow += " ";
      tmpNow += String(newtime->tm_hour);
    }
    tmpNow += ":";
    if ((newtime->tm_min >= 0) && (newtime->tm_min <= 9)) {
      tmpNow += "0";
      tmpNow += String(newtime->tm_min);
    }
    else {
      tmpNow += String(newtime->tm_min);
    }
    tmpNow += ":";
    if ((newtime->tm_sec >= 0) && (newtime->tm_sec <= 9)) {
      tmpNow += "0";
      tmpNow += String(newtime->tm_sec);
    }
    else {
      tmpNow += String(newtime->tm_sec);
    }
    //data_timein[8] = |dd|mm|yyyy|hh|mm|ss|
    data_timein[0] = tmpNow.substring(0, 2).toInt();      //วัน
    data_timein[1] = tmpNow.substring(3, 5).toInt();      //เดือน
    data_timein[2] = tmpNow.substring(6, 10).toInt();     //ปี
    data_timein[3] = tmpNow.substring(11, 13).toInt();    //ชั่วโมง
    data_timein[4] = tmpNow.substring(14, 16).toInt();    //นาที
    data_timein[5] = tmpNow.substring(17, 19).toInt();    //วินาที
    /*Serial.println(String(data_timein[0]) + "/" +
                   String(data_timein[1]) + "/" +
                   String(data_timein[2]) + " " +
                   String(data_timein[3]) + ":" +
                   String(data_timein[4]) + ":" +
                   String(data_timein[5]));*/
    DELAY(1000);
  }
}
/*---------------------------------"SETUP"---------------------------------------*/
/*---------------------------------"SETUP"---------------------------------------*/
void send_json(String data) {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["ESP"] = data;

  JsonArray& values = JSONencoder.createNestedArray("values"); //JSON array

  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);

  HTTPClient http;    //Declare object of class HTTPClient

  http.begin(host);      //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header

  int httpCode = http.POST(JSONmessageBuffer);   //Send the request
  String payload = http.getString();                                        //Get the response payload

  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload

  http.end();  //Close connection
}
/*---------------------------------"onMsghandler"---------------------------------------*/
/*---------------------------------"onMsghandler"---------------------------------------*/
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  char *ms = (char *)msg;
  Serial.print("Incoming message -->");
  msg[msglen] = '\0';
  msg_st = ms;
  Serial.println(msg_st);
  if ((msg_st == "สภาพอากาศ") || (msg_st == "Weather")) {
    //microgear.chat(TargetWeb,"1");
    //send_data("ESP_LED_ON");
    weathershow(weather_now);
    watershow(water_now);
    msg_st == "";
  }
  if ((msg_st == "อุณหภูมิ") || (msg_st == "Temperature")) {
    send_json("อุณหภูมิ : " + String(tmpc, 0) + " องศาเซลเซียส");
    msg_st == "";
  }
  if ((msg_st == "ความชื้น") || (msg_st == "Humidity")) {
    send_json("ความชื้น : " + String(hum, 0) + " %");
    msg_st == "";
  }
  if ((msg_st == "ความเข้มแสง") || (msg_st == "Light")) {
    send_json("ความเข้มแสง : " + String(lux) + " lux");
    msg_st == "";
  }
  if ((msg_st == "ปริมาณน้ำฝน") || (msg_st == "Rain")) {
    send_json("ปริมาณน้ำฝน : " + String(rain_flow) + " mm");
    msg_st == "";
  }
  if ((msg_st == "ระดับน้ำ") || (msg_st == "Water")) {
    send_json("ระดับน้ำ : " + String(water_level) + " cm");
    msg_st == "";
  }
  if ((msg_st == "คู่มือ") || (msg_st == "คำแนะนำ")) {
    send_json("คู่มือรับภัยน้ำท่วม > http://www.rd.go.th/region1/fileadmin/pdf/393-54.pdf");
    msg_st == "";
  }
  if ((msg_st == "หมายเลข") || (msg_st == "โทร") || (msg_st == "Phone")) {
    send_json("เบอร์รับภัยน้ำท่วม โทรสายด่วน 1111 กด 5 ฟรีตลอด 24 ชั่วโมง. หรือ เบอร์อื่น ๆ > http://ilovekamikaze.com/news/GU8NK2KP");
    msg_st == "";
  }
}
/*---------------------------------"onConnected"---------------------------------------*/
/*---------------------------------"onConnected"---------------------------------------*/
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  lcd.setCursor(0, 0);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0
  lcd.print("NETPIE Connecting");
  //lcd.clear(); // ล้างหน้าจอ
  count_wait = 0;
  while (!microgear.connected()) {
    Serial.print(".");
    if (count_wait % 2 == 0) {
      digitalWrite(RLED_PIN, HIGH); // Pin D2 is HIGH
    } else {
      digitalWrite(RLED_PIN, LOW); // Pin D2 is LOW
    }
    count_wait++;
    delay(1000);
    if (count_wait == 20) {
      Serial.println();
      Serial.println("Esp can't connect NETPIE");
      Serial.println("Esp Restart");
      ESP.restart(); //ESP.reset();
    }
  }
  digitalWrite(RLED_PIN, LOW); // Pin D2 is LOW
  digitalWrite(GLED_PIN, HIGH); // Pin D4 is HIGH
  delay(100);
  digitalWrite(GLED_PIN, LOW); // Pin D4 is LOW
  delay(100);
  digitalWrite(GLED_PIN, HIGH); // Pin D4 is HIGH
  delay(100);
  digitalWrite(GLED_PIN, LOW); // Pin D4 is LOW
  count_wait = 0;
  microgear.setName(ALIAS);
  lcd.clear(); // ล้างหน้าจอ
}
/*---------------------------------"SETUP"---------------------------------------*/
/*---------------------------------"SETUP"---------------------------------------*/
void setup() {
  Serial.begin(115200);
  pinMode(FLOW_PIN, INPUT);
  attachInterrupt(FLOW_PIN, rpm, RISING);
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(GLED_PIN, OUTPUT);
  pinMode(RLED_PIN, OUTPUT);
  pinMode(US100_Trig_PIN, OUTPUT);
  pinMode(US100_Echo_PIN, INPUT);

  Serial.println("Light Alert OFF");
  digitalWrite(TRIAC_PIN, LOW); // Pin D0 is LOW
  digitalWrite(GLED_PIN, LOW);  // Pin D5 is LOW
  digitalWrite(RLED_PIN, LOW);  // Pin D6 is LOW

  microgear.on(MESSAGE, onMsghandler);
  microgear.on(CONNECTED, onConnected);

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin();
  lcd.backlight();            // เปิดไฟ backlight

  WiFi.mode(WIFI_STA); // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi Connecting");
  lcd.setCursor(0, 0);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0
  lcd.print("WiFi Connecting");
  //lcd.clear(); // ล้างหน้าจอ
  count_wait = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (count_wait % 2 == 0) {
      digitalWrite(RLED_PIN, HIGH); // Pin D2 is HIGH
    } else {
      digitalWrite(RLED_PIN, LOW); // Pin D2 is LOW
    }
    count_wait++;
    delay(1000);
    if (count_wait == 20) {
      Serial.println();
      Serial.println("Esp can't connect WiFi");
      Serial.println("Esp Restart");
      ESP.restart(); //ESP.reset();
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Esp Online");
    lcd.setCursor(0, 1);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1
    lcd.print("Esp Online");
    delay(2000);
    lcd.clear(); // ล้างหน้าจอ
    digitalWrite(RLED_PIN, LOW); // Pin D2 is LOW
    digitalWrite(GLED_PIN, HIGH); // Pin D4 is HIGH
    delay(100);
    digitalWrite(GLED_PIN, LOW); // Pin D4 is LOW
    delay(100);
    digitalWrite(GLED_PIN, HIGH); // Pin D4 is HIGH
    delay(100);
    digitalWrite(GLED_PIN, LOW); // Pin D4 is LOW

    Serial.println("Wifi Success");
    Serial.print("Wifi Name : ");
    Serial.println(WIFI_SSID);
    Serial.print("Pass Name : ");
    Serial.println(WIFI_PASSWORD);
    Serial.print("Local IP : ");
    Serial.println(WiFi.localIP());

    microgear.init(KEY, SECRET, ALIAS);
    microgear.connect(APPID);

    configTime(TIMEZONE * 3600, dst, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for time...");
    lcd.setCursor(0, 0);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0
    lcd.print("Time Connecting");
    //lcd.clear(); // ล้างหน้าจอ
    count_wait = 0;
    while (!time(nullptr)) {
      Serial.print(".");
      if (count_wait % 2 == 0) {
        digitalWrite(RLED_PIN, HIGH); // Pin D2 is HIGH
      } else {
        digitalWrite(RLED_PIN, LOW); // Pin D2 is LOW
      }
      count_wait++;
      delay(1000);
      if (count_wait == 20) {
        Serial.println();
        Serial.println("Esp can't connect Time");
        lcd.setCursor(0, 1);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1
        lcd.print("Can't Connect");
        //lcd.clear(); // ล้างหน้าจอ
        delay(2000);
        Serial.println("Esp Restart");
        ESP.restart(); //ESP.reset();
      }
    }
    count_wait = 0;
    Serial.println();
    Serial.println("Time Connected");
    lcd.setCursor(0, 1);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1
    lcd.print("Time Connected");
    delay(2000);
    lcd.clear(); // ล้างหน้าจอ
    digitalWrite(RLED_PIN, LOW); // Pin D6 is LOW
    digitalWrite(GLED_PIN, HIGH); // Pin D5 is HIGH
  }

  dht.begin();                // เซ็นเซอร์ DHT22 เริ่มทำงาน
  for (int i = 3 ; i > 0 ; i--) {
    Serial.println("Wait Line " + String(i) + " s");
    lcd.setCursor(0, 0);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0
    lcd.print("Wait Sensor " + String(i) + " s");
    delay(1000);
  }
  lcd.clear(); // ล้างหน้าจอ

  Serial.println();
  Serial.println("Welcome");
  lcd.setCursor(0, 0);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0
  lcd.print("    Welcome   ");
  lcd.setCursor(0, 1);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1
  lcd.print("   Line Alert  ");
  delay(2000);
  lcd.clear(); // ล้างหน้าจอ
  Serial.println("ส่งข้อความ : เริ่มต้นระบบ");
  send_json("เริ่มต้นระบบเตือนภัยน้ำท่วม");
  Serial.println("ส่งข้อความ : คำแนะนำ");
  send_json("คำแนะนำ : เบอร์รับภัยน้ำท่วม โทรสายด่วน 1111 กด 5 ฟรีตลอด 24 ชั่วโมง. หรือ เบอร์อื่น ๆ > http://ilovekamikaze.com/news/GU8NK2KP");

  Sensor_Task.start(SensorTask);
  Ultrasonic_Task.start(UltrasonicTask);
  DHT22_Task.start(DHT22Task);
  SetTime_Task.start(SetTimeTask);
}
/*---------------------------------"LCD"---------------------------------------*/
/*---------------------------------"LCD"---------------------------------------*/
void lcdshow () {
  switch (page) {
    case 0 :
      lcd.setCursor(0, 0);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0
      lcd.print(String(data_timein[3]) + ":" +
                String(data_timein[4]) + ":" +
                String(data_timein[5]) + " T:" + String(tmpc, 0) + "*C");
      lcd.setCursor(0, 1);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1
      lcd.print("H:" + String(hum, 0) + "%" +
                " L:" + String(lux) + "lux");
      break;
    case 1 :
      lcd.setCursor(0, 0);       // ไปที่ตัวอักษรที่ 9 บรรทัดที่ 0
      lcd.print("Rain:" + String(rain_flow, 0) + "mm");
      lcd.setCursor(0, 1);        // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1
      lcd.print("Water :" + String(water_level, 0) + "cm");
      break;

    default :
      Serial.println("No LCD");
      break;
  }
}
/*---------------------------------"Water"---------------------------------------*/
/*---------------------------------"Water"---------------------------------------*/
void waterlevel() {
  if ((water_level > 0) && (water_level <= 5)) {
    water_now = 1;  //ปกติ
  }
  if ((water_level >= 6) && (water_level <= 10)) {
    water_now = 2;  //น้ำท่วมเท้า
  }
  if ((water_level >= 15) && (water_level <= 20)) {
    water_now = 3;  //น้ำท่วมหน้าแข้ง
  }
  if ((water_level >= 30) && (water_level <= 40)) {
    water_now = 4;  //น้ำท่วมเข่า
  }
  if ((water_level >= 70) && (water_level <= 80)) {
    water_now = 5;  //น้ำท่วมเอว
  }
  if ((water_level >= 100) && (water_level <= 120)) {
    water_now = 6;  //น้ำท่วมคอ
  }
  if (water_level >= 160) {
    water_now = 7;  //น้ำท่วมหัว
  }
}
/*---------------------------------"Weather"---------------------------------------*/
/*---------------------------------"Weather"---------------------------------------*/
void weatherlevel() {
  if (!isnan(hum) || !isnan(tmpc) || !isnan(tmpf)) {
    if ((rate_flow == 0) && (rain_flow == 0)) {
      if ((tmpc >= 25) && (tmpc <= 30) && (hum >= 30) && (hum <= 60)) {
        weather_now = 1;    //ปกติ
      }
      if ((tmpc > 30) && (hum < 50)) {
        weather_now = 2;    //ร้อน
      }
      if ((tmpc < 25) && (hum >= 30) && (hum <= 60)) {
        weather_now = 3;    //หนาว
      }
    }

    if ((tmpc < 35) && (hum > 60)) {
      if ((rain_flow > 0) && (rain_flow <= 5.2)) {    //Max 15.4 mm/sec
        weather_now = 4;
      }
      if ((rain_flow > 5.3) && (rain_flow <= 10.4)) {
        weather_now = 5;
      }
      if (rain_flow > 10.4) {
        weather_now = 6;
      }
    }
  }
}
/*---------------------------------"WeatherShow"---------------------------------------*/
/*---------------------------------"WeatherShow"---------------------------------------*/
void weathershow(int weather) {
  switch (weather) {
    case 1 : //อากาศปกติ
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : สภาพอากาศปกติ");
      send_json("สภาพอากาศ ปกติ อุณหภูมิ : " + String(tmpc, 0) +
                " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                " % ความเข้มแสง : " + String(lux) + " lux");
      weather_old = weather_now;
      break;
    case 2 :  //อากาศร้อน
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : สภาพอากาศร้อน");
      send_json("สภาพอากาศ ร้อน อุณหภูมิ : " + String(tmpc, 0) +
                " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                " % ความเข้มแสง : " + String(lux) + " lux");
      weather_old = weather_now;
      break;
    case 3 :  //อากาศหนาว
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : สภาพอากาศหนาว");
      send_json("สภาพอากาศ หนาว อุณหภูมิ : " + String(tmpc, 0) +
                " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                " % ความเข้มแสง : " + String(lux) + " lux");
      weather_old = weather_now;
      break;
    case 4 :  //ฝนตก
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ฝนตก");
      send_json("สภาพอากาศ ฝนตก อุณหภูมิ : " + String(tmpc, 0) +
                " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                " % ความเข้มแสง : " + String(lux) + " lux");
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow));
      weather_old = weather_now;
      break;
    case 5 :  //ฝนตกหนัก
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ฝนตกหนัก");
      send_json("สภาพอากาศ ฝนตกหนัก อุณหภูมิ : " + String(tmpc, 0) +
                " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                " % ความเข้มแสง : " + String(lux) + " lux");
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow));
      weather_old = weather_now;
      break;
    case 6 :  //ฝนตกหนักมาก
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ฝนตกหนักมาก");
      send_json("สภาพอากาศ ฝนตกหนักมาก อุณหภูมิ : " + String(tmpc, 0) +
                " องศาเซลเซียส ความชื้น : " + String(hum, 0) +
                " % ความเข้มแสง : " + String(lux) + " lux");
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow));
      weather_old = weather_now;
      break;
    default :
      Serial.println("No Weather");
      break;
  }
}
/*---------------------------------"WaterShow"---------------------------------------*/
/*---------------------------------"WaterShow"---------------------------------------*/
void watershow(int water) {
  switch (water) {
    case 1 : //ระดับน้ำปกติ
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : ระดับน้ำปกติ");
      send_json("ระดับน้ำปกติ 0 - 5 เซนติเมตร ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : ระดับน้ำที่รถทั่วไปสามารถขับผ่านได้คือ น้ำไหล สูงไม่เกิน 4 นิ้ว เท่ากับ 10.16 เซนติเมตร");
      water_old = water_now;
      break;
    case 2 :  //น้ำท่วมเท้า
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : น้ำท่วมเท้า");
      send_json("น้ำท่วมเท้า 6 - 10 cm ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : ระดับน้ำที่รถทั่วไปสามารถขับผ่านได้คือ น้ำนิ่ง สูงไม่เกิน 6 นิ้ว เท่ากับ 15.24 เซนติเมตร");
      water_old = water_now;
      break;
    case 3 :  //น้ำท่วมหน้าแข้ง
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : น้ำท่วมหน้าแข้ง");
      send_json("น้ำท่วมหน้าแข้ง 15 - 20 cm ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : ควรเฝ้าระวังระดับน้ำ และ ขนของขึ้นที่สูง");
      water_old = water_now;
      break;
    case 4 :  //น้ำท่วมเข่า
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : น้ำท่วมเข่า");
      send_json("น้ำท่วมเข่า 30 - 40 cm ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : ควรเตรียมพร้อมสำหรับการอพยบขึ้นที่สูง");
      Serial.println("ส่งข้อความ : คู่มือ");
      send_json("คำแนะนำ : คู่มือรับภัยน้ำท่วม > http://www.rd.go.th/region1/fileadmin/pdf/393-54.pdf");
      water_old = water_now;
      break;
    case 5 :  //น้ำท่วมเอว
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : น้ำท่วมเข่า");
      send_json("น้ำท่วมเอว 70 - 80 cm ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : ควรเริ่มการอพยบขึ้นที่สูงอย่างระมัดระวัง และติดต่อเจ้าหน้าที่ โทร. 1111 กด 5");
      water_old = water_now;
      break;
    case 6 :  //น้ำท่วมอก
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : น้ำท่วมอก");
      send_json("น้ำท่วมอก 100 - 120 cm ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : เบอร์รับภัยน้ำท่วม โทรสายด่วน 1111 กด 5 ฟรีตลอด 24 ชั่วโมง. หรือ เบอร์อื่น ๆ > http://ilovekamikaze.com/news/GU8NK2KP");
      water_old = water_now;
      break;
    case 7 :  //น้ำท่วมหัว
      //Serial.println("ส่งข้อความ : เวลาปัจจุบัน");
      //send_json("วันเวลาปัจจุบัน " + String(tmpNow));
      Serial.println("ส่งข้อความ : ปริมาณฝน");
      send_json("ปริมาณฝน " + String(rain_flow) + " mm");
      Serial.println("ส่งข้อความ : น้ำท่วมหัว");
      send_json("น้ำท่วมหัว มากกว่า 160 cm ความสูงของน้ำ : " + String(water_level) + " cm");
      Serial.println("ส่งข้อความ : คำแนะนำ");
      send_json("คำแนะนำ : เบอร์รับภัยน้ำท่วม โทรสายด่วน 1111 กด 5 ฟรีตลอด 24 ชั่วโมง. หรือ เบอร์อื่น ๆ > http://ilovekamikaze.com/news/GU8NK2KP");
      water_old = water_now;
      break;
    default :
      Serial.println("No Weather");
      break;
  }
}
/*---------------------------------"LOOP"---------------------------------------*/
/*---------------------------------"LOOP"---------------------------------------*/
void loop() {
  /*
    ----------------------------------------------------------------สภาพอากาศ
    อากาศร้อนจัด    อุณหภูมิมากกว่า 35 องศาเซลเซียสขึ้นไป
    อากาศร้อน      อุณหภูมิ 30 - 34 องศาเซลเซียส
    อากาศปกติ      อุณหภูมิ 24 - 29 องศาเซลเซียส
    อากาศเย็น       อุณหภูมิ 16 - 23 องศาเซลเซียส
    อากาศหนาว     อุณหภูมิ 8 - 15 องศาเซลเซียส
    อากาศหนาวจัด   อุณหภูมิน้อยกว่า 8 องศาเซลเซียส
    ฝนตก          ปริมาณแสง เช้า      lux ความชื้น  % อุณหภูมิ  *C
                   ปริมาณแสง กลางวัน   lux ความชื้น  % อุณหภูมิ  *C
                   ปริมาณแสง เย็น      lux ความชื้น  % อุณหภูมิ  *C
    ----------------------------------------------------------------ปริมาณน้ำฝน
    ฝนตกหนักมาก     ปริมาณฝนต่อวัน มากกว่า 90.1 มิลลิเมตรขึ้นไป
    ฝนตกหนัก        ปริมาณฝนต่อวัน ตั้งแต่ 35.1 - 90.0 มิลลิเมตร
    ฝนตก            ปริมาณฝนต่อวัน ตั้งแต่ 10.1 - 35.0 มิลลิเมตร
    ฝนตกน้อย        ปริมาณฝนต่อวัน ตั้งแต่ 0.10 - 10.0 มิลลิเมตร
    ฝนไม่ตก         ปริมาณฝนต่อวัน น้อยกว่า 0.10 มิลลิเมตร
    ----------------------------------------------------------------ระดับน้ำ
    ไม่มีน้ำท่วม          0 cm
    น้ำท่วมเท้า          10 cm
    น้ำท่วมหน้าแข้ง      20 cm
    น้ำท่วมเข่า          40 cm
    น้ำท่วมเอว          80 cm
    น้ำท่วมอก         120 cm
    น้ำท่วมหัว         160 cm
    คู่มือรับภัยน้ำท่วม > http://www.rd.go.th/region1/fileadmin/pdf/393-54.pdf
    ระดับน้ำที่รถทั่วไปสามารถขับผ่านได้คือ น้ำนิ่ง สูงไม่เกิน 6 นิ้ว เท่ากับ 15.24 เซนติเมตร
    ระดับน้ำที่รถทั่วไปสามารถขับผ่านได้คือ น้ำไหล สูงไม่เกิน 4 นิ้ว เท่ากับ 10.16 เซนติเมตร
    ----------------------------------------------------------------ตัวอย่างส่งไลน์
    send_json("ข้อความ");              //ข้อความ
    send_json(2342);                  //จำนวนเต็ม
    send_json(212.43434,5);           //จำนวนจริง แสดง 5 หลัก
    send_jsonSticker("ข้อความ",1 ,2);  //ส่ง Line Sticker ด้วย PackageID 1 , StickerID 2  พร้อมข้อความ
    เลือก Line Sticker ได้จาก https://devdocs.line.me/files/sticker_list.pdf
    send_jsonPicture("URL.jpg");
    send_jsonPicture("ข้อความ","URL.jpg");
  */

  if (weather_now != weather_old) {   //ตรวจสภาพอากาศ
    weathershow(weather_now);
  }

  if (water_level > 0) {    //กรณีมีน้ำท่วม
    //waterlevel();
    if (water_now != water_old) {
      watershow(water_now);
    }
  }

  if ((data_timein[4] == 0) && (data_timein[5] == 0)) {    //ช่วงการทำงาน
    weathershow(weather_now);
    watershow(water_now);
  }

  if ((microgear.connected()) && (WiFi.status() == WL_CONNECTED)) {
    microgear.loop();
  }
  else {  //กรณีขาดการเชื่อมต่อจาก NETPIE หรือ WiFi
    Serial.println("ขาดการเชื่อมต่อ");
    digitalWrite(GLED_PIN, LOW); // Pin D4 is LOW
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    count_wait = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      if (count_wait % 2 == 0) {
        digitalWrite(RLED_PIN, HIGH); // Pin D2 is HIGH
      } else {
        digitalWrite(RLED_PIN, LOW); // Pin D2 is LOW
      }
      count_wait++;
      delay(1000);
      if (count_wait == 20) {
        Serial.println();
        Serial.println("Esp can't connect WiFi");
        Serial.println("Esp Restart");
        ESP.restart(); //ESP.reset();
        break;
      }
    }
    microgear.connect(APPID);
    digitalWrite(GLED_PIN, HIGH); // Pin D4 is LOW
  }

  lcdshow ();
  delay(1000);
  lcd.clear(); // ล้างหน้าจอ
}
