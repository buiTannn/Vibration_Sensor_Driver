
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/gpio.h>     //GPIO
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/workqueue.h>

#define EN_DEBOUNCE

#ifdef EN_DEBOUNCE
#include <linux/jiffies.h>

extern unsigned long volatile jiffies;
unsigned long old_jiffie = 0;
#endif
int flag = 0;



#define GPIO_22_OUT 534

#define GPIO_27_IN  539

unsigned int GPIO_irqNumber;
static struct delayed_work my_work; 

void gpio_work_handler(struct work_struct *work)
{
    gpio_set_value(GPIO_22_OUT, 0); 
    pr_info("Loa turned off after 5 seconds\n");
}

irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
#ifdef EN_DEBOUNCE
    unsigned long diff = jiffies - old_jiffie;
    if (diff < 100) {
        return IRQ_HANDLED;
    }
    old_jiffie = jiffies;
#endif  

    if (flag == 0) {
        gpio_set_value(GPIO_22_OUT, 1);  
        schedule_delayed_work(&my_work, msecs_to_jiffies(5000)); 
    } else {
        gpio_set_value(GPIO_22_OUT, 0);  
    }
	pr_info("da xu li xong ngat");


    return IRQ_HANDLED;
}



dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
 
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
 
/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
/******************************************************/

//File operation structure 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};

/*
** This function will be called when we open the Device file
*/ 
static int etx_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

/*
** This function will be called when we close the Device file
*/ 
static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

/*
** This function will be called when we read the Device file
*/ 
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  
  //reading GPIO value
  gpio_state = gpio_get_value(GPIO_22_OUT);
  
  //write to user
  len = 1;
  if( copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }
  
  pr_info("Read function : GPIO_22 = %d \n", gpio_state);
  
  return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_22 Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0] == '1') {
    gpio_set_value(GPIO_22_OUT, 1);
  } else if (rec_buf[0] == '0') {
    gpio_set_value(GPIO_22_OUT, 0);
  } else if (rec_buf[0] == '2') {
    uint8_t gpio_state = gpio_get_value(GPIO_22_OUT);
    pr_info("Loa Current State: %d\n", gpio_state);
    
    // Gửi lại trạng thái LOA cho người dùng
    char *user_buf = (char *)buf;
    if (copy_to_user(user_buf, &gpio_state, 1) > 0) {
      pr_err("ERROR: Not all the bytes have been copied to user\n");
    }
    return 1; 
  } else if (rec_buf[0] == '3') {
    pr_info("Sensor Activated: Loa will work based on sensor input.\n");
    flag = 0;
  } else if (rec_buf[0] == '4') {
    pr_info("Sensor Deactivated: Loa will remain OFF.\n");
    flag = 1;
  } else {
    pr_err("Unknown command : Please provide either 1, 0, 2, 3 or 4 \n");
  }
  
  return len;
}



/*
** Module Init function
*/ 
static int __init etx_driver_init(void)
{
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

  /*Creating cdev structure*/
  cdev_init(&etx_cdev,&fops);

  /*Adding character device to the system*/
  if((cdev_add(&etx_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }

  /*Creating struct class*/
  if(IS_ERR(dev_class = class_create("etx_class"))){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }

  /*Creating device*/
  if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  //Output GPIO configuration
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_22_OUT) == false){
    pr_err("GPIO %d is not valid\n", GPIO_22_OUT);
    goto r_device;
  }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_22_OUT,"GPIO_22_OUT") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_22_OUT);
    goto r_gpio_out;
  }
  
  //configure the GPIO as output
  gpio_direction_output(GPIO_22_OUT, 0);
INIT_DELAYED_WORK(&my_work, gpio_work_handler);  // Khởi tạo delayed_work và chỉ định handler


  //Input GPIO configuratioin
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_27_IN) == false){
    pr_err("GPIO %d is not valid\n", GPIO_27_IN);
    goto r_gpio_in;
  }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_27_IN,"GPIO_27_IN") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_27_IN);
    goto r_gpio_in;
  }
  
  //configure the GPIO as input
  gpio_direction_input(GPIO_27_IN);


  
  //Get the IRQ number for our GPIO
  GPIO_irqNumber = gpio_to_irq(GPIO_27_IN);
  pr_info("GPIO_irqNumber = %d\n", GPIO_irqNumber);
  
  if (request_irq(GPIO_irqNumber,             //IRQ number
                  (void *)gpio_irq_handler,   //IRQ handler
                  IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                  "etx_device",               //used to identify the device name using this IRQ
                  NULL)) {                    //device id for shared IRQ
    pr_err("my_device: cannot register IRQ ");
    goto r_gpio_in;
  }
  
  
 
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;

r_gpio_in:
  gpio_free(GPIO_27_IN);
r_gpio_out:
  gpio_free(GPIO_22_OUT);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&etx_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{

  free_irq(GPIO_irqNumber,NULL);
  gpio_free(GPIO_27_IN);
  gpio_free(GPIO_22_OUT);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);

  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NHOM DOM DOM");
MODULE_DESCRIPTION(" INTERRUPT ");
MODULE_VERSION("1.33");

