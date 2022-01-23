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
    #define feed_override_A 27
    #define feed_override_B 14
    #define feed_override_switch 26
    #define spindle_override_A 33
    #define spindle_override_B 25
    #define spindle_override_switch 32
    #define tool_select_A 34
    #define tool_select_B 35
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
  int E1prev, E2prev, E3prev, MPG_Axis_Select_val, MPG_Multiplicaton,PreviousToolSelectOnModbus, PreviousSpindleSpeedOnModbus, PreviousFeedOnModbus, PreviousPulley;
  unsigned long currentMillis;
  unsigned long previousMillis = 0;
  bool isMoving = false;
  bool Debuging_Mode = false;
  int ToolSelectValue = 1;
  int SpindleSpeedOverrideValue = 100;
  int FeedOverrideValue = 100;

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

    //------------------- ToolSelect -------------------//
      void ToolSelect() {
        if (encoder1.getCountRaw() != E1prev) {
          if (encoder1.getCountRaw() < E1prev) {
            if(ToolSelectValue > 0){ToolSelectValue = ToolSelectValue - 1;}
            if(ToolSelectValue == 0){ToolSelectValue = 21;}
          }
          if (encoder1.getCountRaw() > E1prev) {
            if(ToolSelectValue <= 21){ToolSelectValue = ToolSelectValue + 1;}
            if(ToolSelectValue == 22){ToolSelectValue = 1;}
          }
          E1prev = encoder1.getCountRaw();
          mb.Hreg(53, ToolSelectValue);
        }
        
        if (digitalRead(tool_select_switch) == LOW) {
          mb.Hreg(52, 1); // Activate tool change
        }else{
          mb.Hreg(52, 0); // Activate tool change
        }

        // Look for a change fromm UCCNC
        //if(PreviousToolSelectOnModbus != mb.Hreg(13)){
          //ToolSelectValue = PreviousToolSelectOnModbus;
          //mb.Hreg(53, ToolSelectValue);
          //PreviousToolSelectOnModbus = mb.Hreg(13);
        //}
      }

    //------------------- SpindleSpeedOverRide -------------------//

      int CalculateOverride(){
        int CommandedSpindleSpeed = mb.Hreg(17);
        int calculatedSpeed = CommandedSpindleSpeed * SpindleSpeedOverrideValue / 100;
        return calculatedSpeed;
      }

      void SpindleSpeedOverRide(){ 
        int pulleyRatio = mb.Hreg(16); 
        if(PreviousPulley != pulleyRatio){
          PreviousPulley = pulleyRatio;
          SpindleSpeedOverrideValue = 100;
          mb.Hreg(55, SpindleSpeedOverrideValue);
        }
        
        if (encoder2.getCountRaw() != E2prev){
          if (encoder2.getCountRaw() < E2prev){
            if(SpindleSpeedOverrideValue > 0){
              if(pulleyRatio == 1 || (pulleyRatio == 2 && CalculateOverride() > 2500)){
                SpindleSpeedOverrideValue = SpindleSpeedOverrideValue - 10;
              }
            }
          }
          if (encoder2.getCountRaw() > E2prev){
            if(SpindleSpeedOverrideValue < 300){
              if(pulleyRatio == 2 || (pulleyRatio == 1 && CalculateOverride() <= 2500))
              SpindleSpeedOverrideValue = SpindleSpeedOverrideValue + 10;
            }
          }
          E2prev = encoder2.getCountRaw();
          mb.Hreg(55, SpindleSpeedOverrideValue);
        }
        if (digitalRead(spindle_override_switch) == LOW){
          SpindleSpeedOverrideValue = 100;
          mb.Hreg(55, SpindleSpeedOverrideValue);
        }

        // Look for a change fromm UCCNC
        int NewSSOValiewFromUCCNC = mb.Hreg(14);
        if(PreviousSpindleSpeedOnModbus != NewSSOValiewFromUCCNC){
          SpindleSpeedOverrideValue = NewSSOValiewFromUCCNC;
          mb.Hreg(55, SpindleSpeedOverrideValue);
          PreviousSpindleSpeedOnModbus = NewSSOValiewFromUCCNC;
        }
      }

    //------------------- FeedOverRide -------------------//
      void FeedOverRide() {
        if (encoder3.getCountRaw() != E3prev) {
          if (encoder3.getCountRaw() < E3prev) {
            if(FeedOverrideValue > 0){
              FeedOverrideValue = FeedOverrideValue - 10;
            }
          }
          if (encoder3.getCountRaw() > E3prev) {
            if(FeedOverrideValue < 300){
              FeedOverrideValue = FeedOverrideValue + 10;
            }
          }
          E3prev = encoder3.getCountRaw();
          mb.Hreg(57, FeedOverrideValue);
        }
        if (digitalRead(feed_override_switch) == LOW) {
          FeedOverrideValue = 100;
          mb.Hreg(57, FeedOverrideValue);
        }

        // Look for a change fromm UCCNC
        int FROValueFromUCCNC = mb.Hreg(15);
        if(PreviousFeedOnModbus != FROValueFromUCCNC){
          FeedOverrideValue = FROValueFromUCCNC;
          mb.Hreg(57, FeedOverrideValue);
          PreviousFeedOnModbus = FROValueFromUCCNC;
        }

      }

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
          Serial.printf("E1prev = %d E2prev = %d MPG_Axis_Select_val =  %d MPG_Multiplicaton = %d \n", E1prev, E2prev, MPG_Axis_Select_val, MPG_Multiplicaton);
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
      E3prev = encoder3.getCountRaw(); // Feed Override //
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
      mb.addHreg(13);    // Tool select changed on UCCNC
      mb.addHreg(14);    // Spindle Speed Override changed on UCCNC
      mb.addHreg(15);    // Feed Override changed on UCCNC
      mb.addHreg(16);    // Currently selected pulley
      mb.addHreg(17);    // Setspindlespeed
      
      PreviousToolSelectOnModbus = mb.Hreg(13);
      PreviousSpindleSpeedOnModbus = mb.Hreg(14);
      PreviousFeedOnModbus = mb.Hreg(15);
      PreviousPulley = mb.Hreg(16);
      
    /*################ From slave to master ################*/
      mb.addHreg(50);    // MPG_Axis_Select
      mb.addHreg(51);    // MPG_Multiplicaton
      mb.addHreg(52);    // Tool Select Switch
      mb.addHreg(53);    // Tool Select
      mb.addHreg(54);    // Spindle Speed Override Step (Unused)
      mb.addHreg(55);    // Spindle Speed Override
      mb.addHreg(56);    // Feed Override Step (Unused)
      mb.addHreg(57);    // Feed Override

      mb.Hreg(55, 100); // initially set the value to 100% on modbus
      mb.Hreg(57, 100); // initially set the value to 100% on modbus

    // Serial debug. Runs only if variable "Debuging_Mode" is set to true //
      if (Debuging_Mode) {
        Serial.begin(115200);
        Serial.println("Debug mode activated!");
      }
  }

  void loop() {
      LED_Control();
      ToolSelect();
      SpindleSpeedOverRide();
      FeedOverRide();
      MPG_control_Select();
      if (Debuging_Mode){ Debug(); }
      if(mb.Hreg(18) == 1){isMoving = true;}else{isMoving = false;}
      mb.task();
      yield();
      delay(1);
  }