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
unsigned long connectionTime = 60000;
unsigned long rebootDelay = 600000;

PubSubClient client(espClient);
char msg[50];

// arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxIO_Class io(SPI, SS, 0, 2);
GxEPD_Class display(io);

// Pin number for DHT22 data pin
int dhtPin = 16;
DHTesp dht;
String sensorTemperature = "0";
String sensorHumidity = "0";
String outTemperature = "0";
float h = 0;
float t = 0;
float outTemp = 0;
float outTemp_prev = 0;
unsigned long lastGetSensorData = 0;
int getSensorDataPeriod = 15000;


void setup()
{
	Serial.begin(115200);
	display.init();
	dht.setup(dhtPin);
	setup_wifi();
	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);
}

//Reconnection to MQTT broker
void reconnect() {
	int i = 0;
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (client.connect("WemosD1Mini_1")) {
			Serial.println("connected");
			// Once connected, publish an announcement...
			client.publish("WemosD1Mini_1/status", "WemosD1Mini_1 connected");
			// ... and resubscribe
			client.subscribe("WittyCloud/temp");
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
		i++;
		if (i > 15)
		{
			return;
		}
	}
}

void setup_wifi() {

	delay(50);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(SSID);

	WiFi.begin(SSID, PASSWORD);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		if (millis() > connectionTime)
		{
			restartIfDisconnected();
		}
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
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
		if (fabs(outTemp - outT) < 0.1)
		{
			drawDisplay();
		}
		outTemperature = String(outTemp, 1);
		outTemp = outT;
	}
}

void loop()
{
	// process MQTT connection
	if (!client.connected()) {
		reconnect();
	}
	client.loop();
	getDTHSensorData();
	restartIfDisconnected();
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
	display.drawBitmap(out_temp, 203, 15, 50, 50, GxEPD_BLACK);
	if (outTemp > outTemp_prev)
	{
		display.drawExampleBitmap(gridicons_arrow_up, 260, 25, 24, 24, GxEPD_BLACK);
	}
	else if (outTemp < outTemp_prev)
	{
		display.drawExampleBitmap(gridicons_arrow_down, 260, 25, 24, 24, GxEPD_BLACK);
	}
	else
	{
		display.drawExampleBitmap(gridicons_arrow_left, 260, 25, 24, 24, GxEPD_BLACK);
	}
	outTemp_prev = outTemp;
	display.drawExampleBitmap(thermo_image, 303, 110, 43, 50, GxEPD_WHITE);
	display.drawExampleBitmap(gridicons_house, 350, 125, 24, 24, GxEPD_BLACK);
	display.drawExampleBitmap(snow, 307, 207, 80, 80, GxEPD_WHITE);
	display.setCursor(310, 90);
	String hum = String(h, 0) + "%";
	display.print(hum);
	display.setCursor(310, 190);
	String temp = String(t, 1);
	display.print(temp);
	display.setCursor(210, 90);
	String out_temper = String(outTemp, 1);
	//display.setFont(&FreeMonoBold12pt7b);
	display.print(out_temper);
	display.update();
}

void getDTHSensorData() {
	long now = millis();
	if (now - lastGetSensorData > getSensorDataPeriod) {
		lastGetSensorData = now;
		// Reading temperature or humidity takes about 250 milliseconds!
		// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float sensorHum = dht.getHumidity();
		// Read temperature as Celsius (the default)
		float sensorTemp = dht.getTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		// Check if any reads failed and exit early (to try again).
		if (isnan(h) || isnan(t)) {
			Serial.println("Failed to read from DHT sensor!");
			return;
		}
		//Serial.print("Humidity: ");
		//Serial.print(h);
		//Serial.print(" %\t");
		//Serial.print("Temperature: ");
		//Serial.print(t);
		//Serial.println(" *C ");
		if (sensorHum != h || sensorTemp != t)
		{
			//Serial.println(h);
			//Serial.println(t);
			//Serial.println(sensorHum);
			//Serial.println(sensorTemp);
			sensorTemperature = String(t);
			sensorHumidity = String(h);
			publishSensorData();
			if (fabs(h - sensorHum) < 0.1 || fabs(t - sensorTemp) < 0.1)
			{
				drawDisplay();
			}
			h = sensorHum;
			t = sensorTemp;
		}
	}
}

void publishSensorData() {
	client.publish("WemosD1Mini_1/temperature", String(sensorTemperature).c_str());
	client.publish("WemosD1Mini_1/humidity", String(sensorHumidity).c_str());
}

void restartIfDisconnected() {
	if (WiFi.status() != WL_CONNECTED)
	{
		delay(rebootDelay);
		ESP.restart();
	}
}
