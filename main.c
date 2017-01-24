#include <CppUTest/CommandLineTestRunner.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define IRQ 15

#define readl(a) (*(volatile unsigned int *)(a))
#define writel(a,v) (*(volatile unsigned int *)(a)) = (v)

//GICD Register
#define GICD_MEMORY_SIZE 227

unsigned int *GICD = NULL;
unsigned int *GICD_CTLR = NULL;			// 1
unsigned int *GICD_PMR = NULL;			// 1
unsigned int *GICD_ISENABLER = NULL; 	// 20
unsigned int *GICD_ICENABLER = NULL;	// 5
unsigned int *GICD_ITARGETSR = NULL; 	// 100
unsigned int *GICD_IPRIORITYR = NULL; 	// 100

#define GICD_ISENABLER(n) (GICD_ISENABLER + n)
#define GICD_ICENABLER(n) (GICD_ICENABLER + n)
#define GICD_ITARGETSR(n) (GICD_ITARGETSR + n)
#define GICD_IPRIORITYR(n) (GICD_IPRIORITYR + n)


//GICC Register
unsigned int *GICC = NULL;
unsigned int *GICC_CTLR = NULL;
unsigned int *GICC_PMR = NULL;	


//GTimer Co-processor Memory;
unsigned int *GTIMER_CTL = NULL;
unsigned int *GTIMER_DEADLINE = NULL;
//Assembly macro
#define stringify(s)	#s

TEST_GROUP(FirstTestGroup)
{
	void setup()
	{
		//executed before Testing
	}

	void teardown()
	{
		//executed after Testing
	}
};


///////////////////////////////
/////   GTimer Assembly   /////
///////////////////////////////
void set_CNTV_CVAL(unsigned long val)
{
	/*
	asm volatile("mcrr p15, " stringify(3)
			", %0, %1, c" stringify(14)
			"\n" : : "r" ((val) & 0xFFFFFFFF), "r" ((val) >> 32));
			*/

	//Temporary Memory
	writel(GTIMER_DEADLINE, val);
}

unsigned long get_CNTVCT()
{
	unsigned int val1, val2;
	/*
	asm volatile("mrrc p15, " stringify(1)
			", %0, %1, c" stringify(14) "\n"
			: "=r" (val1), "=r" (val2) : :);
	return ((unsigned long) val1 + ((unsigned long) val2 << 32));
	*/

	//Temporary Memory
	return readl(GTIMER_DEADLINE);
}

void set_CNTV_CTL(unsigned int val)
{
	/*
	asm volatile("mcr p15, " stringify(0)
			", %0, c" stringify(14)
			", c" stringify(3)
			", " stringify(1) "\n" : "r" (val));
	*/

	//Temporary Memory
	writel(GTIMER_CTL,val);
}

unsigned int get_CNTV_CTL()
{
	unsigned int val;
	/*
	asm volatile("mrc p15, " stringify(0)
			", %0, c" stringify(14)
			", c" stringify(3)
			", " stringify(1) "\n" : "=r" (val));
	return val;
	*/

	//Temporary Memory
	return readl(GTIMER_CTL);
}

unsigned long gt_get_virtual_count()
{
	return get_CNTVCT();
}

void gt_set_virtual_timer_deadline(unsigned long deadline)
{
	unsigned long current_virtual_time = gt_get_virtual_count();
	set_CNTV_CVAL(current_virtual_time+deadline);
}

void gt_unmask_virtual_timer()
{
	unsigned int val = get_CNTV_CTL();
	val &= ~(1 << 1);
	set_CNTV_CTL(val);
}

void gt_enable_virtual_timer()
{
	unsigned int val = get_CNTV_CTL();
	val |= (1 << 0);
	set_CNTV_CTL(val);
}


///////////////////////////////
/////    GIC FUNCTION     /////
///////////////////////////////
void gic_enable_interrupts()
{
	writel(GICD_CTLR, 0x00000001);
	writel(GICC_CTLR, 0x00000001);
}

void gic_disable_interrupts()
{
	writel(GICC_CTLR, 0x00000000);
	writel(GICD_CTLR, 0x00000000);
}

void gicd_set_enable_irq(int irq)
{
	int idx = irq >> 5;
	int off = irq & 31;
	unsigned int val;
	val = (1 << off);
	writel(GICD_ISENABLER(idx), val);
}

void gicd_set_target_irq(int irq, int cpu_id)
{
	int idx = irq >> 2;
	int off = (irq & 3) << 3;
	unsigned val = readl(GICD_ITARGETSR(idx));

	val |= ((1 << cpu_id) << off);
	writel(GICD_ITARGETSR(idx), val);
}

