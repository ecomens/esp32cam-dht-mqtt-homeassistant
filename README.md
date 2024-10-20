# esp32-cam + dht11 sensor send to message via home assistant mqtt broker
esp32-cam edge board connected dht sensor data send to mqtt integrated the Home Assistant add on.
o esp32-cam arduino ide code: esp32cammqtt_homeassistant.ino, camera_pins.h
 - you can explorer mqtt topics with MQTT EXplorer Tool 
 - https://mqtt-explorer.com/
 - ![image](https://github.com/user-attachments/assets/1f77dd41-aaaa-44f7-9139-88acce6e30e3)

o home assistant(HA) configuration.yaml example
 - ![image](https://github.com/user-attachments/assets/c5323b20-a3f2-4e15-93b3-c2aac09be0db)
 - configuration.yaml example
   # Mqtt configuration examples
    mqtt:
      camera:
        - topic: esp32/cam_0
      sensor:
        - name: "esp32_Temp"
          state_topic: "esp32/dht_0"
          unit_of_measurement: "°C"
          value_template: "{{ value_json.temperature }}"
          
    
        - name: "esp32_Hum"
          state_topic: "esp32/dht_0"
          unit_of_measurement: "%"
          value_template: "{{ value_json.humidity }}"
