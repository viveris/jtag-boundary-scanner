#define VDIG1 2
#define VDIG2 6
#define VDIG3 6
#define VDIG4 1

#define STR_DATE "16 Oct 2024"

#define vxstr(s) vstr(s)
#define vstr(s) #s

#define LIB_JTAG_CORE_VERSION  vxstr(VDIG1) "." vxstr(VDIG2) "." vxstr(VDIG3) "." vxstr(VDIG4)
#define LIB_JTAG_CORE_VERSION_COMMA  vxstr(VDIG1) "," vxstr(VDIG2) "," vxstr(VDIG3) "," vxstr(VDIG4)

#define APP_VER          VDIG1.VDIG2.VDIG3.VDIG4
#define APP_VER_TXT( N ) JTAG Boundary Scanner v##N
#define APP_VER_STR( N ) vxstr( APP_VER_TXT( N ) )
