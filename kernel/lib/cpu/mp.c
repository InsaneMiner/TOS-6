#include <cpu/mp.h>
#include <stivale2.h>
#include <cpu/cpu_info.h>
#include <mm/pmm.h>

extern void ap_main();

void init_smp(struct stivale2_struct* stivale2_struct){
    struct stivale2_struct_tag_smp* smp_stivale = stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);
    struct stivale2_smp_info* smp_info = smp_stivale->smp_info;
    for(uint64_t i = 0; i < smp_stivale->cpu_count; i++){
        smp_info[i].target_stack = (uint64_t)pmm_alloc(1);
        smp_info[i].goto_address = (uint64_t)&ap_main;
    }
}