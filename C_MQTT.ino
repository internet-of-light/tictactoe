//Connect to MQTT server
void reconnect() {
  while (!client.connected()) { // Loop until we're reconnected
    dbprintln("Attempting MQTT connection...");
    if (client.connect(DEVICE_MQTT_NAME, mqtt_username, mqtt_password)) {  // Attempt to connect
      dbprintln("MQTT onnected");
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

#define MQTT_TOPIC "hcdeiot"

//Send MQTT status update
void sendState(bool tttActive) {
  StaticJsonBuffer<512> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["tictactoe"] = tttActive;
//  JsonObject& color = root.createNestedObject("color");
//  color["r"] = red;
//  color["g"] = green;
//  color["b"] = blue;
//
//  root["brightness"] = brightness;
//  root["effect"] = effectString.c_str();


  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(MQTT_TOPIC, buffer, true);
}
