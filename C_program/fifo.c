
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
#include <stdint.h>
#include <arpa/inet.h>
// main bus; scratch RAM
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_ONCHIP_SPAN      0x00001000

// main bus; FIFO write address
#define FIFO_BASE            0xC0000000
#define FIFO_SPAN            0x00001000

/// lw_bus; FIFO status address
#define HW_REGS_BASE          0xff200000
#define HW_REGS_SPAN          0x00005000 

#define PORT 8080
#define BUFFER_SIZE 65536

#define WAIT {}
#define FIFO_WRITE		     (*(FIFO_write_ptr))

#define WRITE_FIFO_FILL_LEVEL (*FIFO_status_ptr)
#define WRITE_FIFO_FULL		  ((*(FIFO_status_ptr+1))& 1 ) 

#define FIFO_WRITE_BLOCK(a)	  {while (WRITE_FIFO_FULL){WAIT};FIFO_WRITE=a;}

volatile unsigned int * FIFO_write_status_ptr = NULL ;

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
int N;
long total_bytes = 0;
struct timeval t1, t2;
double elapsedTime;

//============================================

void decrypt_from_bin(){
	
	FILE *fbin = fopen("output_from_fpga.bin", "rb");
	if (!fbin) {
		perror("Không mở được file output_from_fpga.bin");
		return;
	}
	// fseek(fbin, 0, SEEK_END);
	// long total_bytes_long = ftell(fbin);
	// fseek(fbin, 0, SEEK_SET);
	
	long total_bytes_long = 6432;

	if (total_bytes_long <= 0) {
        printf("File rỗng hoặc lỗi đọc kích thước\n");
        fclose(fbin);
        return;
    }
	size_t total_bytes = (size_t) total_bytes_long;

	unsigned char *buffer_data_bin = malloc(total_bytes);

	size_t read_bytes = fread(buffer_data_bin, 1, total_bytes, fbin);

	printf("Đã đọc %zu bytes từ output_from_fpga.bin\n", read_bytes);
	fclose(fbin);


	// long total_blocks = (total_bytes + 15) / 16; 
	long total_blocks = total_bytes / 16;

    printf("Tổng số block cần gửi: %ld (16 bytes each)\n", total_blocks);
	// for (int i = 0; i < read_bytes; i++) {
	// 	printf("%02X ", buffer_data_bin[i]);
	// 	if ((i + 1) % 16 == 0) printf("\n");
	// }
	
	printf("\n");

	printf("Bắt đầu gửi dữ liệu giải mã lại từ FPGA...\n");
	gettimeofday(&t1, NULL);
	int count = 0;
	
	uint8_t *plaintext_back = malloc(total_bytes); //calloc(1, total_bytes); //malloc(total_bytes); 
	uint32_t* data_from_bin = malloc(4 * sizeof(uint32_t));
	
	for (int block = 0; block < total_blocks; block++) { 
		// printf("\n=== Gửi block %d ===\n", block);

		for (int i = 0; i <4; i++) {
			int idx = block * 16 + i * 4;
			data_from_bin[i] =  (buffer_data_bin[idx] << 24)   |
								(buffer_data_bin[idx+1] << 16) |
								(buffer_data_bin[idx+2] << 8)  |
								buffer_data_bin[idx+3];

			// *(FIFO_write_ptr) = data_from_bin[i];
			FIFO_WRITE_BLOCK( data_from_bin[i] );
		}

		// for (int i = 1; i < 5; i++) {
		// 	printf("return word[%d] = 0x%08X\n", i, *(sram_ptr + i));
		// }
		count += 16;
		while (*(sram_ptr) == 0);
		*(sram_ptr) = 0;

		for (int i = 0; i < 4; i++) {
			uint32_t word_data_bytes2 = *(sram_ptr + i + 1);
			int idx = block * 16 + i * 4;
			plaintext_back[idx + 0] = (word_data_bytes2>> 24)  & 0xFF; 
			plaintext_back[idx + 1] = (word_data_bytes2 >> 16) & 0xFF;
			plaintext_back[idx + 2] = (word_data_bytes2 >> 8)  & 0xFF;
			plaintext_back[idx + 3] =  word_data_bytes2 & 0xFF;        
		}
	}
	
	// printf("\n Plaintext decrypt from FPGA = \n");
	// for (int i = 0; i < total_bytes; i++) {
	// 	printf("%02X ", plaintext_back[i]);
	// 	if ((i + 1) % 16 == 0) printf("\n");
	// }	

	//============================================

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec)  * 1000000.0;     // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) ;   // us to 
	// printf("T=%8.0f uSec  \n\r", elapsedTime);
	printf("Decrypt ciphertext time = %.2f us for %d words\n", elapsedTime, count);
	double MBps = (count * 4.0) / elapsedTime;
	printf("Decrypt ciphertext speed = %.2f MB/s\n", MBps);

	//============================================

	FILE *fout = fopen("plaintext_from_fpga.bin", "wb");
	if (fout) {
		fwrite(plaintext_back, 1, total_bytes, fout);
		fclose(fout);
		printf("\n Đã lưu dữ liệu giải mã vào file plaintext_from_fpga.bin\n");
	}
	free(plaintext_back);
	free(data_from_bin);
	free(buffer_data_bin);
	
}

