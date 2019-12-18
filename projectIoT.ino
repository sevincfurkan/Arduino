#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Wire.h>//I2C kullanılacağı için "Wire" kütüphanesi programa dahil ediliyor.
#include <Adafruit_SSD1306.h>//SSD1306 için kullanılacak Adafruit kütüphanesi dahil ediliyor.
#include <Servo.h>//servo motor için kullanılacak kütüphane dahil ediliyor

#define SCREEN_WIDTH 128 // OLED display genişlik,pixels
#define SCREEN_HEIGHT 64 // OLED display yükseklik,pixels

#define WIFI_SSID "OPPO AX7"  // wifi adı
#define WIFI_PASS "6390elaziz"     // wifi şifresi

#define MQTT_SERV "io.adafruit.com"   // adafruit url si
#define MQTT_PORT 1883  // çalıştıralacak port
#define MQTT_NAME "IoTiF"   // adafruit kullanıcı adı
#define MQTT_PASS "b70fd3d94ff44e1fad89a7230f4cabce"  //adafruit active key

WiFiClient client; //client nesnesi oluşturuldu
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS); //mqtt nesnesi oluşturuldu

Adafruit_MQTT_Subscribe light = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/light"); //light feed'i ile light nesnesi oluşturuldu
Adafruit_MQTT_Subscribe servo = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/servo"); //servo feed'i ile servo nesnesi oluşturuldu
Adafruit_MQTT_Subscribe temp = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/temp"); //temp feed'i ile temp nesnesi oluşturuldu

Servo motor;  // servo motoru kontrol etmek için bir nesne oluştur

Adafruit_SSD1306 ekran(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);//Displayin reset pini olmadığından '-1' yazıyoruz
float sicaklik;       // sıcaklık bilgisini tutacak olan değişken
void setup()
{
  Serial.begin(115200); //Seri iletişim hızı 9600 bps olarak ayarlanarak başlatılıyor.
  
  //Connect to WiFi
  Serial.print("\n\nConnecting Wifi>"); // serial ekranına bu yazıyı yazıyor
  WiFi.begin(WIFI_SSID, WIFI_PASS); // wifi adını ve şifresini giriyor
  

  while (WiFi.status() != WL_CONNECTED) //
  {                                     //
    Serial.print(">");                  //wifi ya bağlanana kadar 50 ms bir bu döngü çalışır
    delay(50);                          //
  }                                     //

  Serial.println("OK!");                // wifi ya bağlanıldığı zaman serial ekranında OK! yazar

  //Subscribe to the feed topic
  mqtt.subscribe(&light); //
  mqtt.subscribe(&servo); // feedlerimize abone oluyoruz.Yani değerleri gönderebiliyoruz 
  mqtt.subscribe(&temp);  //

  pinMode(D7, OUTPUT);  //D7 pini çıkış olarak belirtiliyor
  digitalWrite(D7, LOW);

  ekran.begin(SSD1306_SWITCHCAPVCC, 0x3C);  //Display başlatılıyor ve adresi belirtiliyor (0x3c)
  motor.attach(D4);  // servo D4 pinine bağlı
}

