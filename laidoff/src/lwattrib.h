#pragma once

#define LW_ATTRIB_DEFENCE_NONE (0)
#define LW_ATTRIB_DEFENCE_WEAK (1)
#define LW_ATTRIB_DEFENCE_RESI (2)
#define LW_ATTRIB_DEFENCE_IMMU (3)

// 0b00 - None
// 0b01 - Weak
// 0b10 - Resist
// 0b11 - Immune
typedef union {
	unsigned int value;
	struct {
		unsigned int air : 2;
		unsigned int wat : 2;
		unsigned int fir : 2;
		unsigned int ear : 2;
		unsigned int god : 2;
		unsigned int evl : 2;
	} bits;
} LWATTRIBVALUE;

#define LW_NON_WEAK (LW_ATTRIB_DEFENCE_NONE)
#define LW_AIR_WEAK (LW_ATTRIB_DEFENCE_WEAK << 0)
#define LW_AIR_RESI (LW_ATTRIB_DEFENCE_RESI << 0)
#define LW_AIR_IMMU (LW_ATTRIB_DEFENCE_IMMU << 0)
#define LW_WAT_WEAK (LW_ATTRIB_DEFENCE_WEAK << 2)
#define LW_WAT_RESI (LW_ATTRIB_DEFENCE_RESI << 2)
#define LW_WAT_IMMU (LW_ATTRIB_DEFENCE_IMMU << 2)
#define LW_FIR_WEAK (LW_ATTRIB_DEFENCE_WEAK << 4)
#define LW_FIR_RESI (LW_ATTRIB_DEFENCE_RESI << 4)
#define LW_FIR_IMMU (LW_ATTRIB_DEFENCE_IMMU << 4)
#define LW_EAR_WEAK (LW_ATTRIB_DEFENCE_WEAK << 6)
#define LW_EAR_RESI (LW_ATTRIB_DEFENCE_RESI << 6)
#define LW_EAR_IMMU (LW_ATTRIB_DEFENCE_IMMU << 6)
#define LW_GOD_WEAK (LW_ATTRIB_DEFENCE_WEAK << 8)
#define LW_GOD_RESI (LW_ATTRIB_DEFENCE_RESI << 8)
#define LW_GOD_IMMU (LW_ATTRIB_DEFENCE_IMMU << 8)
#define LW_EVL_WEAK (LW_ATTRIB_DEFENCE_WEAK << 10)
#define LW_EVL_RESI (LW_ATTRIB_DEFENCE_RESI << 10)
#define LW_EVL_IMMU (LW_ATTRIB_DEFENCE_IMMU << 10)