void encrypt_from_bytepicture(){
	
	FILE *img = fopen("received_image.jpg", "rb");
	if (!img) {
		perror("File received_image.jpg open failed");
		return;
	}
	
	fseek(img, 0, SEEK_END);
	long total_bytes = ftell(img);
	fseek(img, 0, SEEK_SET);
	
	long total_blocks = (total_bytes + 15) / 16;
    printf("Total %ld blocks (16 bytes each)\n", total_blocks);
	
	uint8_t *data_byte = malloc(total_bytes);
	fread(data_byte, 1, total_bytes, img);
	fclose(img);
	
	printf("Đã đọc %ld bytes từ received_image.jpg\n", total_bytes);
	
	uint8_t *cipher_back = malloc(total_bytes);
	uint32_t *words_send = malloc(4 * sizeof(uint32_t));

	// printf("First 64 bytes of image:\n");
	// int bytes_to_print = (total_bytes < 64) ? total_bytes : 64;
	// printf("Data_bytes picture: \n");
	// for (int i = 0; i < bytes_to_print; i++) {
	// 	printf("%02X ", data_byte[i]);
	// 	if ((i + 1) % 16 == 0) printf("\n");
	// }

	for (int block = 0; block < total_blocks; block++) { 

		// printf("\n=== Sending block %d ===\n", block);

		for (int i = 0; i <4; i++) {
			int idx = block * 16 + i * 4;
			words_send[i] =  (data_byte[idx] << 24) 	 |
						(data_byte[idx+1] << 16) |
						(data_byte[idx+2] << 8)  |
						 data_byte[idx+3];

			*(FIFO_write_ptr) = words_send[i];
		}

		// for (int i = 1; i < 5; i++) {
		// 	printf("return word[%d] = 0x%08X\n", i, *(sram_ptr + i));
		// }

		while (*(sram_ptr) == 0);
		*(sram_ptr) = 0;  
		       
		for (int i = 0; i <4; i++) {
			uint32_t word_data_bytes2 = *(sram_ptr + i + 1);
			int idx = block * 16 + i * 4;
			cipher_back[idx + 0] = (word_data_bytes2>> 24) & 0xFF; 
			cipher_back[idx + 1] = (word_data_bytes2 >> 16) & 0xFF;
			cipher_back[idx + 2] = (word_data_bytes2 >> 8)  & 0xFF;
			cipher_back[idx + 3] = word_data_bytes2 & 0xFF;        
		}
	}

	printf("\n");
	// printf("data_bytes resend form FPGA = \n");
	// for (int i = 0; i < total_bytes; i++) {
	// 	printf("%02X ", cipher_back[i]);
	// 	if ((i + 1) % 16 == 0) printf("\n");
	// }
	// printf("\n");
	
	FILE *fout = fopen("output_from_fpga.bin", "wb");
	if (!fout) {
		perror("Cannot open output_from_fpga.bin");
		return;
	}
	fwrite(cipher_back, 1, total_bytes, fout);
	fclose(fout);
	printf("Saved %ld bytes to file name output_from_fpga.bin successfully!\n", total_bytes);

	free(data_byte);
	free(words_send);
	free(cipher_back);	
	printf("\n");
}


//============================================


//============================================

