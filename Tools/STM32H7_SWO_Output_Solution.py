#!/usr/bin/env python3

"""
  # STM32H7 SWO Output Solution
    Author: Brian Khuu 2020
    Main Copy: https://gist.github.com/mofosyne/178ad947fdff0f357eb0e03a42bcef5c

    This generates codes that enables SWO output for STM32H7 and was tested on NUCLEO-H743ZI2.
    This is required because stm32h7 changed the registers that OpenOCD expects for SWO output.
    Best to use 400Mhz, tried lower... but had issue with getting stable uart output

    You have the choice of setting up SWO in firmware.
    You also have the choice of gdbinit based register code setup.

  ## Reference:

    Based on this solution (clive1): https://community.st.com/s/question/0D50X00009ce0vWSAQ/nucleoh743zi-board-and-printf-swo-not-working

    Here is some useful note about how openocd command for 'tpiu' and 'itm' works (Since last time I checked).
    Along with equivalent code as well, shown below:

    ```
    // Open OCD armv7m_trace_tpiu_config()
    //     TPIU_CSPSR = 1 << trace_config->port_size
    //     TPIU_ACPR  = prescaler - 1
    //     TPIU_SPPR  = trace_config->pin_protocol
    //     TPIU_FFCR  = ffcr
    // TPIU : Trace Port Interface Unit
    TPI->CSPSR   |= 1;                                  /* TPIU_CSPSR : Current Parallel Port Size Register  */
    TPI->ACPR    |= ((SystemCoreClock /  2000000) - 1); /* TPIU_ACPR.SWOSCALER : Set prescaler  */
    TPI->SPPR    |= 0x00000002;                         /* TPIU_SPPR.TXMODE : trace output protocol (2:  SWO NRZ, 1:  SWO Manchester encoding) */
    TPI->FFCR    &= ~TPI_FFCR_EnFCont_Msk;              /* TPIU_FFCR.EnFCont : Clear Continuous TPIU Formatting (Using ITM only) */
    //TPI->FFCR  |= TPI_FFCR_EnFCont_Msk;               /* TPIU_FFCR.EnFCont : Enable Continuous TPIU Formatting (Using ITM & ETM) */
    ```

    ```
    // Open OCD armv7m_trace_itm_config()
    //     ITM_LAR    = ITM_LAR_KEY;
    //     ITM_TCR    = (1 << 0) | (1 << 3) | ...
    //     ITM_TERx   = trace_config->itm_ter[x])
    // ITM : Instrumented Trace Macrocell
    ITM->LAR     |= 0xC5ACCE55;    /* ITM_LAR  : "Lock Access Register", C5ACCE55 enables more write access to Control Register 0xE00 : 0xFFC */
    ITM->TCR     |= ITM_TCR_ITMENA_Msk | ITM_TCR_DWTENA_Msk | ((0x7FUL & 1) << ITM_TCR_TraceBusID_Pos); /* ITM_TCR  : ITM Trace Control Register */
    ITM->TER     |= 0x1;           /* ITM_TER : "ITM Trace Enable Register" Enabled tracing on stimulus ports. One bit per stimulus port. */
    ```

  ## Viewing SWO stream on openocd

    Found this works best for me

    https://github.com/robertlong13/SWO-Parser

"""

SWO_BASE =0x5C003000
SWO_CODR      = SWO_BASE + 0x010
SWO_SPPR      = SWO_BASE + 0x0F0
SWO_FFSR      = SWO_BASE + 0x300
SWO_CLAIMSET  = SWO_BASE + 0xFA0
SWO_CLAIMCLR  = SWO_BASE + 0xFA4
SWO_LAR       = SWO_BASE + 0xFB0
SWO_LSR       = SWO_BASE + 0xFB4
SWO_AUTHSTAT  = SWO_BASE + 0xFB8
SWO_DEVID     = SWO_BASE + 0xFC8
SWO_DEVTYPE   = SWO_BASE + 0xFCC
SWO_PIDR4     = SWO_BASE + 0xFD0

SWTF_BASE=0x5C004000
SWTF_CTRL     = SWTF_BASE + 0x000
SWTF_PRIORITY = SWTF_BASE + 0x004
SWTF_CLAIMSET = SWTF_BASE + 0xFA0
SWTF_CLAIMCLR = SWTF_BASE + 0xFA4
SWTF_LAR      = SWTF_BASE + 0xFB0
SWTF_LSR      = SWTF_BASE + 0xFB4
SWTF_AUTHSTAT = SWTF_BASE + 0xFB8
SWTF_DEVID    = SWTF_BASE + 0xFC8
SWTF_DEVTYPE  = SWTF_BASE + 0xFCC
SWTF_PIDR4    = SWTF_BASE + 0xFD0
SWTF_PIDR0    = SWTF_BASE + 0xFE0
SWTF_PIDR1    = SWTF_BASE + 0xFE4
SWTF_PIDR2    = SWTF_BASE + 0xFE8
SWTF_PIDR3    = SWTF_BASE + 0xFEC
SWTF_CIDR0    = SWTF_BASE + 0xFF0
SWTF_CIDR1    = SWTF_BASE + 0xFF4
SWTF_CIDR2    = SWTF_BASE + 0xFF8
SWTF_CIDR3    = SWTF_BASE + 0xFFC

