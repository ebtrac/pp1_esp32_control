// contains pinout info for the ESP32 for this specific project
#define DAT0	35 //input only
#define DAT1	33
#define DAT2	25
#define DAT3	26
#define DAT4	27
#define DAT5	14 //NORMALLY HIGH during boot
#define DAT6	12
#define DAT7	13
#define DAT8	23 
#define DAT9	16
#define DAT10	4
#define DAT11	19
#define DAT12	18
#define DAT13	5 //NORMALLY HIGH during boot //appears to be a strapping pin used for boot configurations
#define DAT14	17

#define NOT_BUF_OE    21
#define DAT_BUS_LEVEL_SHIFTER_OE 2 //has a pulldown resistor during boot
#define BUF_CTL_LEVEL_SHIFTER_OE 22