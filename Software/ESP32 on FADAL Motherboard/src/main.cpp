/*################ Libraries ################*/
  #include <Arduino.h>
  #include <ModbusRTU.h> // https://github.com/emelianov/modbus-esp8266
  //#include <OneWire.h> 
  //#include <DallasTemperature.h>

/*################ ESP32 Pin asignement ################*/
  //--- Outputs ----//
    #define MK2_10 5
    #define MK2_11 18
    #define MK2_12 13
    #define MK2_13 12
    #define MK2_14 14
    #define MK2_15 27
    #define MK2_16 26

  //--- Inputs ----//
    #define Charge_pump 2
    #define Temperatur_sensors 22

  //--- RS485 ----//
    #define RS485_TX 17
    #define RS485_RX 16
    #define RS485_RE_DE 4

/*################ Constant ################*/
  #define SLAVE_ID 2 // Used for modbus

/*################ Object Initialisation ################*/
  ModbusRTU mb; // Modbus Initialisation //
  //OneWire oneWire(Temperatur_sensors);
  //DallasTemperature sensors(&oneWire);

/*################ Variables ################*/
  bool Debuging_Mode = false;
  float Current_Temp_air;
  float Current_Temp_chiller;
  unsigned long currentMillis;
  unsigned long previousMillis = 0;
  int hreg100;
  int Current_Charge_pump_value;

/*################ Functions ################*/
  //------------------- Debug -------------------//
    void Debug(){
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 250) {
        previousMillis = currentMillis;
        Serial.printf("Air temperature = %i Chiller temperature = %i Charge Pump = %i \n", Current_Temp_air, Current_Temp_chiller, Current_Charge_pump_value);
      }
    }

void setup() {
  // Pin Modes //
    pinMode(MK2_10, OUTPUT);
    pinMode(MK2_11, OUTPUT);
    pinMode(MK2_12, OUTPUT);
    pinMode(MK2_13, OUTPUT);
    pinMode(MK2_14, OUTPUT);
    pinMode(MK2_15, OUTPUT);
    pinMode(MK2_16, OUTPUT);

    pinMode(Charge_pump, INPUT);
    pinMode(Temperatur_sensors, INPUT);

    digitalWrite(MK2_10,LOW);
    digitalWrite(MK2_11,LOW);
    digitalWrite(MK2_12,LOW);
    digitalWrite(MK2_13,LOW);
    digitalWrite(MK2_14,LOW);
    digitalWrite(MK2_15,LOW);
    digitalWrite(MK2_16,LOW);

  // Temperature sensor //
    //Serial.begin(115200);
    //sensors.begin();

  // Modbus stuf //
    Serial1.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
    mb.begin(&Serial1, RS485_RE_DE);
    mb.slave(SLAVE_ID);

  /*################ From Master to slave ################*/
      mb.addHreg(100);

  /*################ From slave to master ################*/
      mb.addHreg(150);

      hreg100 = mb.Hreg(100);
      mb.Hreg(150, 100); // initially set the value to 100% on modbus
}

void loop() {
  // Get current temperature
    /*sensors.requestTemperatures();
    Current_Temp_air = sensors.getTempCByIndex(0); //Ambiant air
    Current_Temp_chiller = sensors.getTempCByIndex(1); // Chiller*/

  Current_Charge_pump_value = analogRead(Charge_pump);
  // Update Mudbus
    mb.task();
    yield();

  // Call functions
    if (Debuging_Mode){ Debug(); }

  // One milisecond delay for smoot operation
    delay(1);
}