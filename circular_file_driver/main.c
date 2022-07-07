#include "circular_file.h"
#include <linux/init.h>       /* module_init, module_exit */
#include <linux/module.h> /* version info, MODULE_LICENSE, MODULE_AUTHOR, printk() */

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Modeo Team, C2TECH");


/*===============================================================================================*/
static int simple_driver_init(void)
{
	int result = 0;
    printk( KERN_NOTICE "%s: Initialization started\n", CIRCULAR_FILE_DRIVER_NAME);

	result = register_device();
    return result;
}
/*-----------------------------------------------------------------------------------------------*/
static void simple_driver_exit(void)
{
	printk( KERN_NOTICE "%s: Exiting\n", CIRCULAR_FILE_DRIVER_NAME);
	unregister_device();
}
/*===============================================================================================*/

module_init(simple_driver_init);
module_exit(simple_driver_exit);
