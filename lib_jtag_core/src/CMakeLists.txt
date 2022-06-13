# build a reduced library that supports BSDL parsing only
set(JTAG_BSDL_SRC
    bsdl_parser/jtag_bsdl.c
    bsdl_parser/bsdl_strings.c
    natsort/strnatcmp.c
    dbg_logs.c
)

add_library(jtag_bsdl ${JTAG_BSDL_SRC})
set_property(TARGET jtag_bsdl PROPERTY POSITION_INDEPENDENT_CODE ON)

