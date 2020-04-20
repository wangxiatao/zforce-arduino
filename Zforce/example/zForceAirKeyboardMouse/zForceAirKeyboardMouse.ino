#include <Keyboard.h>
#include <Mouse.h>
#include <Zforce.h>
#include <Wire.h>

//#define WAIT_SERIAL_PORT_CONNECTION
#define TOUCH_BUFFER_SIZE 1
const int holdTime = 80;
const int keyboardBoundary = 1100;
long globalMillis = millis();

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

Zforce zf = Zforce();
uint8_t nTouches = 0;
TouchPoint tp;
Location previousLoc;
Location initialLoc;
int previousButtonState = HIGH; // for checking the state of a pushButton
volatile bool newTouchDataFlag = false;

void dataReadyISR() { newTouchDataFlag = true; }

bool isDataReady() { return digitalRead(PIN_NN_DR) == HIGH; }

bool touchInit()
{
    globalMillis = millis();
    pinMode(PIN_NN_DR, INPUT_PULLDOWN);
    pinMode(PIN_NN_RST, OUTPUT);
    digitalWrite(PIN_NN_RST, LOW);
    delay(10);
    digitalWrite(PIN_NN_RST, HIGH);
    
    Wire.begin();
    
    uint16_t timeout = 500;
    while (!isDataReady() && timeout--)
        delay(1);
    if(timeout == 0)
    {
        Serial.println("Timeout during getting boot complete");
        return false;
    }
    Message *msg = zf.GetMessage(); // Get bootcomplete message
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

    zf.Enable(true); // Send and read Enable
    timeout = 500;
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
    newTouchDataFlag = false;   // clear flag
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);

    Serial.print(F("Initialization took "));
    Serial.print(millis() - globalMillis);
    Serial.println("ms");
    return true;
}

void loopMouse()
{
    if(tp.loc.x > keyboardBoundary)
        return;

    if (tp.state == 0)
    {
        previousLoc.x = initialLoc.x = tp.loc.x;
        previousLoc.y = initialLoc.y = tp.loc.y;
        globalMillis = millis();
    }
    else
    {
        if (tp.state == 1)
        {
            if (tp.state == 1 && tp.loc.x <= keyboardBoundary)
            {
                if((millis() - globalMillis) >= holdTime)
                    Mouse.move((tp.loc.x - previousLoc.x) / 1, (tp.loc.y - previousLoc.y) / 1);
                previousLoc.x = tp.loc.x;
                previousLoc.y = tp.loc.y;
            }
        }
        else if (tp.state == 2)
        {
            if(millis() - globalMillis < holdTime)
            {
                Mouse.click(MOUSE_LEFT);
            }
            tp.state = 0xFF;
        }
    }
}

void loopKeyboard()
{
    int buttonState = tp.state < 2; // read the pushbutton:

    if ((buttonState != previousButtonState) && (buttonState == HIGH))
    {   // if the button state has changed, and it's currently pressed
        if (tp.loc.x > keyboardBoundary)
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
    Serial.begin(115200);
    
    #ifdef WAIT_SERIAL_PORT_CONNECTION
    while (!Serial) // 10Hz blink to indicate USB connection is waiting to establish
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    #endif
    
    Keyboard.begin(); // initialize control over the keyboard
    Mouse.begin();
    if(!touchInit())
        Serial.println(F("Touch init failed"));
}

void loop()
{
    digitalWrite(LED_BUILTIN, LOW);
    updateTouch();
    digitalWrite(LED_BUILTIN, HIGH);
    loopKeyboard();
    loopMouse();
}
