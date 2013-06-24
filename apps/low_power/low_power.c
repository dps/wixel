/** low_power app:

This app uses the CC2511's Sleep Timer to enter and leave low power
mode periodically.

*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>


/* FUNCTIONS ******************************************************************/

/* the function that puts the system to sleep (PM2) and configures sleep timer to
   wake it again in 250 seconds.*/
void makeAllOutputs() {
    int i;
    for (i=0; i < 16; i++) {
        setDigitalOutput(i, LOW);
    }
}

// catch Sleep Timer interrupts
ISR (ST, 0) {
    IRCON &= ~0x80;  // clear IRCON.STIF
    SLEEP &= ~0x02;  // clear SLEEP.MODE
    IEN0 &= ~0x20;   // clear IEN0.STIE
    WORIRQ &= ~0x11; // clear Sleep timer EVENT0_MASK
                     // and EVENT0_FLAG
    WORCTRL &= ~0x03; // Set timer resolution back to 1 period.
    }

void goToSleep (uint16 seconds) {
    unsigned char temp;
    // The wixel docs note that any input pins consume ~30uA
    makeAllOutputs();

    IEN0 |= 0x20; // Enable global ST interrupt [IEN0.STIE]
    WORIRQ |= 0x10; // enable sleep timer interrupt [EVENT0_MASK]

    /* the sleep mode i've chosen is PM2.  According to the CC251132 datasheet,
       typical power consumption from the SoC should be around 0.5uA */
    /*The SLEEP.MODE will be cleared to 00 by HW when power
      mode is entered, thus interrupts are enabled during power modes.
      All interrupts not to be used to wake up from power modes must
      be disabled before setting SLEEP.MODE!=00.*/
    SLEEP |= 0x02;                  // SLEEP.MODE = PM2

    // Reset timer, update EVENT0, and enter PM2
    // WORCTRL[2] = Reset Timer
    // WORCTRL[1:0] = Sleep Timer resolution
    //                00 = 1 period
    //                01 = 2^5 periods
    //                10 = 2^10 periods
    //                11 = 2^15 periods

    // t(event0) = (1/32768)*(WOREVT1 << 8 + WOREVT0) * timer res
    // e.g. WOREVT1=0,WOREVT0=1,res=2^15 ~= 1 second 

    WORCTRL |= 0x04;  // Reset
    // Wait for 2x+ve edge on 32kHz clock
    temp = WORTIME0;
    while (temp == WORTIME0) {};
    temp = WORTIME0;
    while (temp == WORTIME0) {};

    WORCTRL |= 0x03; // 2^5 periods
    WOREVT1 = (seconds >> 8);
    WOREVT0 = (seconds & 0xff);

    PCON |= 0x01; // PCON.IDLE = 1;
    }

void updateLeds()
{
    usbShowStatusWithGreenLed();
}


void main()
{
    uint16 i,secs = 0;
    systemInit();
    usbInit();

    LED_RED(1);

    // Keep the USB bus alive for 10 seconds after power on to
    // make reprogramming easy.
    for (i = 0; i < 1000; i++) {
        boardService();
        usbComService();
        delayMs(10);
    }

    LED_RED(0);

    while(1)
    {
        boardService();
    
        LED_YELLOW(1);
        for (i = 0; i < 1000; i++) {delayMs(1);};
        LED_YELLOW(0);

        goToSleep(secs);
        secs++;
    }
}
