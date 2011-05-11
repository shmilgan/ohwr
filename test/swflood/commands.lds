SECTIONS
{
	.data : {
		. = ALIGN(4);
		__cmd_start = .;
		*(.commands);
		__cmd_end = .;
		}
}
