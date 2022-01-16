/*################ Libraries ################*/
  #include <Arduino.h>
  #include <ModbusRTU.h> // https://github.com/emelianov/modbus-esp8266
  #include <ESP32Encoder.h> // https://github.com/madhephaestus/ESP32Encoder
 
/*################ ESP32 Pin asignement ################*/
  //--- Outputs ----//
    #define probe_tools_LED 5
    #define table_front_LED 2
    #define home_Machine_LED 19
    #define reset_LED 18
    #define Spindle_LED 21
    #define coolant_LED 3
    #define mist_LED 1
    #define chip_auger_LED 15
    #define optional_stop_LED 23
    #define single_line_LED 36
    #define light_LED 22
  //--- Inputs ----//
    #define scale_select 13
    #define axis_select 12
    #define feed_override_A 14
    #define feed_override_B 27
    #define feed_override_switch 26
    #define spindle_override_A 25
    #define spindle_override_B 33
    #define spindle_override_switch 32
    #define tool_select_A 35
    #define tool_select_B 34
    #define tool_select_switch 39
  //--- RS485 ----//
    #define RS485_TX 17
    #define RS485_RX 16
    #define RS485_RE_DE 4

/*################ Constant ################*/
  #define SLAVE_ID 1 // Used for modbus

/*################ Object Initialisation ################*/
  ModbusRTU mb; // Modbus Initialisation //
  ESP32Encoder encoder1; // Tool select encoder //
  ESP32Encoder encoder2; // Spindle Speed Override encoder //
  ESP32Encoder encoder3; // Feedrate Override encoder //
  
/*################ Variables ################*/
  int E1prev, E2prev, MPG_Axis_Select_val, MPG_Multiplicaton;
  int E1current = 100;
  int E2current = 100;
  bool FeedOverrideChanged = false;
  bool SpindleSpeedOverrideChanged = false;
  unsigned long currentMillis;
  unsigned long previousMillis = 0;
  bool isMoving = false;
  bool Debuging_Mode = false;