void loop()
{
  //Connect/Reconnect to MQTT
  MQTT_connect();

  //Read from our subscription queue until we run out, or
  //wait up to 5 seconds for subscription to update
  Adafruit_MQTT_Subscribe * subscription; // subscription nesnesi oluşturuluyor
  while ((subscription = mqtt.readSubscription(5000))) // bitene kadar abonelik okunur veya güncellenmesi için 5 ssaniye beklenir
  {
    //If we're in here, a subscription updated...
    if (subscription == &light) //eğer light feed'i okunursa
    {
      //Print the new value to the serial monitor
      Serial.print("light: ");                  //
      Serial.println((char*) light.lastread);   //serial ekrana light: ve light değeri yazılır

      //If the new value is  "ON", turn the light on.
      //Otherwise, turn it off.
      if (!strcmp((char*) light.lastread, "ON")) // okunan değer ON olursa  D7 ledi yanar
      {
        //active low logic
        digitalWrite(D7, HIGH);
      }
      else if (!strcmp((char*) light.lastread, "OFF")) // okunan değer OFF olursa D7 ledi söner
      {
        digitalWrite(D7, LOW);

      }
    }
    if(subscription == &servo){  //eğer servo feed'i okunursa
      Serial.print("Slider(servo): ");        //
      Serial.println((char *)servo.lastread); //serial ekrana Slider(servo): ve servo değeri yazılır
      
      int sliderval = atoi((char *)servo.lastread); //servo feedinden okunan char değeri integer'a çevirir
      Serial.println(sliderval);
      motor.write(sliderval);   //motorun açısını belirle 
      delay(500);               // 500 ms bekle motor o açıya geçsin
      Serial.print("servo motor döndü"); //serial ekrana servo motor döndü yazılır
    }

    if(subscription == &temp){   //eğer temp feed'i okunursa
      Serial.print("temp: ");                 //
      Serial.println((char*) temp.lastread);  // serial ekrana temp: ve temp değeri yazılır

      if (!strcmp((char*) temp.lastread, "ON")) // okunan değer ON olursa  temparature ile ölçülen değer ekrana yazılır
      {
        ekran.clearDisplay();                     //Display temizleniyor 
        ekran.setTextSize(2);                     // Yazı boyutu ayarlanıyor
        ekran.setTextColor(WHITE);                // Yazı rengi ayarlanıyor. (white veya black)
        ekran.setCursor(0,0);                     // İmleç sol üst köşeye alınıyor.
        ekran.println("SICAKLIK:");               // Yazımız ekrana gönderiliyor ve bir alt satıra geçiliyor.
      
        sicaklik=sicaklik_olc();                  // sıcaklık ölçme fonksiyonu çağırılıyor
        ekran.setCursor(30,30);                   // İmleç pozisyonu ayarlanıyor.
        
        ekran.setTextSize(3);                     // Yazı boyutu büyütlüyor.(15x21)
        ekran.print(sicaklik,1);                  // Sıcaklık değeri virgülden sonra 1 hane olacak şekilde ekrana yazdırılıyor.  
        ekran.setTextSize(1);                     // Yazı boyutu derece "°" sembolü yapabilmek içinküçültülüyor.
        ekran.write('o');                         // "o" harfi kullanılarak derece sembolü oluşturuluyor.
        ekran.display();                          // ekran hafızasına gönderilen değerler yansıtılıyor.

        Serial.print("sıcaklık ölçüldü");       // serial ekrana sıcaklık ölçüldü yazılır
      }
      else if (!strcmp((char*) temp.lastread, "OFF")) // okunan değer OFF olursa  temparature ile ölçülen değer ekrana yazılmaz
      {
        ekran.clearDisplay();                     //Display temizleniyor 
        ekran.setTextSize(2);                     // Yazı boyutu ayarlanıyor
        ekran.setTextColor(WHITE);                // Yazı rengi ayarlanıyor. (white veya black)
        ekran.setCursor(0,0);                     // İmleç sol üst köşeye alınıyor.
        ekran.println("KAPALI");                  // ekranda KAPALI yazılır
        Serial.print("temp kapatıldı");           // serial ekranda temp kapatıldı yazılır
      }
    }
  }
}


//MQTT sunucusuna gerektiğinde bağlanıp yeniden bağlanabilme işlevi.
void MQTT_connect()
{

  //Zaten bağlıysa durdurun.
  if (mqtt.connected() && mqtt.ping())
  {
    return;
  }

  int8_t ret; // bu veri tipi temel olarak Arduinodaki byte ile aynıdır.8 bit dir

  mqtt.disconnect(); // bağlantı kesilir

  Serial.print("Connecting to MQTT... "); //serial ekranına Connecting to MQTT... yazılır
  uint8_t deneme = 3;   //bu veri tipi temel olarak Arduinodaki byte ile aynıdır.8 bit dir
  while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
  {
    Serial.println(mqtt.connectErrorString(ret));  //serial ekranına hata mesajı gönderilir
    Serial.println("Retrying MQTT connection in 5 seconds..."); // serial ekrana Retrying MQTT connection in 5 seconds... yazılır
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    deneme--;  //her bağlantı kesilmesinde deneme 1 azalır
    if (deneme == 0) //deneme 0 olduğu zaman
    {
      ESP.reset();  //ESP resetlenir
    }
  }
  Serial.println("MQTT Connected!");// bağlantı sağlanırsa serial ekrana MQTT Connected! yazılır
}
float sicaklik_olc()
{
  float temp;
  // Ölçülecek analog büyüklüğün gürültülerinin azaltılması için 
  // en basit yöntem olan ortalama alma yöntemi kullanılıyor.
  unsigned int toplam=0,ortalama=0; //gerekli değişkenler tanımlanıyor.
  for(byte i=0;i<50;i++)             //50 adet örnek alınıyor.
  {
    toplam+=analogRead(A0);      // Alınan örnekler toplanıyor.               
  }
  ortalama=toplam/50;                //ölçülern verilerin tooplamı 50 ye bölünerek ortalama bulunuyor.
  temp=ortalama*0.09765625;         // Voltaj bilgisi sıcaklık bilgisine dönüştürülüyor.
  return temp;
}
