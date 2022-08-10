#ifndef LIBCHARGER_H
#define LIBCHARGER_H

#include <stdint.h>

typedef enum
{
   CHG_STATE_CHARGING  = 0x0,
   CHG_STATE_CHARGED   = 0x1,
   CHG_STATE_DISCHARGE = 0x2,
   CHG_STATE_ERROR     = 0x3,
   CHG_STATE_UNKNOWN   = 0x4,

   CHG_STATE_COUNT
} charger_state_t;

typedef enum {
	BAT_CHRG_LOW_CURRENT = 0,	// HW  dependent DONT EDIT; nearly 0.7A //
	BAT_CHRG_HIGH_CURRENT = 1,	// HW  dependent DONT EDIT; nearly  3.6A //

} clibc2CHG_Charge_Current_t;

extern int8_t libc2charger_get_charger_status(void);
extern int8_t libc2charger_set_chrg_current(clibc2CHG_Charge_Current_t new_current);

/*return value is in 10mv.
Example: 14.13V is measured return value is 1413*/
//extern int16_t get_charger_ADC_value(void);
extern int32_t libc2charger_get_battery_pack_voltage(void);
extern int8_t libc2charger_get_battery_percentage(void);
extern int8_t libc2charger_init(const int libc2charger_log_level, uint8_t dev_type);
extern void libc2charger_deinit(void);

#endif	/* End of LIBCHARGER_H */

