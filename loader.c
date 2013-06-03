#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "elf.h"

#define kprintf(fmt, ... )do{ /*add printf alike here if needed */ }while(0)

/* just a test function*/
void pstr(char* str)
{
	kprintf("%s", str );
}

void printk(const char* msg)
{
	kprintf(msg);
}

/* check that it has a valid elf header */
int is_image_valid(Elf32_Ehdr *hdr)
{
	char* h = (char*)hdr;
	if( h[0] == 0x7f && h[1] ==  0x45 && h[2] == 0x4c ) {
		return 1;
	}
	kprintf("Bad ELF!\n\r");
	return 0;
}

#define PROT_READ 1
#define PROT_EXEC 2
char* modes[] = {"-","R", "X"};
/* 
 * set memory protection for area 
 * if OS/chip supports memory protection
 * */
void memprotect(unsigned char* addr, size_t size, int mode )
{
	kprintf("mprotect %x size: %d mode: %s\n\r", addr, size, modes[mode] ); 
}

//malloc
static inline void* memget(size_t a_size)
{
	void* mem;
	//mem=memalign(sizeof(int), a_size );
	mem=malloc(a_size);
	kprintf("Image @ %x %d\n\r",mem, a_size);
	return mem;
}

//free
static inline void memfree( void* amem )
{
	free( amem );
}

static void unresolved()
{
	kprintf("unresolved sym!\n\r");
	while(1)
		;
}

void listTasks();
/*
 * resolv symbol address (lookup the function and return address to it )
 */
void *resolve(const char* sym)
{
	void *handle = unresolved;
	kprintf("lookup '%s'",sym);
	// here we need to return the address to a sym (i.e. function, printf, exit, etc..)
	if(!strcmp(sym,"pstr")) {
		kprintf(" @ 0x%x ", pstr );		
		handle = pstr;
	}else if( !strcmp(sym,"listTasks")) {
		kprintf(" @ 0x%x ", listTasks);		
		handle= listTasks;
	}
//	kprintf("\n\r");
	return handle;
}

/*
 * perform relocation for needed symbols
 */
void relocate(Elf32_Shdr* shdr, const Elf32_Sym* syms, const char* strings, const char* src, char* dst)
{
	Elf32_Rel* rel = (Elf32_Rel*)(src + shdr->sh_offset);
	int prev;
	int j;
	for(j = 0; j < shdr->sh_size / sizeof(Elf32_Rel); j += 1) 
	{
		const char* sym = strings + syms[ELF32_R_SYM(rel[j].r_info)].st_name;
		switch(ELF32_R_TYPE(rel[j].r_info)) 
		{
			case R_ARM_JUMP_SLOT: 		/* plt entry, function calls shared libs */
			{
				kprintf("PLT entry - ");
				prev = *((int*)(Elf32_Word*)(dst + rel[j].r_offset));
			//	*(dst + syms[j].st_value) =(Elf32_Word)resolve(sym);
				*(Elf32_Word*)(dst + rel[j].r_offset) = (Elf32_Word)resolve(sym); //maybe += ?
				kprintf("Reloc 0x%x - 0x%x (addr: 0x%x offs:%x)\n\r", 
					prev, 
					*(Elf32_Word*)(dst + rel[j].r_offset), 
					(dst + rel[j].r_offset), 
					rel[j].r_offset );
			}break;
			case R_ARM_GLOB_DAT: 		/* got entry */
				kprintf("GOT entry - ");
				prev = *((int*)(Elf32_Word*)(dst + rel[j].r_offset));
				*(Elf32_Word*)(dst + rel[j].r_offset) = (Elf32_Word)resolve(sym);
				kprintf("Reloc 0x%x - 0x%x (addr: 0x%x offs:%x)\n\r", 
					prev, 
					*(Elf32_Word*)(dst + rel[j].r_offset), 
					(dst + rel[j].r_offset), 
					rel[j].r_offset );

			break;
			default:
				kprintf("reloc %x\n\r", ELF32_R_TYPE(rel[j].r_info) );
			break;
		}
	}
}

void listsyms(Elf32_Shdr* shdr, const Elf32_Sym* syms, const char* strings, const char* src, char* dst)
{
	Elf32_Rel* rel = (Elf32_Rel*)(src + shdr->sh_offset);
	int prev;
	int j;
	for(j = 0; j < shdr->sh_size / sizeof(Elf32_Rel); j += 1) 
	{
		const char* sym = strings + syms[ELF32_R_SYM(rel[j].r_info)].st_name;
		kprintf("sym: %s\n\r", sym );
	}
}


void* find_sym(const char* name, Elf32_Shdr* shdr, const char* strings, const char* src, char* dst)
{
	Elf32_Sym* syms = (Elf32_Sym*)(src + shdr->sh_offset);
	int i;
//	kprintf("Lookup sym %s =>", name );
	for(i = 0; i < shdr->sh_size / sizeof(Elf32_Sym); i += 1) {
		if (strcmp(name, strings + syms[i].st_name) == 0) {
			kprintf("symbol: '%s' is @ %x\n\r", name, dst + syms[i].st_value );
			return dst + syms[i].st_value;
		}
	}
	return NULL;
}

