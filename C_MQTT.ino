//Connect to MQTT server
void reconnect() {
  while (!client.connected()) { // Loop until we're reconnected
    dbprintln("Attempting MQTT connection...");
    if (client.connect(DEVICE_MQTT_NAME, mqtt_username, mqtt_password)) {  // Attempt to connect
      dbprintln("MQTT connected");
      //client.subscribe(subscribe_top); //Does ttt need to sub to anything?
      //sendState(0);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

#define MQTT_TOPIC "hcdeiol"

//Send MQTT status update
void sendState(bool tttActive) {
  StaticJsonBuffer<512> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  String onOff = tttActive? "on" : "off";
  root["tictactoe"] = onOff;


  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(MQTT_TOPIC, buffer, true);
}
