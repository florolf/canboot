BOARD_ADDR_SIDH = (BOARD_ADDR >> 21) & 0xFF
BOARD_ADDR_SIDL = ((BOARD_ADDR >> 13) & 0xE0) | 1 << 3 | ((BOARD_ADDR >> 16) & 0x03)
BOARD_ADDR_EID8 = (BOARD_ADDR >> 8) & 0xFF
BOARD_ADDR_EID0 = BOARD_ADDR & 0xFF

.section .progmem.data
mcp_config:
	.byte BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, BOARD_ADDR_EID8, BOARD_ADDR_EID0,	/* Filter 1.1 */ \
	      BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, BOARD_ADDR_EID8, BOARD_ADDR_EID0,	/* Filter 1.2 */ \
	      BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, BOARD_ADDR_EID8, BOARD_ADDR_EID0,	/* Filter 1.3 */ \
	      1 << 3 | 1 << 2,				/* BFPCTRL: Enable outputs */ \
	      0,					/* TXRTSCTRL: Enable inputs */ \
	      0,					/* CANSTAT: readonly */ \
	      1 << 7,					/* CANCTRL: Configuration mode */ \
	      BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, BOARD_ADDR_EID8, BOARD_ADDR_EID0,	/* Filter 2.3 */ \
	      BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, BOARD_ADDR_EID8, BOARD_ADDR_EID0,	/* Filter 2.4 */ \
	      BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, BOARD_ADDR_EID8, BOARD_ADDR_EID0,	/* Filter 2.5 */ \
	      0, 0, 0,					/* TEC, REC, CANSTAT: readonly */ \
	      1 << 7,					/* CANCTRL: again */ \
	      0xFF, 0xFF, 0xFF, 0xFF,				/* Filter mask 1 */ \
	      0xFF, 0xFF, 0xFF, 0xFF,				/* Filter mask 2 */ \
	      0x05, 0xB1, 0x44,				/* CNF: See busmaster */ \
	      0x01,					/* CANINTE: RX0 */ \
	      0, 0, 0,					/* CANINTF, EFLG, CANSTAT */ \
	      1 << 7,					/* CANCTRL: again */ \
	      0,					/* TXB0CTRL */ \
	      BOARD_ADDR_SIDH, BOARD_ADDR_SIDL, /* Initial Sender Address */ \
	      BOARD_ADDR_EID8, BOARD_ADDR_EID0, \
	      8						/* DLC */ 
mcp_magic:
	.byte 'B', 'O', 'O', 'T', 'M', 'A', 'G', 'C'   /* Magic message */
