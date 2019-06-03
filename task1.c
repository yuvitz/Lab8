#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <sys/mman.h>
#include <limits.h>

//global vars
int curr_fd=0;
int debug_mode=0;
void *map_start; /* will point to the start of the memory mapped file */
struct stat fd_stat; /* this is needed to  the size of the file */
Elf32_Ehdr *header; /* this will point to the header structure */


//structs

struct fun_desc{
	char *name;
	void (*func)();
};

void toggle_debug(){
	debug_mode=(debug_mode+1)%2;
	if(debug_mode)
		fprintf(stderr, "%s\n", "Debug flag now on");
	else
		printf("%s\n", "Debug flag now off");
}

void examine(){
	char file_name[128];
	//int //num_of_section_headers;
	printf("%s\n", "Please enter a file name");
	fgets(file_name, 128, stdin);
	file_name[strlen(file_name)-1]='\0';
	if(curr_fd>0){
		munmap(map_start, fd_stat.st_size);
		close(curr_fd);}

   if((curr_fd = open(file_name, O_RDWR)) < 0 ) {
      perror("error in open");
      exit(-1);
   }

   if( fstat(curr_fd, &fd_stat) != 0 ) {
      perror("stat failed");
      exit(-1);
   }

   if ( (map_start = mmap(0, fd_stat.st_size, PROT_READ | PROT_WRITE , MAP_SHARED, curr_fd, 0)) == MAP_FAILED ) {
      perror("mmap failed");
      exit(-4);
   }

   /* now, the file is mapped starting at map_start.
    * all we need to do is tell *header to point at the same address:
    */
   	header = (Elf32_Ehdr *) map_start;
   	if(header->e_ident[EI_MAG1]!='E'||header->e_ident[EI_MAG2]!='L'||header->e_ident[EI_MAG3]!='F'){
   		printf("NOT an ELF file");
		exit(-1);}
   	
	printf("First 3 bytes of magic number are: %d %d %d\n", header->e_ident[EI_MAG1], header->e_ident[EI_MAG2], header->e_ident[EI_MAG3]);
   		
	printf("Data encoding scheme is: ");
	switch(header->e_ident[EI_DATA]){
		case 1: printf("Two's complement, little-endian.\n");
			break;
		case 2: printf("Two's complement, big-endian.\n");
			break;	
		default:  printf("Unknown data format.\n");
	}
   	printf("Entry point address: %#x\n", header->e_entry);

   	printf("Start of section headers: %d (bytes into file)\n", header->e_shoff);

   	printf("Number of section headers: %d\n", header->e_shnum);

   	printf("Size of section headers: %d\n", header->e_shentsize);
	
	printf("Start of program headers: %d (bytes into file)\n", header->e_phoff);

   	printf("Number of program headers: %d\n", header->e_phnum);

   	printf("Size of program headers: %d\n", header->e_phentsize);


   /* now we can do whatever we want with header!!!!
    * for example, the number of section header can be obtained like this:
    */
   //num_of_section_headers = header->e_shnum;

   /* now, we unmap the file */
   	
}

//next 4 fuctions were copied from https://wiki.osdev.org/ELF_Tutorial#Accessing_Section_Headers, and were modified by me to fit the code
static inline Elf32_Shdr *elf_sheader(Elf32_Ehdr *hdr) {
    return (Elf32_Shdr *)((int)hdr + hdr->e_shoff);
}

static inline Elf32_Shdr *elf_section(Elf32_Ehdr *hdr, int idx) {
    return &elf_sheader(hdr)[idx];
}
static inline char *elf_str_table(Elf32_Ehdr *hdr) {
    if(hdr->e_shstrndx == SHN_UNDEF) return NULL;
    return (char *)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}

static inline char *elf_lookup_string(Elf32_Ehdr *hdr, int offset) {
    char *strtab = elf_str_table(hdr);
    if(strtab == NULL) return NULL;
    return strtab + offset;
}

char *type_name(uint32_t type){
	switch(type){
		case 1:
			return "PROGBITS";
			break;
		case 2:
			return "SYMTAB";
			break;	
		case 3:
			return "STRTAB";
			break;
		case 4:
			return "RELA";
			break;
		case 5:
			return "HASH";
			break;
		case 6:
			return "DYNAMIC";
			break;
		case 7:
			return "NOTE";
			break;
		case 8:
			return "NOBITS";
			break;
		case 9:
			return "REL";
			break;
		case 10:
			return "SHLIB";
			break;
		case 11:
			return "DYNSYM";
			break;	
		default: return "NULL";	
	}
}

void print_sections(){
	Elf32_Shdr* curr_section;
	char* sec_name, *sec_type;
	int sec_size, sec_offset, sec_address;
	printf("Index\tName\t\tAddress\tOffset\tSize\tType\n"); 
	for(int i=0; i<header->e_shnum; i++){
		curr_section= elf_section(header, i);
		sec_name=elf_lookup_string(header, curr_section->sh_name);
		sec_address= curr_section->sh_addr;
		sec_offset= curr_section->sh_offset;
		sec_size= curr_section->sh_size;
		sec_type= type_name(curr_section->sh_type);			
		printf("%d\t%s\t\t%#x\t%#x\t%#x\t%s\n", i, sec_name, sec_address, sec_offset, sec_size, sec_type); 
	}
}
void quit(){
	munmap(map_start, fd_stat.st_size);
	exit(0);
}	

//end of methods

int main(int argc, char **argv)
{	
	char opt[4];
	int chosen, desc_size;
	struct fun_desc func_array[] ={
	{"Toggle Debug Mode", toggle_debug},
	{"Examine ELF File", examine},
	{"Print Section Names", print_sections},
	{"Quit", quit}};
	desc_size=sizeof(func_array)/sizeof(*func_array);

	while(1){
		int i;
		printf("%s\n", "Choose action:");
		for(i=0; i<desc_size; i++){
			printf("%d", i);
			printf("- %s\n", func_array[i].name);
		}
		fgets(opt, sizeof(opt), stdin);
		chosen= atoi(opt);
		printf("%s", "Option: ");
		printf("%s\n", opt);
		if((chosen>=0) && (chosen <desc_size))
			func_array[chosen].func();
		else{
			printf("%s\n", "Not within bounds");
            exit(0);
        }
	}
	return 0;
}