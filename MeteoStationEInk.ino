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
float livingTemp = 0;
float livingTemp_prev = 0;
float livHumidity = 0;
float pressure = 0;
int numberOfValues = 90;
int pressureValues[90];
unsigned long lastPressureSave = 0;
int pressureSavePeriod = 15 * 60000;
unsigned long lastGetSensorData = 0;
int getSensorDataPeriod = 60000;
String iconId = "";
String dayOfWeek = "--------";
String day = "--";
String month = "---";
float windSpeed = 0;
int screenRefreshPeriod = 60000;
unsigned long lastScreenRefresh = 0;



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
	for (int k = 0; k < numberOfValues; k++) {
		pressureValues[k] = 0;
	}
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
		client.subscribe("WemosD1Mini_2/temp");
		client.subscribe("WemosD1Mini_2/humidity");
		client.subscribe("WemosD1Mini_2/pressure");
		client.subscribe("Wheather/showIcon");
		client.subscribe("Wheather/showWind");
		client.subscribe("Date");
	}
	else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.println(" try again in 5 minutes");
	}
}

void addToPressureValues(int val) {
	long now = millis();
	if (now - lastPressureSave > pressureSavePeriod) {
		lastPressureSave = now;
		for (int k = 0; k < numberOfValues - 1; k++) {
			pressureValues[k] = pressureValues[k + 1];
		}
		pressureValues[numberOfValues - 1] = val;

		for (int k = 0; k < numberOfValues; k++) {
			Serial.print(pressureValues[k]);
			Serial.print(",");
		}
		Serial.println("");
	}
}

void drowPressureChart() {
	for (int k = 0; k < numberOfValues; k++) {
		int hights = 37 + 1013 - pressureValues[k];
		display.fillCircle(103 + k, hights, 1, GxEPD_BLACK);
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
	if (strcmp(topic, "WemosD1Mini_2/temp") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		float livT = atof(buffer);
		if (fabs(livingTemp - livT) > 0.1)
		{
			livingTemp_prev = livingTemp;
			livingTemp = livT;
			drawDisplay();
		}
	}
	if (strcmp(topic, "WemosD1Mini_2/humidity") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		livHumidity = atof(buffer);
		drawDisplay();
		
	}
	if (strcmp(topic, "WemosD1Mini_2/pressure") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		float press = atof(buffer);
		addToPressureValues((int)press);
		if (fabs(pressure - press) > 1)
		{
			pressure = press;
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
		float wind = atof(buffer);
		if (fabs(windSpeed - wind) > 1)
		{
			windSpeed = wind;
			drawDisplay();
		}
	}
	if (strcmp(topic, "Date") == 0) {
		char* buffer = (char*)payload;
		buffer[length] = '\0';
		String date = String(buffer);
		dayOfWeek = getSubString(date, ' ', 0);
		Serial.println(dayOfWeek);
		day = getSubString(date, ' ', 1);
		Serial.println(day);
		month = getSubString(date, ' ', 2);
		Serial.println(month);
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
	display.fillRect(300, 190, 100, 3, GxEPD_BLACK);
	//Current date
	display.setFont(&FreeMonoBold9pt7b);
	display.setCursor(2, 37);
	display.print(dayOfWeek);
	display.setFont(&FreeMonoBold18pt7b);
	display.setCursor(30, 73);
	display.print(day);
	display.setFont(&FreeMonoBold12pt7b);
	display.setCursor(30, 95);
	display.print(month);

	//Out temperature
	display.drawBitmap(out_temp_icon, 203, 17, 50, 50, GxEPD_BLACK);
	if (outTemp > outTemp_prev)
	{
		showTrendIcon(gridicons_arrow_up);
	}
	else if (outTemp < outTemp_prev)
	{
		showTrendIcon(gridicons_arrow_down);
	}
	else
	{
		showTrendIcon(gridicons_arrow_left);
	}
	
	showWeatherIcon();

	//kitchen temp
	display.setFont(&FreeMonoBold9pt7b);
	display.setCursor(305, 32);
	display.print("Kitchen");
	display.setFont(&FreeMonoBold18pt7b);
	display.setCursor(305, 63);
	String temp = String(t, 1);
	display.print(temp);
	display.setCursor(320, 95);
	String hum = String(h, 0) + "%";
	display.print(hum);
	//Living temp
	display.setFont(&FreeMonoBold9pt7b);
	display.setCursor(310, 120);
	display.print("Living");
	display.setFont(&FreeMonoBold18pt7b);
	display.setCursor(305, 151);
	String livTemp = String(livingTemp, 1);
	display.print(livTemp);
	display.setCursor(320, 183);
	String livHum = String(livHumidity, 0) + "%";
	display.print(livHum);
	//Pressure
	display.setCursor(108, 95);
	String press = String(pressure, 0);
	display.print(press);
	display.drawLine(100, 37, 200, 37, GxEPD_BLACK);
	drowPressureChart();
	//Out temperature
	display.setCursor(210, 95);
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
	display.setFont(&FreeMonoBold9pt7b);
	display.setCursor(310, 208);
	String windSp = String(windSpeed, 0) + " m/s";
	display.print(windSp);
	display.setFont(&FreeMonoBold18pt7b);
	long now = millis();
	if (now - lastScreenRefresh > screenRefreshPeriod) {
		display.update();
		lastScreenRefresh = now;
	}
}

void showTrendIcon(const uint8_t *bitmap) {
	display.drawBitmap(bitmap, 260, 40, 24, 24, GxEPD_WHITE);
}

void showWeatherIcon() {
	if (iconId.equals("01")) // clear sky
	{
		drowIcon(sun);
	}
	else if (iconId.equals("02")) // few clouds
	{
		drowIcon(sun_cloud);
	}
	else if (iconId.equals("03")) // scattered clouds
	{
		drowIcon(cloud);
	}
	else if (iconId.equals("04")) // broken clouds
	{
		drowIcon(heavy_clouds);
	}
	else if (iconId.equals("09")) // shower rain
	{
		drowIcon(shower);
	}
	else if (iconId.equals("10")) // rain
	{
		drowIcon(rain);
	}
	else if (iconId.equals("11")) // thunderstorm
	{
		drowIcon(storm);
	}
	else if (iconId.equals("13")) // snow
	{
		drowIcon(snow);
	}
	else if (iconId.equals("50")) // mist
	{
		drowIcon(mist);
	}
	else
	{
		drowIcon(question);
	}
}

void drowIcon(const uint8_t *bitmap) {
	display.drawBitmap(bitmap, 307, 212, 80, 80, GxEPD_BLACK);
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

String getSubString(String data, char separator, int index)
{
	int found = 0;
	int strIndex[] = { 0, -1 };
	int maxIndex = data.length() - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++) {
		if (data.charAt(i) == separator || i == maxIndex) {
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}

	return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

