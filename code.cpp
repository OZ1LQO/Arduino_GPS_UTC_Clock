    /**
    * Display time & date and locator from a GPS receiver on an LCD Display.
    *
    * inspired by http://quaxio.com/arduino_gps/
    * 
    * The Maidenhead calculation routine was inspired by Wikipedia:
    *https://en.wikipedia.org/wiki/Maidenhead_Locator_System
    *
    *
    * Added Battery monitor
    * OZ1LQO 2014.06.09
    *
    * Started on Locator functionality 2014.06.22
    * Gives and 'LO' indication after 'UTC' if location is aquired.
    * Pushing the POS button will display the raw Lat/Lon from the GPS
    * Pushing the LOC button will display the current Maidenhead locator
    */
     
    #include <LiquidCrystal.h>
    #include <string.h>
    #include <ctype.h>
    #include <stdlib.h>
    #include <math.h>
     
    LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
     
    int rxPin = 0; // RX pin
    int txPin = 1; // TX pin
    int batpin = A0; //Battery monitor input
    int POSselect = 8; //Pin select for Position switch
    int LOCselect = 9; //Pin select for Locator switch
    int byteGPS=-1;
    char cmd[7] = "$GPRMC";
    int counter1 = 0; // counts how many bytes were received (max 300)
    int counter2 = 0; // counts how many commas were seen
    int offsets[13];
    char buf[300] = "";
     
     
    //defining the calculation function to be called with the coordinate parameters
    void locator(String lat, String NS, String lon, String EW){
    int i=0;
    String deg_lat="";
    String min_lat="";
    String deg_lon="";
    String min_lon="";



    //dividing up the input strings
    for(i=0; i<2;i++){
        deg_lat+=lat[i];
    }

    for(i=2; i<9;i++){
        min_lat+=lat[i];
    }

    for(i=0; i<3;i++){
        deg_lon+=lon[i];
    }

    for(i=3; i<10;i++){
        min_lon+=lon[i];
    }



    //convert it to double floats
    double deg_lat_long = ::atof(deg_lat.c_str());
    double min_lat_long = ::atof(min_lat.c_str());
    double deg_lon_long = ::atof(deg_lon.c_str());
    double min_lon_long = ::atof(min_lon.c_str());



    //assembling the digits and securing right sign
    double lat_final = deg_lat_long + (min_lat_long/60);
    if (NS=="S"){
        lat_final*=-1;
    }

    double lon_final = deg_lon_long + (min_lon_long/60);
    if (EW=="W"){
        lon_final*=-1;
    }


    //Define output string
    String loc = "";

    //Convert A, 0 and a to ASCII integer values
    char AA = 'A';
    int A = AA;
    char OO = '0';
    int O = OO;
    char aa = 'a';
    int a = aa;

    lon_final+=180;
    lat_final+=90;


    //Find the main grid
    loc+=char(A+int(lon_final/20));
    loc+=char(A+int(lat_final/10));


    //Find the sub coordinates
    loc+=char(O+int(fmod(lon_final,20)/2));
    loc+=char(O+int(fmod(lat_final,10)/1));

    //Find the close in sub grid
    loc+=char(a + int((lon_final - (int(lon_final/2)*2)) / (5.0/60)));
    loc+=char(a + int((lat_final - (int(lat_final/1)*1)) / (2.5/60)));

    //Print locator on LCD Screen
    lcd.clear();
    lcd.print("Locator:");
    lcd.setCursor(0, 1);
    lcd.print(loc);
    
} 
    
    //define battery measurement function
   // print the measurement on the display 
    void battery(int input){
      float voltage = analogRead(input)/1024.0*5.0*2.0;  // read battery voltage, compensate for resistor divider
      // use two 10k resistors to divide the 9V battery voltage into the range of the ADC
      lcd.print(" ");
      lcd.print(voltage);
      lcd.print("V");
      if (voltage < 7.0){  //if low battery, print one '!'
        lcd.print("!");
        }
      if (voltage < 6.5){  //if very low battery, print '!!'
        lcd.print("!");
      }  
      
    }
      
    /**
    * Setup display and gps
    */
    void setup() {
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    pinMode(POSselect, INPUT); //define input pin for Position Switch
    pinMode(LOCselect, INPUT); //define input pin for Locator Switch
    Serial.begin(4800);
    //Greeting
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("OZ1LQO UTC Clock");
    lcd.setCursor(0, 1);
    lcd.print("Rev 1.0 070714");
    delay(3000);
    lcd.clear();
    
    lcd.print("waiting for gps");
    offsets[0] = 0;
    reset();
    }
     
    //Reset counters
    void reset() {
    counter1 = 0;
    counter2 = 0;
    }
     
    //Measures the size of the GPS data segment. Used to determine if a valid segment has been formed
    int get_size(int offset) {
    return offsets[offset+1] - offsets[offset] - 1;
    }
     
    //Main part
    //Put the received data from the GPS in a buffer, then print time and date.
    // Handles Battery monitor and position and locator calls
    int handle_byte(int byteGPS) {
    buf[counter1] = byteGPS;
    Serial.print((char)byteGPS);//print the GPS data on the serial port for debug
    counter1++;
    if (counter1 == 300) {
    return 0;
    }
    if (byteGPS == ',') {
    counter2++;
    offsets[counter2] = counter1;
    if (counter2 == 13) {
    return 0;
    }
    }
    if (byteGPS == '*') {
    offsets[12] = counter1;
    }
     
    // Check if we got a <LF>, which indicates the end of line
    if (byteGPS == 10) {
    // Check that we got 12 pieces, and that the first piece is 6 characters
    if (counter2 != 12 || (get_size(0) != 6)) {
    return 0;
    }
     
    // Check that we received the $GPRMC string
    for (int j=0; j<6; j++) {
    if (buf[j] != cmd[j]) {
    return 0;
    }
    }
     
    //TIME and DATE Checks
    // Check that time is well formed
    if (get_size(1) != 10) {
    return 0;
    }
     
    // Check that date is well formed
    if (get_size(9) != 6) {
    return 0;
    }
    
    
     
    // print time
    lcd.clear();
    for (int j=0; j<6; j++) {
    lcd.print(buf[offsets[1]+j]);
    if (j==1) {
    lcd.print("h");
    } else if (j==3) {
    lcd.print("m");
    } else if (j==5) {
    lcd.print("s UTC");
    
    //Check for Valid Position data and print LO indication if so
    //After that, if user presses the LOC button (pin9), call the locator function
    //and read out the locator
    if (get_size(3) == 9 && get_size(4) == 1 && get_size(5) == 10 && get_size(6) == 1){
    lcd.print(" LO");
    
    //Preparing for the Locator readout, if the LOC pin is pressed at this point
    //This way, the locator button will ONLY work if correct position data has been formed
    //Not causing the SW to crash due to garbled positional data
    //(Pushing the POS button will show the garbled data)
    if (digitalRead(LOCselect)==HIGH){
      
      String lat = "";
      String NS = "";
      String lon = "";
      String EW = "";
      //Get Latitude
      for (int j=0; j<9; j++) {
      lat+=buf[offsets[3]+j];
      }
      // Get N/S
      NS+=buf[offsets[4]];
      
      
      //Get Longtitude
      for (int j=0; j<10; j++) {
      lon+=buf[offsets[5]+j];
      }
      //Get E/W
      EW+=buf[offsets[6]];
      
      //call locator function with the position data
      locator(lat,NS,lon,EW);
      
      //Stay here while the button is pressed  
      while(digitalRead(LOCselect)==HIGH){}
    }
    }
    
  }
    }
     
    // print date
    lcd.setCursor(0, 1);
    for (int j=0; j<6; j++) {
    lcd.print(buf[offsets[9]+j]);
    if (j==1 || j==3) {
    lcd.print(".");
    }
    }
    
    //print battery voltage
    battery(batpin); 
    
    // Check if position switch is enabled
    // If so, print position info
    if (digitalRead(POSselect)==HIGH){
      
      lcd.clear();
      //Print Latitude
      for (int j=0; j<9; j++) {
      lcd.print(buf[offsets[3]+j]);
      }
      lcd.print(buf[offsets[4]]);
      
      
      //Print Longtitude
      lcd.setCursor(0, 1);
      for (int j=0; j<10; j++) {
      lcd.print(buf[offsets[5]+j]);
      }
      lcd.print(buf[offsets[6]]);
      
      //Stay here until the button is released      
      while(digitalRead(POSselect)==HIGH){}
      
    }
      
    
    
    
    return 0;
    }
    return 1;
    }
     
    /**
    * Main loop
    */
    void loop() {
    byteGPS=Serial.read(); // Read a byte of the serial port
    if (byteGPS == -1) { // See if the port is empty yet
    delay(100);
    } else {
    if (!handle_byte(byteGPS)) {
    reset();
    }
    }
    }