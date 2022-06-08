# build a reduced library that supports BSDL parsing only
set(JTAG_BSDL_SRC
    bsdl_parser/jtag_bsdl.c
    bsdl_parser/bsdl_strings.c
    natsort/strnatcmp.c
)

add_library(jtag_bsdl ${JTAG_BSDL_SRC})

