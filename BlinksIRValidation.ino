/*
   Blinks IR Offset Test
   
   Use this test to validate IR comms on all 6 sides with 1.5mm offset (left/right)
   There are two modes, REPORTER and TESTER, the app boots into TESTER. We only need one
   or two REPORTERS (best case is one for left offset, one for right offset)
   The REPORTER shows the status on a face by face basis for the TESTER.
   GREEN = PASS
   RED = FAIL
   
   
   MAKE A REPORTER BLINK
   To use this application correctly, you must modify a Blink
   turning its button into a sensor for correct offset
   1. With patch wire, add a momentary switch to the existing momentary switch
   To use 

            |     _______         |
            |    /       \        |
            |   / MOUNTED \       |
            |   \         /       |
            |____\___•___/___     |
                    _______  |    |
                   /   º   \ |
  SLIDE BLINK --> /         \¢ <-- BUTTON HERE
  TOWARD BUTTON   \         /|
                   \_______/ |____|

   HOW TO TEST BLINKS
   
   Verify that all 6 sides are sending and receiving
   to the tolerances of placement
   1.5mm left/right alignment


   by Jonathan Bobrow
   07.10.2019
*/

#define VALIDATION_DURATION 2000
#define SUCCESS_VAL 63

enum Mode {
  REPORTING,
  TESTING
};

enum Status {
  NONE,
  SEND,
  RECEIVE,
  PASS,
  FAIL
};

byte mode = TESTING;
byte stat[6];
byte testingFace = 0;
byte reportingFace = 0;

bool didPassTest = false;

uint32_t buttonPressedTime ;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  if (buttonMultiClicked()) {
    mode = !mode; // switch between testing and reporting
  }

  if (buttonDoubleClicked()) {
    if (mode == REPORTING) {
      resetStatus();
    }
  }

  switch (mode) {

    case TESTING:   testingLoop();    break;
    case REPORTING: reportingLoop();  break;

    default: break;
  }
}

void testingLoop() {

  if (buttonPressed()) {
    didPassTest = false;
  }

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      // has neighbor
      byte data = f + 10;
      setValueSentOnFace(data, f);

      if (getLastValueReceivedOnFace(f) == SUCCESS_VAL) {
        didPassTest = true;
      }
      setColorOnFace(YELLOW, f);
    }
    else {
      setValueSentOnFace(f, f);
      setColorOnFace(OFF, f);
    }
  }

  if (didPassTest) {
    setColor(GREEN);
  }
}

/*
   The sequence should be as follows:
   1. See neighbor - SEND
   2. See neighbor ack - RECEIVE
   3. Button down... hold this with only RECEIVE
   4. If passed threshold, PASS
   5. If back to SEND or not seen, FAIL
*/
void reportingLoop() {
  // use the button to validate the travel distance
  if (buttonPressed()) {
    buttonPressedTime = millis();
  }

  if (!isValueReceivedOnFaceExpired(testingFace)) {
    byte data = getLastValueReceivedOnFace(testingFace);
    reportingFace = data % 10;
    bool ack = data / 10;

    if (stat[reportingFace] == PASS || stat[reportingFace] == FAIL) {
      // no need to update
    }
    else {

      if ( ack ) {
        // both send and receive working
        stat[reportingFace] = RECEIVE;

        if (buttonDown() && millis() - buttonPressedTime > VALIDATION_DURATION) {
          stat[reportingFace] = PASS;
        }
      }
      else {
        // only send is working
        if (stat[reportingFace] == NONE) {
          stat[reportingFace] = SEND;
        }
        else {
          if (buttonDown()) {
            stat[reportingFace] = FAIL;
          }
        }
      }
    }
  }
  else {
    if (stat[reportingFace] == PASS || stat[reportingFace] == FAIL) {
      // no need to update
    }
    else {
      // no neighbor or
      // not seeing tester
      // if button down, this means a fail
      // if not, this is simply nothing
      if (buttonDown() && stat[reportingFace] != NONE) {
        stat[reportingFace] = FAIL;
      }
      else {
        stat[reportingFace] = NONE;
      }
    }
  }

  bool allPass = true;
  bool allNone = true;

  // display status
  FOREACH_FACE(f) {

    if (stat[f] != PASS) allPass = false;
    if (stat[f] != NONE) allNone = false;

    switch (stat[f]) {
      case NONE:    setColorOnFace(OFF, f);      break;
      case SEND:    setColorOnFace(YELLOW, f);   break;
      case RECEIVE: setColorOnFace(BLUE, f);     break;
      case PASS:    setColorOnFace(GREEN, f);    break;
      case FAIL:    setColorOnFace(RED, f);      break;
    }
  }


  // celebrate all pass
  if (allPass) {
    setColor(dim(GREEN, sin8_C((millis() / 2) % 255)));
    setValueSentOnAllFaces(SUCCESS_VAL);
  }
  else if (allNone) {
    // show sensing side
    setColorOnFace(dim(WHITE, sin8_C((millis() / 6) % 255)), testingFace);
  }

  // if alone, we stop sending success
  if(isAlone() && allPass) {
    resetStatus();
  }
}

void resetStatus() {
  FOREACH_FACE(f) {
    stat[f] = NONE;
  }
  setValueSentOnAllFaces(0);
}
