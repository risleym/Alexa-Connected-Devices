/*********************************************************************
 *
 *  aws.c
 *
 *  AWS IoT Example Application
 *
 *  Pin Connections:
 *   PC5 Input - Switch (Active Low)
 *   PC7 Output - LED (Active HIGH)
 *   
 *   Follow Instructions in the "Alexa Connected Devices" training 
 *    to run this application.
 *
 *   This sketch connects to a AWS IOT with mutual verification.
 *
 *  To run this demo:
 *  1. Create an account and log in http://aws.amazon.com/iot
 *  2. Create a THING and name it 'arduino'
 *  3. Create and Download a certificate package.
 *  4. Copy the contents of private key to 'aws_private_key' variable.
 *     NOTE: Each line must be added with '\n' and quoted with " " to be parsed successfully.
 *  5. Use pycert to convert certificate to header file by running
 *       $ python ../../../tools/pycert/pycert.py convert -c local_cert -l LOCAL_CERT_LEN certificate.pem.crt.txt
 *     This should create an certificates.h with variable local_cert.
 *     Place in "aws" arduino project folder.
 *  6. Close and reopen this sketch to load certificates.h
 *  7. Compile and run
 *
 *********************************************************************/

#include <adafruit_feather.h>
#include <adafruit_mqtt.h>
#include "certificates.h"

/* TODO: Enter your WiFi credentials here */
#define WLAN_SSID         "Mobile"
#define WLAN_PASS         "*****"

#define MQTT_TX_BUFSIZE   1024
#define MQTT_RX_BUFSIZE   1024

/* Physical Pins for LED and Switch */
#define LED_PIN     PC7
#define SWITCH_PIN  PC5
#define GND_PIN     PA4

#define ONBOARD_LED_PIN PA15

AdafruitMQTT mqtt;

// ======================================================
// TODO: Place your AWS IoT Thing Address here
#define AWS_IOT_MQTT_HOST              "TODO"
#define AWS_IOT_MQTT_PORT              8883
#define AWS_IOT_MQTT_CLIENT_ID         "arduino"
#define AWS_IOT_MY_THING_NAME          "arduino"
// ======================================================

#define AWS_IOT_STATE_TOPIC            AWS_IOT_MY_THING_NAME "/state"
#define AWS_IOT_CONTROL_TOPIC          AWS_IOT_MY_THING_NAME "/control"

#define SHADOW_PUBLISH_STATE_OFF      "{ \"state\": {\"reported\": { \"status\": \"OFF\" } } }"
#define SHADOW_PUBLISH_STATE_ON       "{ \"state\": {\"reported\": { \"status\": \"ON\" } } }"
#define TEST_SWITCH_ON "{ \"switch\":\"open\", \"temperature\":\"100\"}"
#define TEST_SWITCH_OFF "{ \"switch\":\"closed\", \"temperature\":\"100\"}"

/* TODO: Place your private key downloaded from AWS here */
const char aws_private_key[] =
 "YOUR PRIVATE KEY HERE";


/*********************************************************************
 *
 *   Disconnect handler for MQTT broker connection
 *   
 *********************************************************************/
void disconnect_callback(void)
{
  Serial.println();
  Serial.println("-----------------------------");
  Serial.println("DISCONNECTED FROM MQTT BROKER");
  Serial.println("-----------------------------");
  Serial.println();
}

/*********************************************************************
 *
 *   The setup function runs once when the board comes out of reset
 *   
 *********************************************************************/
void setup()
{
  Serial.begin(115200);

  // Wait 10 seconds for the Serial Monitor to open
  //  else proceed 
  uint32_t bootTime = millis();
  while (!Serial)
  {
    if( millis()-bootTime > 10000 )
      break;
    // Delay required to avoid RTOS task switching problems
    delay(1);
  }

  /* Configure LED and Switch Pins as GPIOs */
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  pinMode(GND_PIN, OUTPUT);
  digitalWrite(LED_PIN,LOW);
  digitalWrite(ONBOARD_LED_PIN,LOW);
  digitalWrite(GND_PIN, LOW);
  

  Serial.println("======================================");
  Serial.println("Alexa Connected Device Example");
  Serial.print( F(" Compiled: "));
  Serial.print( F(__DATE__));
  Serial.print( F(", "));
  Serial.println( F(__TIME__));
  Serial.print(" ");
  Serial.println( F(__VERSION__));

  while ( !connectAP() )
  {
    delay(500); // delay between each attempt
  }

  // Connected: Print network info
  Feather.printNetwork();

  // Tell the MQTT client to auto print error codes and halt on errors
  mqtt.err_actions(true, true);

  // Set ClientID
  mqtt.clientID(AWS_IOT_MQTT_CLIENT_ID);
  mqtt.setBufferSize(MQTT_TX_BUFSIZE, MQTT_RX_BUFSIZE);

  // Set the disconnect callback handler
  mqtt.setDisconnectCallback(disconnect_callback);

  // default RootCA include certificate to verify AWS
  Feather.useDefaultRootCA(true);
  mqtt.tlsRequireVerification(false);

  // Setting Indentity with AWS Private Key & Certificate
  mqtt.tlsSetIdentity(aws_private_key, local_cert, LOCAL_CERT_LEN);

  // Connect with SSL/TLS
  Serial.printf("Connecting to " AWS_IOT_MQTT_HOST " port %d ... ", AWS_IOT_MQTT_PORT);
  mqtt.connectSSL(AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);
  Serial.println("OK");

  Serial.print("Subscribing to " AWS_IOT_CONTROL_TOPIC " ... ");
  mqtt.subscribe(AWS_IOT_CONTROL_TOPIC, MQTT_QOS_AT_MOST_ONCE, subscribed_callback); // Will halted if an error occurs
  Serial.println("OK");

  Serial.println("======================================");

  // Indicate Boot-up Complete
  led_blink(3);
}

