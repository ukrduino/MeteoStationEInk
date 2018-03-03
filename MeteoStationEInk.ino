#include "WifiCredentials.h"
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.cpp> 
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
#include <DHTesp.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "Credentials.h"
#include "Images.h"
#include <imglib\gridicons_arrow_up.h>
#include <imglib\gridicons_arrow_down.h>
#include <imglib\gridicons_arrow_left.h>
#include <imglib\gridicons_house.h>


const char* mqtt_server = MQTT_SERVER_IP;
WiFiClient espClient;
unsigned long reconnectionPeriod = 60000;
unsigned long lastBrokerConnectionAttempt = 0;
unsigned long lastWifiConnectionAttempt = 0;


PubSubClient client(espClient);
char msg[50];

// arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxIO_Class io(SPI, SS, 0, 2);
GxEPD_Class display(io);

// Pin number for DHT22 data pin
int dhtPin = 16;
DHTesp dht;
float h = 0;
float t = 0;
float outTemp = 0;
float outTemp_prev = 0;
unsigned long lastGetSensorData = 0;
int getSensorDataPeriod = 15000;
String iconId = "";
String windSpeed = "";


void setup()
{
	Serial.begin(115200);
	display.init();
	display.fillScreen(GxEPD_WHITE);
	display.update();
	dht.setup(dhtPin);
	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);
	setup_wifi();
	drawDisplay();
}

void loop()
{		
	if (!client.connected()) {
		reconnectToBroker();
	}
	client.loop();
	getDTHSensorData();
}

//Connection to MQTT broker
void connectToBroker() {
	Serial.print("Attempting MQTT connection...");
	// Attempt to connect
	if (client.connect("WemosD1Mini_1")) {
		Serial.println("connected");
		// Once connected, publish an announcement...
		client.publish("WemosD1Mini_1/status", "WemosD1Mini_1 connected");
		client.publish("Weather/update", "1");
		// ... and resubscribe
		client.subscribe("WittyCloud/temp");
		client.subscribe("Wheather/showIcon");
		client.subscribe("Wheather/showWind");
	}
	else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.println(" try again in 5 minutes");
	}
}

void setup_wifi() {

	delay(500);
	// We start by connecting to a WiFi network
	int numberOfNetworks = WiFi.scanNetworks();

	for (int i = 0; i < numberOfNetworks; i++) {
		Serial.print("Network name: ");
		Serial.println(WiFi.SSID(i));
		if (WiFi.SSID(i).equals(SSID_1))
		{
			Serial.print("Connecting to ");
			Serial.println(SSID_1);
			WiFi.begin(SSID_1, PASSWORD_1);
			delay(1000);
			connectToBroker();
			return;
		}
		else if (WiFi.SSID(i).equals(SSID_2))
		{
			Serial.print("Connecting to ");
			Serial.println(SSID_2);
			WiFi.begin(SSID_2, PASSWORD_2);
			delay(1000);
			connectToBroker();
			return;
		}
		else if (WiFi.SSID(i).equals(SSID_3))
		{
			Serial.print("Connecting to ");
			Serial.println(SSID_3);
			WiFi.begin(SSID_3, PASSWORD_3);
			delay(1000);
			connectToBroker();
			return;
		}
		else
		{
			Serial.println("Can't connect to " + WiFi.SSID(i));
		}
	}
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println("-----");
	if (strcmp(topic, "WittyCloud/temp") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		float outT = atof(buffer);
		if (fabs(outTemp - outT) > 0.1)
		{
			outTemp_prev = outTemp;
			outTemp = outT;
			drawDisplay();
		}
	}
	if (strcmp(topic, "Wheather/showIcon") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		String icon = String(buffer);
		if (!iconId.equals(icon))
		{
			iconId = icon;
			drawDisplay();
		}
	}
	if (strcmp(topic, "Wheather/showWind") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		String speed = String(buffer);
		if (!windSpeed.equals(speed))
		{
			windSpeed = speed;
			drawDisplay();
		}
	}
}



void reconnectWifi() {
	long now = millis();
	if (now - lastWifiConnectionAttempt > reconnectionPeriod) {
		lastWifiConnectionAttempt = now;
		setup_wifi();
	}
}

void reconnectToBroker() {
	long now = millis();
	if (now - lastBrokerConnectionAttempt > reconnectionPeriod) {
		lastBrokerConnectionAttempt = now;
		{
			if (WiFi.status() == WL_CONNECTED)
			{
				if (!client.connected()) {
					connectToBroker();
				}
			}
			else
			{
				reconnectWifi();
			}
		}
	}
}


