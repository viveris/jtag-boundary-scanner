# Microsoft Developer Studio Project File - Name="libjtagcore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libjtagcore - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libjtagcore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libjtagcore.mak" CFG="libjtagcore - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libjtagcore - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libjtagcore - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libjtagcore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\src\os_interface\win32" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libjtagcore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\src\os_interface\win32" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libjtagcore - Win32 Release"
# Name "libjtagcore - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "bsdl_parser"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\bsdl_parser\bsdl_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\bsdl_parser\bsdl_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\bsdl_parser\bsdl_strings.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\bsdl_parser\bsdl_strings.h
# End Source File
# End Group
# Begin Group "bus_over_jtag"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\bus_over_jtag\i2c_over_jtag.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\bus_over_jtag\mdio_over_jtag.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\bus_over_jtag\memory_over_jtag.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\bus_over_jtag\spi_over_jtag.c
# End Source File
# End Group
# Begin Group "probes_drivers"

# PROP Default_Filter ""
# Begin Group "ftdi_jtag"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\drivers\ftdi_jtag\ftdi\ftd2xx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\ftdi_jtag\ftdi_jtag_drv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\ftdi_jtag\ftdi_jtag_drv.h
# End Source File
# End Group
# Begin Group "lpt_jtag"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\drivers\lpt_jtag\lpt_jtag_drv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\lpt_jtag\lpt_jtag_drv.h
# End Source File
# End Group
# Begin Group "jlink_jtag"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\drivers\jlink_jtag\jlink_jtag_drv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\jlink_jtag\jlink_jtag_drv.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\src\drivers\drivers_list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\drivers_list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\drv_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\drivers\drv_loader.h
# End Source File
# End Group
# Begin Group "script"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\script\env.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\script\env.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\script\script.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\script\script.h
# End Source File
# End Group
# Begin Group "os_interface"

# PROP Default_Filter ""
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\os_interface\win32\stdint.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\src\os_interface\fs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\os_interface\network.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\os_interface\network.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\os_interface\os_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\os_interface\os_interface.h
# End Source File
# End Group
# Begin Group "natsort"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\natsort\strnatcmp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\natsort\strnatcmp.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\src\dbg_logs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\dbg_logs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jtag_core.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jtag_core.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jtag_core_internal.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
