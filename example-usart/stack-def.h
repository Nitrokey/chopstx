#define MAIN_SIZE 0x0080 /* Idle+Exception handlers  */
#define SIZE_0 0x0200 /* Main program             */
#define SIZE_1 0x0100 /* first thread program     */
#define SIZE_2 0x0100 /* second thread program    */ 
#define SIZE_3 0x0200 /* third thread program    */ 

#if defined(STACK_MAIN)
/*
 * The terminology of "main" is confusing in ARM architecture.
 * Here, "main_base" is for exception handlers.
 */
/* Idle+Exception handlers */
char __main_stack_end__[0] __attribute__ ((section(".main_stack")));
char main_base[MAIN_SIZE] __attribute__ ((section(".main_stack")));

/* Main program            */
char __process0_stack_end__[0] __attribute__ ((section(".process_stack.0")));
char process0_base[SIZE_0] __attribute__ ((section(".process_stack.0")));
#endif

/* First thread program    */
#if defined(STACK_PROCESS_1)
char process1_base[SIZE_1] __attribute__ ((section(".process_stack.1"))); 
#endif

/* Second thread program   */
#if defined(STACK_PROCESS_2)
char process2_base[SIZE_2] __attribute__ ((section(".process_stack.2")));
#endif

/* Third thread program    */
#if defined(STACK_PROCESS_3)
char process3_base[SIZE_3] __attribute__ ((section(".process_stack.3")));
#endif

/* Fourth thread program    */
#if defined(STACK_PROCESS_4)
char process4_base[SIZE_4] __attribute__ ((section(".process_stack.4")));
#endif

/* Fifth thread program    */
#if defined(STACK_PROCESS_5)
char process5_base[SIZE_5] __attribute__ ((section(".process_stack.5")));
#endif

/* Sixth thread program    */
#if defined(STACK_PROCESS_6)
char process6_base[SIZE_6] __attribute__ ((section(".process_stack.6")));
#endif

/* Seventh thread program    */
#if defined(STACK_PROCESS_7)
char process7_base[SIZE_7] __attribute__ ((section(".process_stack.7")));
#endif