void drawDisplay()
{
	display.fillScreen(GxEPD_WHITE);
	display.setFont(&FreeMonoBold18pt7b);
	display.setTextColor(GxEPD_BLACK);
	display.drawBitmap(Olga_Sergey, 0, 100, 300, 200, GxEPD_BLACK);
	display.fillRect(300, 0, 3, 300, GxEPD_BLACK);
	display.fillRect(0, 100, 400, 3, GxEPD_BLACK);
	display.fillRect(100, 0, 3, 100, GxEPD_BLACK);
	display.fillRect(200, 0, 3, 100, GxEPD_BLACK);
	display.fillRect(300, 100, 100, 3, GxEPD_BLACK);
	display.fillRect(300, 200, 100, 3, GxEPD_BLACK);
	display.drawBitmap(drop_image, 310, 20, 20, 31, GxEPD_BLACK);
	display.drawBitmap(gridicons_house, 350, 25, 24, 24, GxEPD_WHITE);
	display.drawBitmap(out_temp_icon, 203, 15, 50, 50, GxEPD_BLACK);
	if (outTemp > outTemp_prev)
	{
		display.drawBitmap(gridicons_arrow_up, 260, 25, 24, 24, GxEPD_WHITE);
	}
	else if (outTemp < outTemp_prev)
	{
		display.drawBitmap(gridicons_arrow_down, 260, 25, 24, 24, GxEPD_WHITE);
	}
	else
	{
		display.drawBitmap(gridicons_arrow_left, 260, 25, 24, 24, GxEPD_WHITE);
	}
	if (WiFi.status() != WL_CONNECTED)
	{
		display.drawBitmap(no_wifi, 3, 50, 40, 40, GxEPD_BLACK);
	}
	if (WiFi.status() == WL_CONNECTED)
	{
		display.drawBitmap(wifi, 5, 50, 40, 40, GxEPD_BLACK);
		display.setFont(&FreeMonoBold9pt7b);
		display.setCursor(3, 98);
		display.print(WiFi.SSID());
		display.setFont(&FreeMonoBold18pt7b);
	}
	if (client.connected())
	{
		display.drawBitmap(server, 55, 55, 30, 30, GxEPD_BLACK);
	}
	display.drawBitmap(thermo_image, 303, 110, 43, 50, GxEPD_BLACK);
	display.drawBitmap(gridicons_house, 350, 125, 24, 24, GxEPD_WHITE);
	showWeatherIcon();
	display.setCursor(310, 90);
	String hum = String(h, 0) + "%";
	display.print(hum);
	display.setCursor(310, 190);
	String temp = String(t, 1);
	display.print(temp);
	display.setCursor(210, 90);
	String out_temper = String(outTemp, 1);
	if (out_temper.length() > 4)
	{
		display.setFont(&FreeMonoBold12pt7b);
		display.print(out_temper);
		display.setFont(&FreeMonoBold18pt7b);
	}
	else
	{
		display.print(out_temper);
	}
	display.update();
}

void showWeatherIcon() {
	if (iconId.equals("01")) // clear sky
	{
		display.drawBitmap(sun, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("02")) // few clouds
	{
		display.drawBitmap(sun_cloud, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("03")) // scattered clouds
	{
		display.drawBitmap(cloud, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("04")) // broken clouds
	{
		display.drawBitmap(heavy_clouds, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("09")) // shower rain
	{
		display.drawBitmap(shower, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("10")) // rain
	{
		display.drawBitmap(rain, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("11")) // thunderstorm
	{
		display.drawBitmap(storm, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("13")) // snow
	{
		display.drawBitmap(snow, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else if (iconId.equals("50")) // mist
	{
		display.drawBitmap(mist, 307, 207, 80, 80, GxEPD_BLACK);
	}
	else
	{
		display.drawBitmap(question, 307, 207, 80, 80, GxEPD_BLACK);
	}
}

void getDTHSensorData() {
	long now = millis();
	if (now - lastGetSensorData > getSensorDataPeriod) {
		lastGetSensorData = now;
		Serial.println("Reading from DHT sensor!");
		// Reading temperature or humidity takes about 250 milliseconds!
		// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float sensorHum = dht.getHumidity();
		// Read temperature as Celsius (the default)
		float sensorTemp = dht.getTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		// Check if any reads failed and exit early (to try again).
		if (isnan(sensorHum) || isnan(sensorTemp)) {
			Serial.println("Failed to read from DHT sensor!");
			return;
		}
		if (sensorHum != h || sensorTemp != t)
		{
			// redraw display only if data changed more then  0.1
			if (fabs(h - sensorHum) > 1 || fabs(t - sensorTemp) > 0.1)
			{
				h = sensorHum;
				t = sensorTemp;
				if (client.connected()) {
					publishSensorData();
				}
				drawDisplay();
			}
		}
	}
}

void publishSensorData() {
	String sensorTemperature = String(t);
	String sensorHumidity = String(h);
	client.publish("WemosD1Mini_1/temperature", String(sensorTemperature).c_str());
	client.publish("WemosD1Mini_1/humidity", String(sensorHumidity).c_str());
}

