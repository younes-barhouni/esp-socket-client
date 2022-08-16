/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

#include <WebSocketsClient.h>
#include <SocketIoClient.h>
#include <WiFiManager.h> // V0.16.0  https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>

#include <Hash.h>

SocketIOclient socket;

// Initialize sensor parameters
// float temperature = 0.0, humidity = 0.0, pressure = 0.0, gas = 0.0;

// Instantiate an object for the OLED screen
// U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

char host[] = "192.168.1.18";
int port = 3000;

unsigned long previousMillis = 0;
unsigned long messageTimestamp = 0;
long interval = 5000;

String old_value, value;
int rCounter = 0;
int currentState;
int initState;

int angle = 0;
int aState;
int aLastState;
int lastValue = 0;

#define USE_SERIAL Serial1

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{

    Serial.println("Event Received !!!!");
    Serial.println(type);

    switch (type)
    {
    case sIOtype_DISCONNECT:
        USE_SERIAL.printf("[IOc] Disconnected!\n");
        break;
    case sIOtype_CONNECT:
        USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

        // join default namespace (no auto join in Socket.IO V3)
        socket.send(sIOtype_CONNECT, "/");
        break;
    case sIOtype_EVENT:

        Serial.printf("[IOc] get event: %s\n", payload);
        {
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload, length);
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return;
            }

            String eventName = doc[0];
            Serial.printf("[IOc] event name: %s\n", eventName.c_str());

            String eventValue = doc[1];
            Serial.printf("[IOc] event value: %s\n", eventValue.c_str());

            const char *encoderVal = doc[1]["rotEnc"];

            // Print values.
            Serial.println(encoderVal);
        }

        break;
    case sIOtype_ACK:
        USE_SERIAL.printf("[IOc] get ack: %u\n", length);
        hexdump(payload, length);
        break;
    case sIOtype_ERROR:
        USE_SERIAL.printf("[IOc] get error: %u\n", length);
        hexdump(payload, length);
        break;
    case sIOtype_BINARY_EVENT:
        Serial.println("Binary value ");
        Serial.printf("[IOc] get binary: %u\n", length);
        Serial.println("Binary Received !!!!");
        hexdump(payload, length);
        break;
    case sIOtype_BINARY_ACK:
        USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
        hexdump(payload, length);
        break;
    case '50':
        Serial.println("Value ");
        Serial.printf("[IOc] get binary: %u\n", length);
        hexdump(payload, length);
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(D5, INPUT_PULLUP);
    pinMode(D6, INPUT_PULLUP);

    aLastState = digitalRead(D5);

    // Connect to WiFi
    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    // reset saved settings
    // wifiManager.resetSettings();

    // set custom ip for portal
    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("esp-socket-manager");
    // or use this for auto generated name ESP + ChipID
    // wifiManager.autoConnect();

    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println(WiFi.localIP());

    // Serial.println(WiFi.localIP());

    // Initialize the screen
    // u8g2.setBusClock(100000);
    // u8g2.begin();
    // u8g2.setFont(u8g2_font_ncenB08_tr);

    // event handler
    socket.onEvent(socketIOEvent);
    //
    socket.begin("192.168.1.18", 3000, "/socket.io/?EIO=4");

    delay(5000);
}

void loop()
{
    socket.loop();

    compute_angle();

    // creat JSON message for Socket.IO (event)
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // add evnet name
    // Hint: socket.on('event_name', ....
    array.add("event_name");

    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();
    value = (String)(rCounter);
    if (rCounter != lastValue)
    {
        // socket.broadcastTXT(value);
        param1["rotEnc"] = value;
        // JSON to String (serializion)
        String output;
        serializeJson(doc, output);
        // Send event
        socket.sendEVENT(output);
    }

    lastValue = rCounter;
}

void compute_angle()
{
    aState = digitalRead(D5);

    if (aState != aLastState)
    {
        if (digitalRead(D6) != aState)
        {
            rCounter++;
            if (rCounter > 179)
                rCounter = 179;
        }
        else
        {
            rCounter--;
            if (rCounter < 0)
                rCounter = 0;
        }

        Serial.print("Position: ");
        Serial.print(rCounter);
        Serial.print("deg");
        Serial.println("");
    }
    aLastState = aState;
}
