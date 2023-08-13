BL33H_FILTER := 	lib/cpus/% \

BL31_SOURCES_NOW :=	${BL31_SOURCES}
BL33H_SOURCES +=	$(filter-out ${BL33H_FILTER}, ${BL31_SOURCES_NOW}) \
			bl33h/aarch64/cpu_helpers.S \

BL33H_DEFAULT_LINKER_SCRIPT_SOURCE := ${BUILD_BASE}/bl33h.ld.S
$(BL33H_DEFAULT_LINKER_SCRIPT_SOURCE): bl31/bl31.ld.S
	sed -e 's/BL31/BL33H/g' -e 's/__CPU_OPS_END__ > __CPU_OPS_START__/true/g' $< > $@

BUILD_DIR := $(BUILD_PLAT)/bl33h
bl33h_el3_check: $(call IMG_ELF,bl33h)
	@! $(OD) -d $< | grep -e 'mrs' -e 'msr' | grep el3 || :
bl33h: bl33h_el3_check