SystemCoreClock = 400000000 # Note: Urge you to use 400Mhz for stable SWO output
SWOSpeed_Hz = 2000000
SWOPrescaler = round(SystemCoreClock / SWOSpeed_Hz) - 1
print(SWOPrescaler)

c_code = f"""\
extern uint32_t SystemCoreClock;
void SWO_ITM_enable(void)
{{
  /*
    This functions recommends system speed of {SystemCoreClock}Hz and will
    use SWO clock speed of {SWOSpeed_Hz}Hz

    # GDB OpenOCD commands to connect to this:
    monitor tpiu config internal - uart off {SystemCoreClock}
    monitor itm port 0 on

    Code Gen Ref: https://gist.github.com/mofosyne/178ad947fdff0f357eb0e03a42bcef5c
  */

  /* Setup SWO and SWO funnel (Note: SWO_BASE and SWTF_BASE not defined in stm32h743xx.h) */
  // DBGMCU_CR : Enable D3DBGCKEN D1DBGCKEN TRACECLKEN Clock Domains
  DBGMCU->CR =  DBGMCU_CR_DBG_CKD3EN | DBGMCU_CR_DBG_CKD1EN | DBGMCU_CR_DBG_TRACECKEN; // DBGMCU_CR
  // SWO_LAR & SWTF_LAR : Unlock SWO and SWO Funnel
  *((uint32_t *)({SWO_LAR  :#08x})) = 0xC5ACCE55; // SWO_LAR
  *((uint32_t *)({SWTF_LAR :#08x})) = 0xC5ACCE55; // SWTF_LAR
  // SWO_CODR  : {SystemCoreClock}Hz -> {SWOSpeed_Hz}Hz
  // Note: SWOPrescaler = ((sysclock_Hz / SWOSpeed_Hz) - 1) --> {SWOPrescaler:#08x} = {SWOPrescaler} = ({SystemCoreClock} / {SWOSpeed_Hz}) - 1)
  *((uint32_t *)({SWO_CODR :#08x})) = ((SystemCoreClock /  {SWOSpeed_Hz}) - 1); // SWO_CODR
  // SWO_SPPR : (2:  SWO NRZ, 1:  SWO Manchester encoding)
  *((uint32_t *)({SWO_SPPR :#08x})) = 0x00000002; // SWO_SPPR
  // SWTF_CTRL : enable SWO
  *((uint32_t *)({SWTF_CTRL:#08x})) |= 0x1; // SWTF_CTRL

  /* SWO GPIO Pin Setup */
  //RCC_AHB4ENR enable GPIOB clock
  *(__IO uint32_t*)(0x580244E0) |= 0x00000002;
  // Configure GPIOB pin 3 as AF
  *(__IO uint32_t*)(0x58020400) = (*(__IO uint32_t*)(0x58020400) & 0xffffff3f) | 0x00000080;
  // Configure GPIOB pin 3 Speed
  *(__IO uint32_t*)(0x58020408) |= 0x00000080;
  // Force AF0 for GPIOB pin 3
  *(__IO uint32_t*)(0x58020420) &= 0xFFFF0FFF;
}}
"""

gdbinit_openocd = f"""\
#*****************************************************************************
# Enable ITM support (SWO Output)
# This is a workaround for openocd STM32H7 SWO support.
# Expects Core Clock of {SystemCoreClock}Hz for SWO speed of {SWOSpeed_Hz}Hz
# Code Gen Ref: https://gist.github.com/mofosyne/178ad947fdff0f357eb0e03a42bcef5c
#*****************************************************************************
# DBGMCU_CR : Enable D3DBGCKEN D1DBGCKEN TRACECLKEN Clock Domains
set *0x5C001004 = 0x00700000
# SWO_LAR & SWTF_LAR : Unlock SWO and SWO Funnel
set *{SWO_LAR:#08x} = 0xC5ACCE55
set *{SWTF_LAR:#08x} = 0xC5ACCE55
# SWO_CODR  : systemCoreClock -> SWO_Hz == {SystemCoreClock}Hz -> {SWOSpeed_Hz}Hz
# SWO_CODR  = {SWOPrescaler:#08x} = {SWOPrescaler} = ({SystemCoreClock} / {SWOSpeed_Hz}) - 1)
set *{SWO_CODR:#08x} = {SWOPrescaler:#08x}
# SWO_SPPR  : (2:  SWO NRZ, 1:  SWO Manchester encoding)
set *{SWO_SPPR:#08x} = 0x00000002
# SWTF_CTRL : enable SWO
set *{SWTF_CTRL:#08x} = (*{SWTF_CTRL:#08x}) | 0x1

monitor tpiu config internal - uart off {SystemCoreClock}
monitor itm port 0 on
"""


print_solution = f"""
# STM32H7 SWO Output Solution

## C

```.c
{c_code}\
```

## GDBINIT

```.gdbinit
{gdbinit_openocd}\
```

"""

print(print_solution)