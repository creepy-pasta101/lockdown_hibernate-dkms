#include <linux/module.h>	/* Needed by all modules */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kprobes.h>
#include <linux/security.h>
#include <linux/ptrace.h>
#include <linux/moduleparam.h>  /* Needed for module parameters */

/* Module parameter definition */
static bool enable = false;
module_param(enable, bool, 0444); /* 0444 makes it read-only in sysfs at runtime */
MODULE_PARM_DESC(enable, "Enable hibernation when kernel is in lockdown (default: false)");

/* Data passed from the entry handler to the return handler */
struct kretprobe_data {
  bool is_hibernate;
};


int ret;

/* * This executes RIGHT BEFORE security_locked_down() runs.
 * We inspect the arguments being passed to it.
 */
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  struct kretprobe_data *data = (struct kretprobe_data *)ri->data;

  /* Fetch the 1st argument passed to the function (enum lockdown_reason what) */
  int reason = regs_get_kernel_argument(regs, 0);

  /* Check if the kernel is asking about hibernation */
  if (reason == LOCKDOWN_HIBERNATION) {
    data->is_hibernate = true;
  } else {
    data->is_hibernate = false;
  }
  return 0;
}

/* * This executes RIGHT AFTER security_locked_down() finishes,
 * but before it hands the result back to the kernel.
 */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  struct kretprobe_data *data = (struct kretprobe_data *)ri->data;

  /* If it was a hibernation check, override the return value to 0 (Allowed) */
  if (data->is_hibernate) {
    #ifdef CONFIG_X86_64
    regs->ax = 0;
    #elif defined(CONFIG_ARM64)
    regs->regs[0] = 0;
    #else
    #error "Architecture not supported for return value manipulation"
    #endif
  }

  /* Otherwise, let the original return value pass through untouched */
  return 0;
}

/* Define the kretprobe */
static struct kretprobe my_kretprobe = {
  .handler = ret_handler,
  .entry_handler = entry_handler,
  .data_size = sizeof(struct kretprobe_data),
  .maxactive = 20, /* Handle up to 20 concurrent calls */
  .kp = {
    .symbol_name = "security_locked_down",
  },
};

static int __init lockdown_hibernate_start(void)
{

  /* Check if the user passed the kernel boot parameter */
  if (!enable) {
    pr_info("lockdown_hibernate: Module loaded but INACTIVE. Boot with 'lockdown_hibernate.enable=1' to activate bypass.\n");
    return 0; /* Return 0 so systemd-modules-load doesn't throw an error, but we do nothing */
  }

  ret = register_kretprobe(&my_kretprobe);
  if (ret != 0) {
    pr_err("lockdown_hibernate: register_kretprobe failed, returned %d\n", ret);
    return -1;
  }
  pr_info("lockdown_hibernate: kretprobe successfully planted on security_locked_down\n");
  return 0;
}

static void __exit lockdown_hibernate_end(void)
{
  /* Only attempt to unregister if we successfully registered it in the first place */
  if (enable && ret == 0) {
    unregister_kretprobe(&my_kretprobe);
    pr_info("lockdown_hibernate: kretprobe unregistered\n");
  }
  pr_info("lockdown_hibernate: goodbye\n\n");
}

module_init(lockdown_hibernate_start);
module_exit(lockdown_hibernate_end);

#define VERSION "0.1.0"

MODULE_AUTHOR("creepy-pasta101 <> / Modified by Gemini");
MODULE_DESCRIPTION("lockdown_hibernate (Perform Hibernation during Kernel Lockdown) ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
