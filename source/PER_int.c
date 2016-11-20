/****************************************************************
 * FILENAME:     PER_int.c
 * DESCRIPTION:  periodic interrupt code
 ****************************************************************/
#include    "PER_int.h"
#include    "TIC_toc.h"
#include	"math.h"
#include 	"GPIO.h"

//for ADC variables
float u_faza1 = 0.0; //phase 1 voltage [V]
float u_faza2 = 0.0; //phase 2 voltage [V]
float u_faza3 = 0.0; //phase 3 voltage [V]
float i_faza1 = 0.0; //phase 1 current [A]
float i_faza2 = 0.0; //phase 2 current [A]
float i_faza3 = 0.0; //phase 3 current [A]
float u_dc = 0.0; //DC output voltage [V]
float i_dc = 0.0; //DC output current [A]
float adc_pot1 = 0.0; //potentiometer 1 position [0.0-1.0]
float adc_pot2 = 0.0; //potentiometer 2 position [0.0-1.0]

//GAIN/OFFSET values
float u_faza1_gain = 0.0042502450;
float u_faza1_offset = 0.4616595883;
float u_faza2_gain = 0.004459308;
float u_faza2_offset = -0.044593088;
float u_faza3_gain = 0.004456824;
float u_faza3_offset = -0.102506963;
float u_dc_gain = 0.004383561;
float u_dc_offset = 0.298082191;

// CPU load evaluation
float   cpu_load  = 0.0;
long    interrupt_cycles = 0;

// interrupt overflow (timeout) counter
int interrupt_overflow_counter = 0;

/**************************************************************
* variables for synchronization
**************************************************************/

/**************************************************************
 * Interrupt for regulation
 **************************************************************/
#pragma CODE_SECTION(PER_int, "ramfuncs");
void interrupt PER_int(void)
{
    // acknowledge interrupt:
    // Clear INT flag - ePWM1
    EPwm1Regs.ETCLR.bit.INT = 1;
    // Clear INT flag- PIE
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

    // start timer
    interrupt_cycles = TIC_time;
    TIC_start();

    // calculate CPU load
    cpu_load = (float)interrupt_cycles / (CPU_FREQ/SWITCH_FREQ);

    // wait for the ADC to finish conversion
    ADC_wait();

    // INTERRUPT CODE HERE
    // INTERRUPT CODE HERE
    // INTERRUPT CODE HERE


	// ADC calibration testing
	u_faza1 = U_FAZA1*u_faza1_gain+u_faza1_offset;
	u_faza2 = U_FAZA2*u_faza2_gain+u_faza2_offset;
	u_faza3 = U_FAZA3*u_faza3_gain+u_faza3_offset;
	i_faza1 = I_FAZA1;
	i_faza2 = I_FAZA2;
	i_faza3 = I_FAZA3;
	u_dc = U_DC*u_dc_gain+u_dc_offset;
	i_dc = I_DC;
	adc_pot1 = ADC_POT1/4095.0;
	adc_pot2 = ADC_POT2/4095.0;

	// pin toggling to test interrupt frequency
	GPIO_Set(GPIO_LED_Y); //toggle pin #24 - Yellow LED

	//__asm ("      ESTOP0");

    // save values in buffer
    DLOG_GEN_update();
    GPIO_Toggle(GPIO_LED_Y);

    /* check for interrupt while this interrupt is running -
     * if true, there is something wrong - if we count 10 such
     * occurances, something is seriousely wrong!
     */
    if (EPwm1Regs.ETFLG.bit.INT == TRUE)
    {
        // increase error counter
        interrupt_overflow_counter = interrupt_overflow_counter + 1;

        // if counter reaches 10, stop the operation
        if (interrupt_overflow_counter >= 10)
        {
            asm(" ESTOP0");
        }
    }

    // stop timer
    TIC_stop();

}   // end of PWM_int

/**************************************************************
 * Function for interrupt initialization
 **************************************************************/
void PER_int_setup(void)
{

    // Interrupt triggering
    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;    // trigger on period
    EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;         // at each first case
    EPwm1Regs.ETCLR.bit.INT = 1;                // clear possible flag
    EPwm1Regs.ETSEL.bit.INTEN = 1;              // enable interrupt

    // Data logger initialization
    dlog.trig_value = 0;    			   // specify trigger value
    dlog.slope = Positive;                 // trigger on positive slope
    dlog.prescalar = 1;                    // store every  sample
    dlog.mode = Normal;                    // Normal trigger mode
    dlog.auto_time = 100;                  // number of calls to update function
    dlog.holdoff_time = 100;               // number of calls to update function

    //dlog.trig = &napetost;
    //dlog.iptr1 = &napetost;
    //dlog.iptr2 = &tok;


    // acknowledge interrupt
    EALLOW;
    PieVectTable.EPWM1_INT = &PER_int;
    EDIS;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    IER |= M_INT3;
    // interrupt in real time (for main loop and BACK_loop debugging)
    SetDBGIER(M_INT3);
}