/*################ Functions ################*/
    //------------------- LED control -------------------//
      void LED_Control(){
        if(mb.Hreg(0) == 1){ // Probe Tool LED
          digitalWrite(probe_tools_LED,HIGH);
        }else{
          digitalWrite(probe_tools_LED,LOW);
        }
        
        if(mb.Hreg(1) == 1){ // Table Front LED
          digitalWrite(table_front_LED,HIGH);
        }else{
          digitalWrite(table_front_LED,LOW);
        }

        if(mb.Hreg(2) == 1){ // Home Machine LED
          digitalWrite(home_Machine_LED,HIGH);
        }else{
          digitalWrite(home_Machine_LED,LOW);
        }

        if(mb.Hreg(3) == 1){ // Reset LED
          digitalWrite(reset_LED,HIGH);
        }else{
          digitalWrite(reset_LED,LOW);
        }
        
        if(mb.Hreg(4) == 1){ // Spindle ON/OFF LED
          digitalWrite(Spindle_LED,HIGH);
        }else{
          digitalWrite(Spindle_LED,LOW);
        }
        
        if(mb.Hreg(5) == 1){ // Coolant ON/OFF LED
          digitalWrite(coolant_LED,HIGH);
        }else{
          digitalWrite(coolant_LED,LOW);
        }
        
        if(mb.Hreg(6) == 1){ // Mist ON/OFF LED
          digitalWrite(mist_LED,HIGH);
        }else{
          digitalWrite(mist_LED,LOW);
        }
        
        if(mb.Hreg(7) == 1){ // Chip Auger ON/OFF LED
          digitalWrite(chip_auger_LED,HIGH);
        }else{
          digitalWrite(chip_auger_LED,LOW);
        }
        
        if(mb.Hreg(8) == 1){ // Optional Stop LED
          digitalWrite(optional_stop_LED,HIGH);
        }else{
          digitalWrite(optional_stop_LED,LOW);
        }
        
        if(mb.Hreg(9) == 1){ // Single Line LED
          digitalWrite(single_line_LED,HIGH);
        }else{
          digitalWrite(single_line_LED,LOW);
        }

        if(mb.Hreg(9) == 1){ // Lights ON/OFF LED
          digitalWrite(light_LED,HIGH);
        }else{
          digitalWrite(light_LED,LOW);
        }
      }
    //------------------- SpindleSpeedOverRide -------------------//
      void SpindleSpeedOverRide() {  
        if (digitalRead(spindle_override_switch) == LOW) {
          E2current = 100;
          E2prev = encoder2.getCountRaw();
          SpindleSpeedOverrideChanged = true;
        }
        
        if (encoder2.getCountRaw() != E2prev) {
          if (encoder2.getCountRaw() < E2prev) {
            if (E2current > 0) {
              E2current = E2current - 10;
            }
          }
          if (encoder2.getCountRaw() > E2prev) {
            if (E2current < 300) {
              E2current = E2current + 10;
            }
          }
          E2prev = encoder2.getCountRaw();
          SpindleSpeedOverrideChanged = true;
        }

        // From Master to slave
        // mb.addHreg(15);    // Spindle Speed Override Watchdog
        // mb.addHreg(16);    // Spindle Speed Override UCCNC Value

        // From slave to master
        // mb.addHreg(54);    // Spindle Speed Override
        // mb.addHreg(55);    // Spindle Speed Override watchdog

        // Change made in UCCNC
        if(mb.Hreg(15) == 1){ // UCCNC watchdog signals a change
          E2current = mb.Hreg(16); // Place value from UCCNC into Control Panel
          mb.Hreg(55, 2); // Confirm to UCCNC that message is recieved
          while (mb.Hreg(15) != 0){ //Wait for confirmation from UCCNC
            mb.Hreg(54, mb.Hreg(16)); // Equalize Value in both registers
            mb.task(); yield(); // Keep modbus engine running
          }
          mb.Hreg(55, 0); // All is equalised, set watchdog to sleep!
        }

        // Change made in Control Panel
        if(SpindleSpeedOverrideChanged){ 
          SpindleSpeedOverrideChanged = false;
          mb.Hreg(55, 1); // Control panel watch dog sends signal to UCCNC that someting has changed
          while (mb.Hreg(15) != 2){ //Whait for confirmation from UCCNC that fields have been updated
            mb.Hreg(54, E2current); // Place new value from Control Panel in register
            mb.task(); yield(); // Keep modbus engine running
          }
          mb.Hreg(55, 0); // All is equalised, set watchdog to sleep!
        }
      }

    //------------------- FeedOverRide -------------------//
      /*void FeedOverRide() {
        if (digitalRead(tool_select_switch) == LOW) {
          E1current = 100;
          E1prev = encoder1.getCountRaw();
          FeedOverrideChanged = true;
        }
        
        if (encoder1.getCountRaw() != E1prev) {
          if (encoder1.getCountRaw() < E1prev) {
            if (E1current > 0) {
              E1current = E1current - 10;
            }
          }
          if (encoder1.getCountRaw() > E1prev) {
            if (E1current < 300) {
              E1current = E1current + 10;
            }
          }
          E1prev = encoder1.getCountRaw();
          FeedOverrideChanged = true;
        }

        // Change made in UCCNC
        if(mb.Hreg(17) == 1){ // UCCNC watchdog signals a change
          E1current = mb.Hreg(18); // Place value from UCCNC into Control Panel
          mb.Hreg(57, 2); // Confirm to UCCNC that message is recieved
          while (mb.Hreg(17) != 0){ //Whait for confirmation from UCCNC
            mb.Hreg(56, mb.Hreg(18)); // Equalize Value in both registers
            mb.task(); yield(); // Keep modbus engine running
          }
          mb.Hreg(57, 0); // All is equalised, set watchdog to sleep!
        }

        // Change made in Control Panel
        if(FeedOverrideChanged){ 
          FeedOverrideChanged = false;
          mb.Hreg(57, 1); // Control panel watch dog sends signal to UCCNC that someting has changed
          while (mb.Hreg(17) != 2){ //Whait for confirmation from UCCNC that fields have been updated
            mb.Hreg(56, E1current); // Place new value from Control Panel in register
            mb.task(); yield(); // Keep modbus engine running
          }
          mb.Hreg(57, 0); // All is equalised, set watchdog to sleep!
        }
      }*/

    //------------------- MPG_control_Select -------------------//
      int previous_MPG_Axis_Select_val, previous_MPG_Multiplicaton;
      void MPG_control_Select() {
        MPG_Axis_Select_val = analogRead(axis_select);
        MPG_Multiplicaton = analogRead(scale_select);
        if (MPG_Axis_Select_val != previous_MPG_Axis_Select_val || MPG_Multiplicaton != previous_MPG_Multiplicaton) {
          previous_MPG_Axis_Select_val = MPG_Axis_Select_val;
          if (MPG_Axis_Select_val < 500) {
            mb.Hreg(50, 1);
            // MPG Selected Axis = "X";
          } else if (MPG_Axis_Select_val >= 500 && MPG_Axis_Select_val < 1800) {
            mb.Hreg(50, 2);
            // MPG Selected Axis = "Y";
          } else if (MPG_Axis_Select_val >= 1800 && MPG_Axis_Select_val < 3300) {
            mb.Hreg(50, 3);
            // MPG Selected Axis = "Z";
          } else if (MPG_Axis_Select_val >= 3300) {
            mb.Hreg(50, 4);
            // MPG Selected Axis = "C";
          }

          previous_MPG_Multiplicaton = MPG_Multiplicaton;
          if (MPG_Multiplicaton < 500) {
            mb.Hreg(51, 4);
            // MPG Selected Multiplication = 0.1;
          } else if (MPG_Multiplicaton >= 500 && MPG_Multiplicaton < 1800) {
            mb.Hreg(51, 3);
            // MPG Selected Multiplication = 0.01;
          } else if (MPG_Multiplicaton >= 1800 && MPG_Multiplicaton < 3300) {
            mb.Hreg(51, 2);
            // MPG Selected Multiplication = 0.001;
          } else if (MPG_Multiplicaton >= 3300) {
            mb.Hreg(51, 1);
            // MPG Selected Multiplication = 0.0001;
          }
        }
      }

    //------------------- Debug -------------------//
      void Debug(){
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= 250) {
          previousMillis = currentMillis;
          Serial.printf("E1prev = %d E2prev = %d MPG_Axis_Select_val =  %d MPG_Multiplicaton = %d E1current = %d E2current = %d \n", E1prev, E2prev, MPG_Axis_Select_val, MPG_Multiplicaton, E1current, E2current);
        }
      }

