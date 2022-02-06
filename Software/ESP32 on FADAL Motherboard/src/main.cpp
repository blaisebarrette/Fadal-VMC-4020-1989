/*################ Libraries ################*/
  #include <Arduino.h>
  #include <ModbusRTU.h> // https://github.com/emelianov/modbus-esp8266
  
/*################ ESP32 Pin asignement ################*/
  //--- Outputs ----//
    #define MK2_6_pin 13
    #define MK2_7_pin 12
    #define MK2_8_pin 14
    #define MK2_9_pin 27
    #define MK2_10_pin 26
    #define MK2_11_pin 25
    #define MK2_12_pin 33

  //--- Inputs ----//
    #define Charge_pump_pin 2
    #define Temperatur_sensors_pin 22

  //--- RS485 ----//
    #define RS485_TX_pin 17
    #define RS485_RX_pin 16
    #define RS485_RE_DE_pin 4

/*################ Constant ################*/
  #define SLAVE_ID 2 // Used for modbus

/*################ Features switches ################*/  
  #define Temperature_sensor_features_ON
  //#define Debug_ON
  #define Multicore_ON

/*################ Multicore stuff ################*/
  #ifdef Multicore_ON
    TaskHandle_t Task1;
    TaskHandle_t Task2;
  #endif

/*################ Object Initialisation ################*/
  ModbusRTU mb; // Modbus Initialisation //

  #ifdef Temperature_sensor_features_ON
    #include <DallasTemperature.h>
    OneWire oneWire(Temperatur_sensors_pin);
    DallasTemperature sensors(&oneWire);
  #endif
/*################ Variables ################*/
  float Current_Temp_air;
  float Current_Temp_chiller;
  unsigned long currentMillis;
  unsigned long previousMillis = 0;
  int hreg100;
  int Current_Charge_pump_value;

/*################ Functions ################*/
  //------------------- Write charge pump value to modbus -------------------//
    void Charge_pump(){
      Current_Charge_pump_value = analogRead(Charge_pump_pin);
      mb.Hreg(150, Current_Charge_pump_value);
    }
  //------------------- Debug -------------------//
    #ifdef Debug_ON
      void Debug(){
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= 250) {
          previousMillis = currentMillis;
          Serial.printf("Air temperature = %d Chiller temperature = %d Charge Pump = %i \n", Current_Temp_air, Current_Temp_chiller, Current_Charge_pump_value);
        }
      }
    #endif

/*################ Tasks specific to core 0 ################*/
  #ifdef Multicore_ON
    void Task1code( void * pvParameters ){
      //--- Setup Section Start ----//
        pinMode(Temperatur_sensors_pin, INPUT);
        // Temperature sensor //
        Serial.begin(115200);
        #ifdef Temperature_sensor_features_ON
          sensors.begin();
        #endif
      //--- Setup Section End ----//
      
      while (true) {
        //--- Loop Section Start ----//
          // Get current temperature
          #ifdef Temperature_sensor_features_ON
            sensors.requestTemperatures();
            Current_Temp_air = sensors.getTempCByIndex(0); //Ambiant air
            Current_Temp_chiller = sensors.getTempCByIndex(1); // Chiller
          #endif
        //--- Loop Section End ----//
      }
    }
  #endif

/*################ Tasks specific to core 1 ################*/
  #ifdef Multicore_ON
    void Task2code( void * pvParameters ){
      //--- Setup Section Start ----//
        // Pin Modes //
          pinMode(MK2_6_pin, OUTPUT);
          pinMode(MK2_7_pin, OUTPUT);
          pinMode(MK2_8_pin, OUTPUT);
          pinMode(MK2_9_pin, OUTPUT);
          pinMode(MK2_10_pin, OUTPUT);
          pinMode(MK2_11_pin, OUTPUT);
          pinMode(MK2_12_pin, OUTPUT);

          digitalWrite(MK2_6_pin,LOW);
          digitalWrite(MK2_7_pin,LOW);
          digitalWrite(MK2_8_pin,LOW);
          digitalWrite(MK2_9_pin,LOW);
          digitalWrite(MK2_10_pin,LOW);
          digitalWrite(MK2_11_pin,LOW);
          digitalWrite(MK2_12_pin,LOW);

          pinMode(Charge_pump_pin, INPUT);

        // Modbus stuf //
          Serial1.begin(115200, SERIAL_8N1, RS485_RX_pin, RS485_TX_pin);
          mb.begin(&Serial1, RS485_RE_DE_pin);
          mb.slave(SLAVE_ID);

        // Debug
          #ifdef Debug_ON || Temperature_sensor_features_ON
            Serial.begin(115200);
          #endif

          /*################ From Master to slave ################*/
              mb.addHreg(100);

          /*################ From slave to master ################*/
              mb.addHreg(150);
              hreg100 = mb.Hreg(100);

      //--- Setup Section End ----//
      
      while (true) {
        //--- Loop Section Start ----//
          // Update Mudbus
            mb.task();
            yield();

          // Call functions
            Charge_pump();
            #ifdef Debug_ON
              Debug();
            #endif

          // One milisecond delay for smoot operation
            delay(1);
        //--- Loop Section End ----//
      }
    }
  #endif

/*################ Setup ################*/
  void setup() {
    btStop();
    // Multicore specific code //
      #ifdef Multicore_ON
        xTaskCreatePinnedToCore(
          Task1code,   /* Task function. */
          "Task1",     /* name of task. */
          10000,       /* Stack size of task */
          NULL,        /* parameter of the task */
          0,           /* priority of the task */
          &Task1,      /* Task handle to keep track of created task */
          0);          /* pin task to core 0 */      

        xTaskCreatePinnedToCore(
          Task2code,   /* Task function. */
          "Task2",     /* name of task. */
          10000,       /* Stack size of task */
          NULL,        /* parameter of the task */
          1,           /* priority of the task */
          &Task2,      /* Task handle to keep track of created task */
          1);
      #endif
  }

/*################ Loop ################*/
  void loop() {

  }