/*********************************************************************
 *
 *   Check for Switch changes and send updates to cloud
 *   
 *********************************************************************/
void switch_poll()
{
  static uint8_t sw_last = 0;
  uint8_t sw_cur = digitalRead(SWITCH_PIN);
  if( sw_last != sw_cur )
  {
    // Send new state
    if( sw_cur == 1 )
    {
      Serial.println("Switched ON");
      mqtt.publish(AWS_IOT_STATE_TOPIC, TEST_SWITCH_ON, MQTT_QOS_AT_LEAST_ONCE);
    }
    else
    {
      Serial.println("Switched OFF");
      mqtt.publish(AWS_IOT_STATE_TOPIC, TEST_SWITCH_OFF, MQTT_QOS_AT_LEAST_ONCE);
    }
  }
  sw_last = sw_cur;
}

/*********************************************************************
 *
 *   Simple Function to Blink the LED 'n' number of times
 *   
 *********************************************************************/
void led_blink( uint8_t n )
{
  uint8_t PERIOD = 200;
  for( uint8_t i=0; i<n; i++ )
  {
    digitalWrite(LED_PIN,HIGH);
    digitalWrite(ONBOARD_LED_PIN,HIGH);
    delay(200);
    digitalWrite(LED_PIN,LOW);
    digitalWrite(ONBOARD_LED_PIN,LOW);
    delay(200);
  }
}

/*********************************************************************
 *
 *   Blink the onboard LED to indicate code is running
 *   
 *********************************************************************/
void led_heartbeat( void )
{
  static uint32_t t = 0;
  static bool led;
  if( millis() - t > 1000 )
  {
    led^=1;
    digitalWrite( ONBOARD_LED_PIN, led );
    t = millis();
  }
}

/*********************************************************************
 *
 *   MQTT subscribe event callback handler
 *   
 *   topic      The topic causing this callback to fire
 *   message    The new value associated with 'topic'
 *   
 *   'topic' and 'message' are UTF8Strings (byte array), which means
 *           they are not null-terminated like C-style strings. You can
 *           access its data and len using .data & .len, although there is
 *           also a Serial.print override to handle UTF8String data types.
 *   
 *********************************************************************/
void subscribed_callback(UTF8String topic, UTF8String message)
{
  Serial.println("Received message:");
  Serial.println(message);
  if ( 0 == memcmp("0", message.data, message.len) )
  {
    Serial.println("Turning LED Off");
    digitalWrite(LED_PIN, LOW);
  }

  if ( 0 == memcmp("1", message.data, message.len) )
  {
    Serial.println("Turning LED On");
    digitalWrite(LED_PIN, HIGH);
  }
}

/*********************************************************************
 *
 *   Connect to defined Access Point
 *   
 *********************************************************************/
bool connectAP(void)
{
  // Attempt to connect to an AP
  Serial.print("Please wait while connecting to: '" WLAN_SSID "' ... ");

  if ( Feather.connect(WLAN_SSID, WLAN_PASS) )
  {
    Serial.println("Connected!");
  }
  else
  {
    Serial.printf("Failed! %s (%d)", Feather.errstr(), Feather.errnum());
    Serial.println();
  }
  Serial.println();

  return Feather.connected();
}

/*********************************************************************
 *
 *   This loop function runs over and over again (Main)
 *   
 *********************************************************************/
void loop()
{
  // Check for changes from Switch
  switch_poll();
  
  // Signal we're alive
  led_heartbeat();
  
  delay(100);
}

