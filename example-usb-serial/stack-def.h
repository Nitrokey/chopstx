#if defined(STACK_MAIN)
/* Idle+Exception handlers */
char __main_stack_end__[0] __attribute__ ((section(".main_stack")));
char main_base[0x0080] __attribute__ ((section(".main_stack")));

/* Main program            */
char __process0_stack_end__[0] __attribute__ ((section(".process_stack.0")));
char process0_base[0x0400] __attribute__ ((section(".process_stack.0")));
#endif

/* First thread program    */
#if defined(STACK_PROCESS_1)
char process1_base[0x0200] __attribute__ ((section(".process_stack.1"))); 
#endif

/* Second thread program   */
#if defined(STACK_PROCESS_2)
char process2_base[0x0200] __attribute__ ((section(".process_stack.2")));
#endif

/* Third thread program    */
#if defined(STACK_PROCESS_3)
char process3_base[0x0200] __attribute__ ((section(".process_stack.3")));
#endif

/* Fourth thread program    */
#if defined(STACK_PROCESS_4)
char process4_base[0x0200] __attribute__ ((section(".process_stack.4")));
#endif

/* Fifth thread program    */
#if defined(STACK_PROCESS_5)
char process5_base[0x0200] __attribute__ ((section(".process_stack.5")));
#endif

/* Sixth thread program    */
#if defined(STACK_PROCESS_6)
char process6_base[0x0200] __attribute__ ((section(".process_stack.6")));
#endif
