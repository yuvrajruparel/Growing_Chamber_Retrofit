void setup()
{
  pinMode(9, OUTPUT); // fan PWM control pin
  pinMode(6, OUTPUT); // control pin for fan indicator light
  pinMode(7, OUTPUT); // control pin for cycle indicator light
  pinMode(8, OUTPUT); //enable pin (controlls current flow to fan)
}

unsigned long start = millis(); //the time in ms the code started 
unsigned long elapsed = 0; //the time in ms the code has been running
int testIndex = 0; //the test currently being run or next to begin

/* paralell list items correspond to
  	test start time (in minutes)
  	test duration (in minutes)
  	test fan speed (percent) 
*/
float startTimes[] = 	{0};
float durations[] = 	{.25};
float motorSpeeds[] = 	{1};

int numTests = sizeof(startTimes) / sizeof(startTimes[0]); //the number of tests

int indicatorCycleCount = 0; //number of times the indicator light has cycled on/off 
unsigned long indicatorStartTime = 0; //holds the start time of each blinking cycle

void loop()
{
  //update the amount of time elapsed since start;
  elapsed = millis() - start;

  if(testIndex < numTests){
    //run the current test cycle, multiplying time values by number of millisconds in a minute
    testCycle(startTimes[testIndex] * 60000, durations[testIndex] * 60000, motorSpeeds[testIndex]);
     
    //Blink the indicator on/off at a fixed rate of 200ms for testCycle +1 times
    if(elapsed - indicatorStartTime >= 300){
    	indicatorCycleCount++;
      	indicatorStartTime = elapsed;
    }     
    if(indicatorCycleCount % 2 == 0 && indicatorCycleCount <= testIndex*2 ){
    	digitalWrite(7, HIGH);
    } else {
    	digitalWrite(7, LOW);
    } 
    //After blinking testCycle +1 times, turn off indicator for an additional number of cycles and then restart
    if(indicatorCycleCount > 5 + (testIndex*2)){
      indicatorCycleCount = 0;
    }
  } else {
  	//all tests complete, solid indicator light
    digitalWrite(7, HIGH);
  }
}

void testCycle(unsigned long startTime, unsigned long duration, float motorSpeed){
  if(elapsed >= startTime && elapsed <= startTime + duration){
    //run fan
    analogWrite(9, 255 * motorSpeeds[testIndex]);
    digitalWrite(8, HIGH); //turn on enable pin

    //turn on fan indicator light
    digitalWrite(6, HIGH);
    
  } else if (elapsed > startTime + duration){
    //turn off fan indicator light
    digitalWrite(6, LOW);
    digitalWrite(8, LOW); //turn off enable pin
    
    //move onto next test,
  	analogWrite(9, 0);
    testIndex++;
  } else {
     digitalWrite(8, LOW); //turn off enable pin in weird edge case
  }
}