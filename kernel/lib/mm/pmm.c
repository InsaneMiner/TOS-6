#include <mm/pmm.h>
#include <debug.h>

uint16_t number_of_entrys; 

void bitmap_setb(uint64_t index){
	bitmap[index / 8] |= 1 << (index % 8);
}

void bitmap_clearb(uint64_t index){
	bitmap[index / 8] &= ~(1 << (index % 8));
}

uint64_t bitmap_getb(uint64_t index){
	return !!(bitmap[index / 8] & (1 << (index % 8)));
}

static inline uint64_t round_up(uint64_t number, uint64_t alignment){
	return number % alignment == 0 ? number : (number + (alignment - number % alignment));
}

static inline uint64_t round_down(uint64_t number, uint64_t alignment){
	return number % alignment == 0 ? number : (number - (number - alignment));
}

static inline uint64_t difference(uint64_t number1, uint64_t number2){
	return number2 > number1 ? number2 - number1 : number1 - number2;
}

void init_bitmap(struct stivale2_struct* stivale2_struct){

	memmap = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
	kernel_size = _kernel_end - _kernel_start;

	bitmap_size = (memmap->memmap[memmap->entries - 1].base + memmap->memmap[memmap->entries - 1].length) / 0x1000 / 8;

	block_limit = bitmap_size * 8;
	bitmap = (uint8_t*) 0x0;
	
	for(uint64_t i = 0; i < memmap->entries; i++){
		if(memmap->memmap[i].type == STIVALE2_MMAP_USABLE && memmap->memmap[i].length >= bitmap_size){
			bitmap = (uint8_t*)(memmap->memmap[i].base);
			break;
		} 
	}

	for(uint64_t i = 0; i < memmap->entries; i++){
		bitmap[i] = 0;
	}

	print("Bitmap: ");
	printhex((uint64_t) bitmap);
	assert((uint64_t) bitmap == 0, "Couldn't allocate bitmap");
	
}

void populate_bitmap(){
	if(memmap->memmap[0].base != 0){
		for(uint64_t i = 0; i < round_up(memmap->memmap[0].base, PMM_PAGE_SIZE) / PMM_PAGE_SIZE; i++){
			bitmap_setb(i);
		}
	}

	for(uint64_t memmap_entry = 0; memmap_entry < memmap->entries; memmap_entry++){
		if(memmap->memmap[memmap_entry].type != STIVALE2_MMAP_USABLE){
			for(uint64_t i = 0; i < round_up(memmap->memmap[memmap_entry].length, PMM_PAGE_SIZE) / PMM_PAGE_SIZE; i++){
				bitmap_setb(i + round_down(memmap->memmap[memmap_entry].base, PMM_PAGE_SIZE) / PMM_PAGE_SIZE);	
			}
		}
		if(memmap_entry != memmap->entries - 1){
			if(memmap->memmap[memmap_entry].length + memmap->memmap[memmap_entry].base != )
		}
	}
}