/*################ Setup and loop ################*/
  void setup() {
    // Tool select, Feedrate override and Spindle override encoders relevant setup //
      ESP32Encoder::useInternalWeakPullResistors = true;
      encoder1.clearCount();
      encoder2.clearCount();
      encoder3.clearCount();
      encoder1.attachSingleEdge(tool_select_A, tool_select_B);
      encoder2.attachSingleEdge(spindle_override_A, spindle_override_B);
      encoder3.attachSingleEdge(feed_override_A, feed_override_B);
      E1prev = encoder1.getCountRaw(); // Tool select //
      E2prev = encoder2.getCountRaw(); // Spindle Override //
      E2prev = encoder3.getCountRaw(); // Feed Override //
      pinMode(tool_select_switch, INPUT_PULLUP); // Encoder 1 switch pin //
      pinMode(spindle_override_switch, INPUT_PULLUP); // Encoder 2 switch pin //
      pinMode(feed_override_switch, INPUT_PULLUP); // Encoder 3 switch pin //
    
    // Pin Modes //
      pinMode(probe_tools_LED, OUTPUT);
      pinMode(table_front_LED, OUTPUT);
      pinMode(home_Machine_LED, OUTPUT);
      pinMode(reset_LED, OUTPUT);
      pinMode(Spindle_LED, OUTPUT);
      pinMode(coolant_LED, OUTPUT);
      pinMode(mist_LED, OUTPUT);
      pinMode(chip_auger_LED, OUTPUT);
      pinMode(optional_stop_LED, OUTPUT);
      pinMode(single_line_LED, OUTPUT);
      pinMode(light_LED, OUTPUT);

    // Modbus stuf //
      Serial1.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX);
      mb.begin(&Serial1, RS485_RE_DE);
      mb.slave(SLAVE_ID);

    /*################ From Master to slave ################*/
      mb.addHreg(0);    // Probe Tool LED
      mb.addHreg(1);    // Table Front LED
      mb.addHreg(2);    // Home Machine LED
      mb.addHreg(3);    // Reset LED
      mb.addHreg(4);    // Spindle ON/OFF LED
      mb.addHreg(5);    // Coolant ON/OFF LED
      mb.addHreg(6);    // Mist ON/OFF LED
      mb.addHreg(7);    // Chip Auger ON/OFF LED
      mb.addHreg(8);    // Optional Stop LED
      mb.addHreg(9);    // Single Line LED
      mb.addHreg(10);    // Lights ON/OFF LED
      mb.addHreg(11);    // Hard MPG Watchdog
      mb.addHreg(12);    // IsMoving Watchdog
      mb.addHreg(13);    // Tool Select Watchdog
      mb.addHreg(14);    // Tool Select UCCNC Value
      mb.addHreg(15);    // Spindle Speed Override Watchdog
      mb.addHreg(16);    // Spindle Speed Override UCCNC Value
      mb.addHreg(17);    // Feed Override Watchdog
      mb.addHreg(18);    // Feed Override UCCNC Value
      
    /*################ From slave to master ################*/
      mb.addHreg(50);    // MPG_Axis_Select
      mb.addHreg(51);    // MPG_Multiplicaton
      mb.addHreg(52);    // Tool_Select
      mb.addHreg(53);    // Tool Select watchdog
      mb.addHreg(54);    // Spindle Speed Override
      mb.addHreg(55);    // Spindle Speed Override watchdog
      mb.addHreg(56);    // Feed Override
      mb.addHreg(57);    // Feed Override watchdog

    // Serial debug. Runs only if variable "Debuging_Mode" is set to true //
      if (Debuging_Mode) {
        Serial.begin(115200);
        Serial.println("Debug mode activated!");
      }
  }

  void loop() {
      LED_Control();
      SpindleSpeedOverRide();
      //FeedOverRide();
      MPG_control_Select();
      if (Debuging_Mode){ Debug(); }
      if(mb.Hreg(18) == 1){isMoving = true;}else{isMoving = false;}
      mb.task();
      yield();
      delay(1);
  }