#include <pci/pci_e.h>
#include <pci/pci_e_headers.h>
#include <rsdt.h>
#include <debug.h>
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <pci/capabilities/msi_capab.h>

MCFG* mcfg = 0;
ECM_info_struct* ecm_info_structs;
uint64_t ecm_info_struct_count;

static uint16_t get_vendor_id(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->vendor_id;
}

/*static uint16_t get_device_id(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->device_id;
}*/

/*static uint16_t get_status(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->status;
}*/

/*static uint8_t get_revision_id(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->revision_id;
}*/

static uint8_t get_prog_if(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->prog_if;
}

static uint8_t get_subclass(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->subclass;
}

static uint8_t get_class_code(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->class_code;
}

/*static uint8_t get_cache_line_size(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->cache_line_size;
}*/

/*static uint8_t get_latency_timer(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->latency_timer;
}*/

static uint8_t get_header_type(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->header_type;
}

/*static uint8_t get_bist(void* ecm_address){
    return ((PCIE_std_header*)ecm_address)->bist;
}*/

void* get_ecm_address(uint8_t bus, uint8_t device, uint8_t function){

    for(uint64_t i = 0; i < ecm_info_struct_count; i++){
        if(bus >= ecm_info_structs[i].start_pci_bus_number && bus <= ecm_info_structs[i].end_pci_bus_number){
            return (void*)(ecm_info_structs[i].enhanced_config_space_base + ((bus - ecm_info_structs[i].start_pci_bus_number) << 20 | device << 15 | function << 12));
        }
    }
    panic("Couldn't get ECM address for specified PCI device!");

    return 0;
}

static uint8_t has_capabilities_pointer(void* ecm_address){
    return !!(((PCIE_std_header*)ecm_address)->status & 0x10);
}

static uint8_t device_exists(uint8_t bus, uint8_t device, uint8_t function){
    return ((PCIE_std_header*)get_ecm_address(bus, device, function))->vendor_id != 0xFFFF ? 1 : 0;
}

void* get_pcie_capabilities_addr(uint8_t bus, uint8_t device, uint8_t function){
    void* addr = get_ecm_address(bus, device, function);

    if(!device_exists(bus, device, function)){
        return 0;
    }

    if(!has_capabilities_pointer(addr)){
        return 0;
    }

    if(!(get_header_type(addr) & 0x80) && ((get_header_type(addr) & (~0x80)) != 0x2)){
        return (void*)(addr + ((PCIE_header_type_0*)addr)->capabilities_pointer);
    }

    return 0;

}

void* get_pcie_capability(uint8_t capability_id, uint8_t bus, uint8_t device, uint8_t function){
    for(uint8_t next_ptr = *((uint8_t*)get_pcie_capabilities_addr(bus, device, function) + 1); next_ptr; next_ptr = *((uint8_t*)get_pcie_capabilities_addr(bus, device, function) + 1)){
        if(*(uint8_t*)((uint64_t)get_ecm_address(bus, device, function) + next_ptr) == capability_id){
            return (void*)((uint64_t)get_ecm_address(bus, device, function) + next_ptr);
        } 
    }

    return 0;
}

void set_msi_address(MSI_capability* data, uint8_t vector, __attribute__((unused)) uint32_t processor, uint8_t edgetrigger, uint8_t deassert){
    *((uint64_t*)((uint64_t)data->message_address)) = vector | (edgetrigger == 1 ? 0 : (1 << 15)) | (deassert == 1 ? 0 : (1 << 14));
}

static uint16_t search_bus(uint8_t bus, uint8_t class, uint8_t subclass, uint8_t progif){
    
    for(uint8_t device = 0; device < 32; device++){
        if(get_vendor_id(get_ecm_address(bus, device, 0)) == 0xFFFF) continue;
        if((get_header_type(get_ecm_address(bus, device, 0)) & 0x80) != 0){
            for(uint8_t function = 1; function < 8; function++){
                void* addr = get_ecm_address(bus, device, function);
                if(get_vendor_id(addr) != 0xFFFF){
                    if(get_class_code(addr) == 0x6 && get_subclass(addr) == 0x4){
                        uint16_t result = search_bus(((PCIE_header_type_1*)addr)->secondary_bus_number, class, subclass, progif);
                        if(result != UINT16_MAX) return result;
                    }
                    if(get_class_code(addr) == class && get_subclass(addr) == subclass && get_prog_if(addr) == progif){
                        return (uint16_t)device | (function << 8);
                    }
                }
            }
        }
    }
    return UINT16_MAX;
}

PCIE_device_struct get_pcie_device(uint8_t class, uint8_t subclass, uint8_t progif){
    PCIE_device_struct device_struct = {.bus = 0, .device = 0, .ecma = 0, .function = 0, .segment_group = 0};
    for(uint64_t i = 0; i < ecm_info_struct_count; i++){
        for(uint64_t curr_bus = ecm_info_structs[i].start_pci_bus_number; curr_bus < ecm_info_structs[i].end_pci_bus_number; curr_bus++){
            uint32_t result = search_bus(curr_bus, class, subclass, progif);
            if(result != UINT16_MAX){
                device_struct.bus = curr_bus;
                device_struct.device = (uint8_t)result;
                device_struct.function = result >> 8;
                device_struct.ecma = get_ecm_address(curr_bus, (uint8_t) result, result >> 8);
                device_struct.segment_group = ecm_info_structs[i].pci_segment_group;
            }
        }
    }
    return device_struct;
}

void init_pci(){
    mcfg = (MCFG*)find_sdt_entry("MCFG");

    ecm_info_struct_count = (mcfg->header.length - sizeof(ACPISDTheader) - sizeof(uint64_t)) / 16;

    ecm_info_structs = kmalloc(ecm_info_struct_count * sizeof(ECM_info_struct));

    for(uint64_t i = 0; i < ecm_info_struct_count; i++){
        ecm_info_structs[i].end_pci_bus_number = mcfg->config_space_structs[i].end_pci_bus_number;
        ecm_info_structs[i].start_pci_bus_number = mcfg->config_space_structs[i].start_pci_bus_number;
        ecm_info_structs[i].pci_segment_group = mcfg->config_space_structs[i].pci_segment_group;
        ecm_info_structs[i].enhanced_config_space_base = mcfg->config_space_structs[i].enhanced_config_space_base;
    }

}
