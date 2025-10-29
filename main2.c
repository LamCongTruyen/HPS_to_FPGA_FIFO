///////////////////////////////////////
/// 640x480 version!
/// test VGA with hardware video input copy to VGA
// compile with
// gcc fp_test_1.c -o fp1 -lm
///////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <sys/time.h> 
#include <math.h> 
#include "hps_0.h"

#define FIFO_0_IN_BASE 0x0
#define FIFO_0_IN_SPAN 4
#define FIFO_0_IN_END 0x3
#define FIFO_0_IN_CSR_BASE 0x0
#define FIFO_0_IN_CSR_SPAN 32
#define FIFO_0_IN_CSR_END 0x1f
#define ONCHIP_MEMORY2_0_BASE 0x8000000
#define ONCHIP_MEMORY2_0_SPAN 1024
#define ONCHIP_MEMORY2_0_END 0x80003ff
#define H2F_AXI_BASE          0xC0000000  // Base for h2f_axi_master

// Lightweight bridge base (for CSR/status)
#define H2F_LW_AXI_BASE       0xFF200000  // Base for h2f_lw_axi_master

// main bus; scratch RAM
#define FPGA_ONCHIP_BASE      (H2F_AXI_BASE + ONCHIP_MEMORY2_0_BASE)
#define FPGA_ONCHIP_SPAN      ONCHIP_MEMORY2_0_SPAN

// main bus; FIFO write address
#define FIFO_BASE            (H2F_AXI_BASE + FIFO_0_IN_BASE)
#define FIFO_SPAN            FIFO_0_IN_SPAN

/// lw_bus; FIFO status address
#define HW_REGS_BASE          (H2F_LW_AXI_BASE + FIFO_0_IN_CSR_BASE)
#define HW_REGS_SPAN          FIFO_0_IN_CSR_SPAN 
#define FIFO_CSR_BASE         (H2F_LW_AXI_BASE + FIFO_0_IN_CSR_BASE)
#define FIFO_CSR_SPAN         FIFO_0_IN_CSR_SPAN
// the light weight buss base
void *h2p_lw_virtual_base;
// HPS_to_FPGA FIFO status address = 0
volatile unsigned int * FIFO_status_ptr = NULL ;

// RAM FPGA command buffer
// main bus addess 0x0800_0000
volatile unsigned int * sram_ptr = NULL ;
void *sram_virtual_base;

// HPS_to_FPGA FIFO write address
// main bus addess 0x0000_0000
void *h2p_virtual_base;
volatile unsigned int * FIFO_write_ptr = NULL ;

// /dev/mem file id
int fd;	
	
int main(void)
{

	// Declare volatile pointers to I/O registers (volatile 	// means that IO load and store instructions will be used 	// to access these pointer locations, 
	// instead of regular memory loads and stores) 
  	
	// === need to mmap: =======================
	// FPGA_CHAR_BASE
	// FPGA_ONCHIP_BASE      
	// HW_REGS_BASE        
  
	// === get FPGA addresses ==================
    // Open /dev/mem
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) 	{
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
    
	//============================================
    // get virtual addr that maps to physical
	// for light weight bus
	// FIFO status registers
	h2p_lw_virtual_base = mmap( NULL, FIFO_CSR_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FIFO_CSR_BASE );	
	if( h2p_lw_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap1() failed...\n" );
		close( fd );
		return(1);
	}
	FIFO_status_ptr = (unsigned int *)(h2p_lw_virtual_base);
	
	//============================================
	
	// scratch RAM FPGA parameter addr 
	sram_virtual_base = mmap( NULL, FPGA_ONCHIP_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_ONCHIP_BASE); 	
	
	if( sram_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap3() failed...\n" );
		close( fd );
		return(1);
	}
    // Get the address that maps to the RAM buffer
	sram_ptr =(unsigned int *)(sram_virtual_base);
	
	// ===========================================

	// FIFO write addr 
	h2p_virtual_base = mmap( NULL, FIFO_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FIFO_BASE); 	
	
	if( h2p_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap3() failed...\n" );
		close( fd );
		return(1);
	}
    // Get the address that maps to the FIFO
	FIFO_write_ptr =(unsigned int *)(h2p_virtual_base);
	
	//============================================
	
	
	while(1) 
	{
		int num;
		 
		// input a number
		scanf("%d", &num);
		
		// send to FIFO
		*(FIFO_write_ptr) = num ;
		
		// wait for SRAM flag 
		// set to 1 when FPGA state machine writes SRAM scratch
		while (*(sram_ptr)==0) ;
		// clear flag
		*(sram_ptr) = 0 ;
		
		// receive back and print
		printf("return=%d\n\r", *(sram_ptr+1)) ;
		
	} // end while(1)
} // end main

/// /// ///////////////////////////////////// 
/// end /////////////////////////////////////