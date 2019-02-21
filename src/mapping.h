#ifdef OLT_INTERN_MAPPING_H
#error multiple inclusion
#endif
#define OLT_INTERN_MAPPING_H

typedef long mapping_handle_t;

int olt_INTERN_unmap_file(void *addr, mapping_handle_t mapping);
int olt_INTERN_map_file(char const *filename, void **addr, mapping_handle_t *mapping);