int main(void)
{
	int server_fd, new_socket;
	struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
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
	h2p_lw_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
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
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return -1;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }
    listen(server_fd, 3);
    printf("✅ Server đang chạy trên port %d...\n", PORT);

	//============================================
	// uint8_t data_bytes2[128];
	// int temp1, temp2;
	//============================================

	while(1) 
	{
		new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }
        FILE *f = fopen("received_image.jpg", "wb");
        if (!f) {
            perror("File open failed");
            close(new_socket);
            continue;
        }
		long total_bytes2 = 0;
        long bytes_received;
        while ((bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0){
            fwrite(buffer, 1, bytes_received, f);
            total_bytes2 += bytes_received;
        }
		fclose(f);
        printf("📸 Ảnh đã được lưu: received_image.jpg (%ld byte)\n", total_bytes2);
		close(new_socket);

		//============================================

		FILE *img = fopen("received_image.jpg", "rb");
		if (!img) {
			perror("File received_image.jpg open failed");
			return 1;
		}

		// fseek(img, 0, SEEK_END);
		// long total_bytes = ftell(img);
		// fseek(img, 0, SEEK_SET);

		long total_bytes = 6432; //KÍCH THƯỚC ẢNH VÍ DỤ LÀ 6362

		// if (total_bytes <= 0) {
		// 	printf("File rỗng hoặc lỗi đọc kích thước\n");
		// 	fclose(fbin);
		// 	return;
		// }
		
		uint8_t *data_byte = malloc(total_bytes);
		fread(data_byte, 1, total_bytes, img);
		// long total_blocks = (total_bytes + 15) / 16;
		long total_blocks = total_bytes / 16;
		printf("Total %ld blocks (16 bytes each)\n", total_blocks);
		fclose(img);
		
		printf("Đã đọc %ld bytes từ received_image.jpg\n", total_bytes);
		
		uint8_t *cipher_back = malloc(total_bytes);
		uint32_t *words_send = malloc(4 * sizeof(uint32_t));

		// printf("First 64 bytes of image:\n");
		// int bytes_to_print = (total_bytes < 64) ? total_bytes : 64;
		// printf("Data_bytes picture: \n");
		// for (int i = 0; i < bytes_to_print; i++) {
		// 	printf("%02X ", data_byte[i]);
		// 	if ((i + 1) % 16 == 0) printf("\n");
		// }

		//============================================
		gettimeofday(&t1, NULL);
		int count = 0;
		//============================================

		for (int block = 0; block < total_blocks; block++) { 

			// printf("\n=== Sending block %d ===\n", block);

			for (int i = 0; i <4; i++) {
				int idx = block * 16 + i * 4;
				words_send[i] =  (data_byte[idx] << 24) 	 |
									(data_byte[idx+1] << 16) |
									(data_byte[idx+2] << 8)  |
									data_byte[idx+3];
				// *(FIFO_write_ptr) = words_send[i];
				FIFO_WRITE_BLOCK( words_send[i] );	
			}

			// for (int i = 1; i < 5; i++) {
			// 	printf("return word[%d] = 0x%08X\n", i, *(sram_ptr + i));
			// }
			count += 16;
			while (*(sram_ptr) == 0);
			*(sram_ptr) = 0;  
				
			for (int i = 0; i <4; i++) {
				uint32_t word_data_bytes2 = *(sram_ptr + i + 1);
				int idx = block * 16 + i * 4;
				cipher_back[idx + 0] = (word_data_bytes2>> 24) & 0xFF; 
				cipher_back[idx + 1] = (word_data_bytes2 >> 16) & 0xFF;
				cipher_back[idx + 2] = (word_data_bytes2 >> 8)  & 0xFF;
				cipher_back[idx + 3] = word_data_bytes2 & 0xFF;       
			}
		}

		// printf("\n");
		// printf("data_bytes resend from FPGA = \n");
		// for (int i = 0; i < total_bytes; i++) {
		// 	printf("%02X ", cipher_back[i]);
		// 	if ((i + 1) % 16 == 0) printf("\n");
		// }
		// printf("\n");
		
		FILE *fout = fopen("output_from_fpga.bin", "wb");
		if (!fout) {
			perror("Cannot open output_from_fpga.bin");
			return 1;
		}
		fwrite(cipher_back, 1, total_bytes, fout);
		fclose(fout);
		printf("Saved %ld bytes to file name output_from_fpga.bin successfully!\n", total_bytes);

		printf("\n");
		free(data_byte);
		free(words_send);
		free(cipher_back);

//============================================
		gettimeofday(&t2, NULL);
		elapsedTime = (t2.tv_sec - t1.tv_sec)  * 1000000.0;     // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) ;   // us to 
		// printf("T=%8.0f uSec  \n\r", elapsedTime);
		printf("Encrypt palaintext time = %.2f us for %d words\n", elapsedTime, count);
		double MBps = (count * 4.0) / elapsedTime;
		printf("Encrypt palaintext speed = %.2f MB/s\n", MBps);
//============================================
		
		printf("Nhập 1 để giải mã, khác 1 để thoát : ");
		scanf("%d", &N);
		if( N != 1 ) {
			return 1;
		} else if( N == 1 ) {
			decrypt_from_bin();
		}
//============================================
        break;
	}
	close(fd);
	return 0;
}
 // end main
