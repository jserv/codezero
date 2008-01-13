/*
 * The next free p_align'ed LMA base address
 *
 * p_align = 0x8000
 *
 * Recap from ELF spec: p_align: Loadable process segments must have
 * congruent values for p_vaddr and p_offset, modulo the page size. 
 * This member gives the value to which the segments are aligned in 
 * memory and in the file. Values 0 and 1 mean that no alignment is 
 * required. Otherwise, p_align should be a positive, integral power
 * of 2, and p_addr should equal p_offset, modulo p_align. 
 * This essentially means next available address must be aligned at
 * p_align, rather than the page_size, which one (well, I) would 
 * normally expect. 
 */

physical_base = 0x20800;
