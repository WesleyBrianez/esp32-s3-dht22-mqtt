This project was initialy develop on wokwi simulator with arduino format (setup() + loop())
We can easily test it as follow:

1. Prepare MQTT broker:
1.1 Go to http://www.hivemq.com/demos/websocket-client/
1.2 Click on Connect button to make connection avaiable
1.3 Add New Topic Subscription with "topic_json/#" text in the Topic field

2. Prepare wokwi device wifi connection:
2.1 It's necessary to run Wokwi Gateway and "Enable Private Wokwi IoT Gateway" by F1 from the code-editor
2.2 Download Wokwi IoT Network Gateway by: https://github.com/wokwi/wokwigw/releases/

3. Start simulation:
3.1 Click on wokwi Play button; The ESP32 have to start the bootloader and the firmware application.
3.2 Device have to conenct on wifi and after that to mqtt broker;

4. Changing MQTT message:
4.1 Device have to send JSON data each 3s to broker;
4.2 From send a message from HiveMQ broker, into Publish session:
   4.2.1 Write "topic_rgb" on Topic field;
   4.2.2 Write some string from mqtt_commands at Message field;
   4.2.3 Click on Publish button;
   4.2.4. Device should print the command received and execute the corresponding order
