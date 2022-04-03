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
    #define MK2_12_pin 33 // Charge pump

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
  //#define Temperature_sensor_features_ON
  //#define Debug_ON
  #define ChargePumpSignalWatch_ON

/*################ Object Initialisation ################*/
  ModbusRTU mb; // Modbus Initialisation //

  //TaskHandle_t Task2;

  #ifdef Temperature_sensor_features_ON
    #include <DallasTemperature.h>
    #include <OneWire.h>
    OneWire oneWire(Temperatur_sensors_pin);
    DallasTemperature sensors(&oneWire);
  #endif
/*################ Variables ################*/
  float Current_Temp_air;
  float Current_Temp_chiller;
  unsigned long currentMillis;
  unsigned long previousMillis = 0;

  #ifdef ChargePumpSignalWatch_ON
    volatile unsigned long PreviousInterupt = millis();
    int ChargePumpSignal = 0;
    int previousChargePumpStatus;
  #endif

/*################ Functions ################*/
  //------------------- Charge pump -------------------//
    #ifdef ChargePumpSignalWatch_ON
      void ChargePumpSignalWatch(){ // This funtion is called by an interupt 12500 times per seconds
        PreviousInterupt = millis(); 
      }

      void ChargePumpLoop(){
        if((millis() - PreviousInterupt) < 10){
          ChargePumpSignal = 1;
          if(previousChargePumpStatus != 1){
            mb.Hreg(150, 1);
            digitalWrite(MK2_12_pin,HIGH);
            previousChargePumpStatus = 1;
          }
        }else{
          ChargePumpSignal = 0;
          if(previousChargePumpStatus != 0){
            mb.Hreg(150, 0);
            digitalWrite(MK2_12_pin,LOW);
            previousChargePumpStatus = 0;
          }
        }
      }
    #endif

  //------------------- Relays  -------------------//
    void Relays(){
      // Mist
        if(mb.Hreg(100) == 1){
          digitalWrite(MK2_6_pin, HIGH);
        }
        if(mb.Hreg(100) == 0){
          digitalWrite(MK2_6_pin, LOW);
        }

      // MK2_7_pin
        if(mb.Hreg(101) == 1){
          digitalWrite(MK2_7_pin, HIGH);
        }
        if(mb.Hreg(101) == 0){
          digitalWrite(MK2_7_pin, LOW);
        }

      // MK2_8_pin
        if(mb.Hreg(102) == 1){
          digitalWrite(MK2_8_pin, HIGH);
        }
        if(mb.Hreg(102) == 0){
          digitalWrite(MK2_8_pin, LOW);
        }

      // MK2_9_pin
        if(mb.Hreg(103) == 1){
          digitalWrite(MK2_9_pin, HIGH);
        }
        if(mb.Hreg(103) == 0){
          digitalWrite(MK2_9_pin, LOW);
        }

      // MK2_10_pin
        if(mb.Hreg(104) == 1){
          digitalWrite(MK2_10_pin, HIGH);
        }
        if(mb.Hreg(104) == 0){
          digitalWrite(MK2_10_pin, LOW);
        }

      // MK2_11_pin
        if(mb.Hreg(105) == 1){
          digitalWrite(MK2_11_pin, HIGH);
        }
        if(mb.Hreg(105) == 0){
          digitalWrite(MK2_11_pin, LOW);
        }
    }
    

  
  //------------------- Debug -------------------//
    #ifdef Debug_ON
      void Debug(){
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= 250) {
          previousMillis = currentMillis;
          Serial.println(ChargePumpSignal);
        }
      }
    #endif

/*################ Core 2 ################*/
  void Core_2_code( void * pvParameters ){
    // Core 2 setup

      
      
    // Core 2 loop
    while(true){
      
    }
  }

    

/*################ Setup ################*/
  void setup() {

      //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
     /*
      xTaskCreatePinnedToCore(
                        Core_2_code,   // Task function. //
                        "Task2",     // name of task. //
                        10000,       // Stack size of task //
                        NULL,        // parameter of the task //
                        1,           // priority of the task //
                        &Task2,      // Task handle to keep track of created task //
                        0);          // pin task to core 0 //
    */


    // Charge pump //
      #ifdef ChargePumpSignalWatch_ON
        pinMode(Charge_pump_pin,INPUT);
        attachInterrupt( // Interupt on GPIO 2 called 12500 times per second
          digitalPinToInterrupt(Charge_pump_pin), 
          ChargePumpSignalWatch, 
          RISING
        );
      #endif

      // Temperature sensor //
      #ifdef Temperature_sensor_features_ON
        pinMode(Temperatur_sensors_pin, INPUT);
        sensors.begin();
      #endif

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

    // Modbus stuff //
      Serial1.begin(115200, SERIAL_8N1, RS485_RX_pin, RS485_TX_pin);
      mb.begin(&Serial1, RS485_RE_DE_pin);
      mb.slave(SLAVE_ID);

    /*################ From Master to slave ################*/
        mb.addHreg(100); // Mist button state (MK2_6_pin)
        mb.addHreg(101); // MK2_7_pin
        mb.addHreg(102); // MK2_8_pin
        mb.addHreg(103); // MK2_9_pin
        mb.addHreg(104); // MK2_10_pin
        mb.addHreg(105); // MK2_11_pin
        mb.addHreg(106);
        mb.addHreg(107);
        mb.addHreg(108);
        mb.addHreg(109);

    /*################ From slave to master ################*/
      mb.addHreg(150); // Charge pump
      mb.addHreg(151);
      mb.addHreg(152);
      mb.addHreg(153);
      mb.addHreg(154);
      mb.addHreg(155);
      mb.addHreg(156);
      mb.addHreg(157);
      mb.addHreg(158);
      mb.addHreg(159);

    // Debug
      #ifdef Debug_ON
        Serial.begin(115200);
      #endif
  }

/*################ Loop ################*/
  void loop() {
    mb.task();
    yield();
    Relays();

    // Call functions
      #ifdef Debug_ON
        Debug();
      #endif

      #ifdef Temperature_sensor_features_ON
        sensors.requestTemperatures();
        Current_Temp_air = sensors.getTempCByIndex(0); //Ambiant air
        Current_Temp_chiller = sensors.getTempCByIndex(1); // Chiller
      #endif

      #ifdef ChargePumpSignalWatch_ON
        ChargePumpLoop();
      #endif

    // One milisecond delay for smoot operation
      delay(1);
  }