/*
 * amc.h
 *
 *  Created on: 29-Jun-2009
 *      Author: poliveir
 */

#ifndef AMC_H_
#define AMC_H_


typedef struct amc_link_info {
	uint8_t	link_grouping_id;	/* [23:16] – Link Grouping ID */
#ifdef BF_MS_FIRST
	uint16_t link_type_extension:4,	/* [15:12] – Link Type Extension */
		link_type:8,		/* [11:4] – Link Type */
		lane_3_port:1,		/* [3] – Lane 3 Port */
		lane_2_port:1,		/* [2] – Lane 2 Port */
		lane_1_port:1,		/* [1] – Lane 1 Port */
		lane_0_port:1;		/* [0] – Lane 0 Port */
#else
	uint16_t lane_0_port:1,
		lane_1_port:1,
		lane_2_port:1,
		lane_3_port:1,
		link_type:8,
		link_type_extension:4;
#endif
} AMC_LINK_INFO;


typedef struct get_amc_port_state_cmd_resp {
	uint8_t	completion_code;	/* Completion Code. */
	uint8_t	picmg_id;		/* PICMG Identifier. Indicates that
					   this is a PICMG®-defined group
					   extension command. A value of
					   00h shall be used. */
					/* The following bytes are optional */
	AMC_LINK_INFO	link_info1;	/* Bytes 3-5: Link info 1. Least significant
					   byte first. This optional field
					   describes information about the first
					   Link associated with the specified AMC
					   Channel ID, if any. If this set of
					   bytes is not provided, the AMC Channel
					   ID does not have any defined Link. */
	uint8_t	state1;			/* State 1. Must be present if Link
					   Info 1 is present. Indicates the
					   first state of the Link.
					   	00h = Disable
						01h = Enable
					   All other values reserved. */
	AMC_LINK_INFO	link_info2;	/* (7:9)Link Info 2. Least significant
					   byte first. Optional. Bit assignments
					   identical to Link Info 1. Used for
					   cases where a second Link has been
					   established. */
	uint8_t	state2;			/* (10)State 2. Bit assignments identical to
					   State 1. */
	AMC_LINK_INFO	link_info3;	/* (11:13)Link Info 3. Least significant
					   byte first. Optional. Bit assignments
					   identical to Link Info 1. Used for
					   cases where a third Link has been
					   established. */
	uint8_t	state3;			/* (14)State 3. Bit assignments identical
					   to State 1. */
	AMC_LINK_INFO	link_info4;	/* (15:17)Link Info 4. Least significant
					   byte first. Optional. Bit assignments
					   identical to Link Info 1. Used for
					   cases where a fourth Link has been
					   established. */
	uint8_t	state4;			/* (18)State 4. Bit assignments identical
					   to State 1. */
} GET_AMC_PORT_STATE_CMD_RESP;







typedef struct get_clock_state_cmd_resp {
	uint8_t	completion_code;	/* Completion Code. */
	uint8_t	picmg_id;		/* PICMG Identifier. Indicates that
					   this is a PICMG®-defined group
					   extension command. A value of
					   00h shall be used. */
					/* Clock Setting */
#ifdef BF_MS_FIRST
	uint8_t	:4,			/* [7:4] Reserved, write as 0h. */
		clock_state:1,		/* [3] - Clock State
					   	0b = Disable
						1b = Enable */
		clock_direction:1,	/* [2] - Clock Direction
					   	0b = Clock receiver
						1b = Clock source */
		pll_control:2;		/* [1:0] - PLL Control
					   	00b = Default state (Command
						      receiver decides the state)
						01b = Connect through PLL
						10b = Bypass PLL (No action if
						      no PLL used)
						11b = Reserved */
#else
	uint8_t	pll_control:2,
		clock_direction:1,
		clock_state:1,
		:4;
#endif

	uint8_t	clock_config_index;	/* (4) Clock Configuration Descriptor Index.
					   Present if the clock is enabled,
					   otherwise absent. */
	uint8_t	clock_family;		/* (5) Clock Family
					   See Table 3-39, “Clock Family
					   definition.” Present if the clock
					   is enabled, otherwise absent. */
	uint8_t	clock_accuracy_level;	/* (6) Clock Accuracy Level.
					   Present if the clock is enabled,
					   otherwise absent. */
					/* The following field is defined based
					   on the Clock Family. */
	unsigned clock_frequency;	/* (7-10) Clock Frequency in Hz.
					   Least Significant Byte First.
					   Present if clock is enabled,
					   otherwise absent. */
} GET_CLOCK_STATE_CMD_RESP;


typedef struct fru_control_capabilities_cmd_resp {
	uint8_t	completion_code;	/* Completion Code. */
	uint8_t	picmg_id;		/* PICMG Identifier. Indicates that
					   this is a PICMG®-defined group
					   extension command. A value of
					   00h shall be used. */
	/* FRU Control Capabilities Mask*/
	uint8_t	:4,			/* [4-7] - Reserved */
		diag_int:1,		/* [33] - 1b - Capable of issuing a diagnostic interrupt */
		graceful_reboot:1,	/* [2] - 1b - Capable of issuing a graceful reboot */
		warm_reset:1, 		/* [1] - 1b - Capable of issuing a warm reset */
		:1;			/* [0] - Reserved */
} FRU_CONTROL_CAPABILITIES_CMD_RESP;



#endif /* AMC_H_ */