/*
 * load elf file, file in buffer elf_start, size of buffer size
 */
void *image_load (char *elf_start, unsigned int size)
{
	Elf32_Ehdr      *hdr     = NULL;
	Elf32_Phdr      *phdr    = NULL;
	Elf32_Shdr      *shdr    = NULL;
	Elf32_Sym       *syms    = NULL;
	char            *strings = NULL;
	char            *start   = NULL;
	char            *taddr   = NULL;
	void            *entry   = NULL;
	int i = 0;
	char *exec = NULL;
	hdr = (Elf32_Ehdr *) elf_start;
	if(!is_image_valid(hdr)) {
		printk("image_load:: invalid ELF image\n");
		return 0;
	}
	exec = memget( size );  /* allocate memory for binary, might need to be aligned ? */
	if(!exec) {
		printk("image_load:: error allocating memory\n");
		return 0;
	}
	// Start with clean memory.
	memset(exec,0x0,size);

	/*iterrate prg headers (sections) */
	phdr = (Elf32_Phdr *)(elf_start + hdr->e_phoff);
	for(i=0; i < hdr->e_phnum; ++i) 
	{
		switch( phdr[i].p_type )
		{
			case PT_NULL:
			{
				kprintf("header: null\n\r");
			}break;
			case PT_LOAD:
			{
				kprintf("header: load ");
				if(phdr[i].p_filesz > phdr[i].p_memsz) {
					printk("image_load:: p_filesz > p_memsz\n");
					memfree(exec);
					return 0;
				}
				if(!phdr[i].p_filesz) {
					continue;
				}
				// p_filesz can be smaller than p_memsz,
				// the difference is zeroe'd out.
				start = elf_start + phdr[i].p_offset;
				taddr = exec      + phdr[i].p_vaddr;
				kprintf(" cp %x -> %x (%d bytes) \n\r", taddr, start, phdr[i].p_filesz);
				memcpy(taddr,start,phdr[i].p_filesz);
#if 0
				if(!(phdr[i].p_flags & PF_W)) {
					// Read-only.
					memprotect((unsigned char *) taddr, phdr[i].p_memsz, PROT_READ);
				}
				if(phdr[i].p_flags & PF_X) {
					// Executable.
					memprotect((unsigned char *) taddr, phdr[i].p_memsz, PROT_EXEC);
				}
#endif				
			}break;
			case PT_DYNAMIC:
			{
				kprintf("header: dynamic\n\r");
			}break;
			case PT_NOTE:
			{
				kprintf("header: note\n\r");
			}break;
			case PT_INTERP:
			{
				kprintf("header: interp\n\r");
			}break;
			case PT_PHDR:
			{
				kprintf("header: prg\n\r");
			}break;
			default:
			{
				kprintf("ukn header:%d p_type %x\n\r", i, phdr[i].p_type );
			}break;
		}
	}
	/* itterate section headers */
	shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);
	for(i=0; i < hdr->e_shnum; ++i) 
	{
		kprintf("section: %2d type: ", i ); 
		switch( shdr[i].sh_type )
		{
			case SHT_NULL:
			{
				kprintf("null ");
			}break;
			case SHT_PROGBITS:
			{
				kprintf("prgbits ");
			}break;
			case SHT_STRTAB:
			{
				kprintf("strtab ");
			}break;
			case SHT_HASH:
			{
				kprintf("hash ");
			}break;
			case SHT_DYNAMIC:
			{
				kprintf("dynamic ");
			}break;
			case SHT_MIPS_GPTAB:
			{
				kprintf("GPTAB? ");
			}break;
			case SHT_REL:
			{
				int n;
				for(n=0; n < hdr->e_shnum; ++n) 
				{
					if( shdr[n].sh_type == SHT_DYNSYM )
					{
						syms = (Elf32_Sym*)(elf_start + shdr[n].sh_offset);
						strings = elf_start + shdr[shdr[n].sh_link].sh_offset;
					}
				}
				//kprintf("relocation (%x, %x)", syms, strings);
				//listsyms(shdr + i, syms, strings, elf_start, exec);
				relocate(shdr + i, syms, strings, elf_start, exec);
			}break;
			case SHT_DYNSYM: 		/* here we will find __data_start, _edata, __bss_start etc */
			{
				syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);
				strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
				kprintf("dynsym (%x, %x)",syms, strings );
	//			listsyms(shdr + i, syms, strings, elf_start, exec);
			}break;
			case SHT_SYMTAB:  		/* here all program symbols are found, main, _edata, runmain etc */
			{
				syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);
				strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
				kprintf("symtab (%x, %x)\n\r",syms, strings );
				entry = find_sym("_runmain", shdr + i, strings, elf_start, exec);
			}break;
			default:
			{
				kprintf("unknown %x ", (shdr[i].sh_type));
			}break;
		}
		kprintf("\n\r");
	}
	return entry;
}

/*
 *
 * Call this function with a_image containing the elf file, and a_imagesize the size of a_image 
 */
void* load( char* a_image, size_t a_imagesize )
{
	int (*ptr)(int, char **, char**);
	ptr=image_load(a_image, a_imagesize );
	return ptr; 
}

