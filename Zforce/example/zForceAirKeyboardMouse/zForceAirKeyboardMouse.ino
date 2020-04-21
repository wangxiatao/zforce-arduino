#include <Keyboard.h>
#include <Mouse.h>
#include <Zforce.h>
#include <Wire.h>

//#define WAIT_SERIAL_PORT_CONNECTION   // uncomment to enforce waiting for serial port connection
                                        // (not only USB), requires active serial terminal/app to work
#define TOUCH_BUFFER_SIZE 1             // used by system to buffer touch, "1" means single touch buffer
const int holdTime = 80;                // sensitivity for mouse "left click", unit in milli-second
const int keyboardBoundary = 1100;      // separate mouse area and keyboard area on the x-axis
long globalMillis = millis();           // global timestamp

typedef struct Location
{
    uint16_t x;
    uint16_t y;
} Location;

typedef struct TouchPoint
{
    Location loc;
    uint8_t state;
} TouchPoint;

Zforce zf = Zforce();                   // instatiate touch instance
uint8_t nTouches = 0;
TouchPoint tp;
Location previousLoc;
Location initialLoc;
int previousButtonState = HIGH;         // for checking the state of a pushButton
volatile bool newTouchDataFlag = false; // new data flag works with data ready pin ISR

/*
 * Interrupt service callback for Data ready pin, set flag and get data in process()
 */
void dataReadyISR() { newTouchDataFlag = true; }

/*
 * Check if data is ready to fetch
 * @return true if there is data waiting to fetch
 */
bool isDataReady() { return digitalRead(PIN_NN_DR) == HIGH; }

/*
 * Sensor initialization and configuration
 * @return true if all steps are excuted successfully
 */
bool touchInit()
{
    globalMillis = millis();
    pinMode(PIN_NN_DR, INPUT_PULLDOWN);     // setup DataReady pin
    pinMode(PIN_NN_RST, OUTPUT);            // setup Reset pin
    digitalWrite(PIN_NN_RST, LOW);
    delay(10);
    digitalWrite(PIN_NN_RST, HIGH);         // Reset sensor
    
    Wire.begin();
    
    uint16_t timeout = 500;
    while (!isDataReady() && timeout--)     // Wait for BootComplete message to come
        delay(1);
    if(timeout == 0)
    {
        Serial.println("Timeout during getting boot complete");
        return false;
    }
    Message *msg = zf.GetMessage();         // Get bootcomplete message
    if (msg->type == MessageType::BOOTCOMPLETETYPE)
        Serial.println(F("Sensor connected"));
    else
    {
        zf.DestroyMessage(msg);
        Serial.print(F("BootComplete failed, unexpected sensor message. "));
        Serial.print("Expect ");
        Serial.print((uint8_t)MessageType::BOOTCOMPLETETYPE);
        Serial.print(", but get ");
        Serial.println((uint8_t)msg->type);
        return false;
    }
    zf.DestroyMessage(msg);

    zf.Enable(true);                        // Send and get Enable message
    timeout = 500;                          // Timeout is set to 500ms
    while (!isDataReady() && timeout--)
        delay(1);
    if(timeout == 0)
    {
        Serial.println("Timeout during enable");
        return false;
    }
    msg = zf.GetMessage();
    if (msg == NULL)
    {
        zf.DestroyMessage(msg);
        return false;
    }
    zf.DestroyMessage(msg);

    newTouchDataFlag = false;               // clear new data flag

    // enable interrupt
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);

    Serial.print(F("Initialization took "));
    Serial.print(millis() - globalMillis);
    Serial.println("ms");
    return true;
}

/*
 * Mouse event service to convert touch event to mouse event
 */
void loopMouse()
{
    if(tp.loc.x > keyboardBoundary)     // outside mouse area, do nothing
        return;

    if (tp.state == 0)                  // touch "down", buffer the location
    {
        previousLoc.x = initialLoc.x = tp.loc.x;
        previousLoc.y = initialLoc.y = tp.loc.y;
        globalMillis = millis();
    }
    else
    {
        if (tp.state == 1)              // touch "move"
        {
            if (tp.state == 1 && tp.loc.x <= keyboardBoundary)  // within mouse area
            {
                if((millis() - globalMillis) >= holdTime)   // not a "left click"
                    Mouse.move((tp.loc.x - previousLoc.x) / 1, (tp.loc.y - previousLoc.y) / 1);
                previousLoc.x = tp.loc.x;
                previousLoc.y = tp.loc.y;
            }
        }
        else if (tp.state == 2)         // touch "up"
        {
            if(millis() - globalMillis < holdTime)  // mouse "left click", sensitivity 
                                                    // can be tuned by changing "holdTime"
            {
                Mouse.click(MOUSE_LEFT);
            }
            tp.state = 0xFF;            // reset state machine
        }
    }
}

/*
 * Keyboard event service to convert touch event to keybard event
 */
void loopKeyboard()
{
    int buttonState = tp.state < 2;     // touch "down" or "move" represents keyboard state LOW
                                        // and touch "up" represents keyboard state HIGH

    if ((buttonState != previousButtonState) && (buttonState == HIGH))  // if the button state has changed, 
                                                                        // and it's currently pressed
    {   
        if (tp.loc.x > keyboardBoundary) // below conditions can be changed to fit your purpose
        {
            if (tp.loc.y < 250)
                Keyboard.print('A');
            else if (tp.loc.y < 500)
                Keyboard.print('B');
            else if (tp.loc.y < 750)
                Keyboard.print('C');
            else if (tp.loc.y < 1000)
                Keyboard.print('D');
            else
                Keyboard.print('E');
        }
        else
        {
            // May do something to catch the rest of the cases
        }
    }
    previousButtonState = buttonState;  // save the current button state for comparison next time
}

/*
 * Get touch events
 * @return 0 if no touch event is present, -1 if message is not touch message, otherwise the number of touches received
 */
uint8_t updateTouch()
{
    if (newTouchDataFlag == false)
        return 0;

    newTouchDataFlag = false;
    Message *touch = zf.GetMessage();
    if (touch != NULL)
    {
        if (touch->type == MessageType::TOUCHTYPE)
        {
            auto size = ((TouchMessage *)touch)->touchCount;
            nTouches = size >= TOUCH_BUFFER_SIZE ? TOUCH_BUFFER_SIZE : size;
            for (uint8_t i = 0; i < nTouches; i++)
            {
                tp.loc.x = ((TouchMessage *)touch)->touchData[0].x;
                tp.loc.y = ((TouchMessage *)touch)->touchData[0].y;
                tp.state = ((TouchMessage *)touch)->touchData[0].event;
            }
            zf.DestroyMessage(touch);
            return nTouches;
        }
        zf.DestroyMessage(touch);
    }
    return -1;
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(115200);       // start serial port, this is optional and is 
                                // NOT required to run mouse and keybaord
                                // services, comment out to skip
    
    #ifdef WAIT_SERIAL_PORT_CONNECTION
    while (!Serial)             // 10Hz blink to indicate USB connection is waiting to establish
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    #endif
    
    Keyboard.begin();           // initialize keyboard
    Mouse.begin();              // initialize mouse
    if(!touchInit())
        Serial.println(F("Touch init failed"));
}

void loop()
{
    digitalWrite(LED_BUILTIN, LOW); // blink when new touch event is received
    updateTouch();
    digitalWrite(LED_BUILTIN, HIGH);
    loopKeyboard();
    loopMouse();
}