void gicd_set_priority_irq(int irq, int pri)
{
	int idx = irq >> 2;
	int off = (irq & 3) << 3;
	unsigned int val = readl(GICD_IPRIORITYR(idx));
	unsigned int mask = 0xFF << off;

	val &= ~mask;

	val |= (pri & 0xFF) << off;
	writel(GICD_IPRIORITYR(idx), val);
}


//////////////////////////////////////
/////    INITIALIZE FUNCTION     /////
//////////////////////////////////////
void init_gicd(void)
{
	unsigned int result;
	unsigned int cpu_id = 0;

	int i = 0;

	gic_disable_interrupts();
	CHECK_EQUAL(0x00000000, readl(GICC_CTLR));
	CHECK_EQUAL(0x00000000, readl(GICD_CTLR));

	for(i = 0 ; i < 5 ; i++)
	{
		writel(GICD_ICENABLER(i),0xFFFFFFFF);
	}

	gicd_set_enable_irq(IRQ);
	gicd_set_target_irq(IRQ, cpu_id);
	gicd_set_priority_irq(IRQ, 0xA0);

	gic_enable_interrupts();
	CHECK_EQUAL(0x00000001, readl(GICC_CTLR));
	CHECK_EQUAL(0x00000001, readl(GICD_CTLR));
}

void init_gicc(void)
{
	gic_disable_interrupts();
	CHECK_EQUAL(0x00000000, readl(GICC_CTLR));
	CHECK_EQUAL(0x00000000, readl(GICD_CTLR));
	writel(GICC_PMR, 0xFF);
	gic_enable_interrupts();
	CHECK_EQUAL(0x00000001, readl(GICC_CTLR));
	CHECK_EQUAL(0x00000001, readl(GICD_CTLR));
}

void init_gtimer()
{
	gt_set_virtual_timer_deadline(300000);
	gt_unmask_virtual_timer();
	gt_enable_virtual_timer();
}

TEST(FirstTestGroup, GIC)
{
	unsigned int *GICD_RESULT = NULL;
	int i = 0;

	///////////////////////////
	//   GICC Initailizing   //
	///////////////////////////
	init_gicc();
	CHECK_EQUAL(0xFF, readl(GICC_PMR));


	///////////////////////////
	//   GICD Initailizing   //
	///////////////////////////
	init_gicd();

	//GICD ISENABLER TEST
	CHECK_EQUAL(
			0x08000000,
			readl(GICD_ISENABLER((unsigned int)(IRQ >> 5)))
			);

	//GICD ICENABLER TEST
	for(i = 0; i < 5; i++)
	CHECK_EQUAL(0xFFFFFFFF, readl(GICD_ICENABLER(i)));

	//GICD ITARGETSR TEST
	CHECK_EQUAL(
			0x01000000,
			readl(GICD_ITARGETSR((unsigned int)(IRQ >> 2)))
			);

	//GICD IPRIORITYR TEST
	CHECK_EQUAL(
			0xA0000000,
			readl(GICD_IPRIORITYR((unsigned int)(IRQ >> 2)))
			);


	///////////////////////////
	//  GTimer Initailizing  //
	///////////////////////////
	init_gtimer();
	CHECK_EQUAL(0x1, readl(GTIMER_CTL));
	CHECK_EQUAL(300000, readl(GTIMER_DEADLINE));
}

TEST(FirstTestGroup, GTimer)
{
}

void memory_alloc()
{
	//Register Memory Allocation

	//GICD Register Memory Setting
	GICD = (unsigned int *)malloc(sizeof(unsigned int) * 227);
	GICD_CTLR = GICD;			// 0
	GICD_PMR = GICD+1;			// 1
	GICD_ISENABLER = GICD+2; 	// 2~21
	GICD_ICENABLER = GICD+22;	// 22~26
	GICD_ITARGETSR = GICD+27; 	// 27~126
	GICD_IPRIORITYR = GICD+127; // 127~226

	//GICC Register Memory Setting
	GICC = (unsigned int *)malloc(sizeof(unsigned int) * 2);
	GICC_CTLR = GICC;
	GICC_PMR = GICC+1;

	//GTIMER Register Memory Setting
	GTIMER_CTL = (unsigned int *)malloc(sizeof(unsigned int) * 2);
	GTIMER_DEADLINE = GTIMER_CTL+1;
}

void memory_free()
{
	//Register Memory Clear
	free(GICD);
	free(GICC);
	free(GTIMER_CTL);
}

int main(int ac, char** av){
	memory_alloc();
	int result = CommandLineTestRunner::RunAllTests(ac, av);
	memory_free();
	return result;
}
