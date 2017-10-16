Extracts code page fonts from a CPI file into a hex byte array.

cpi2hex \<file\>

Options:

	-i		List information only, don't output to file
	-o <name>	Specify an output file name (font.h by default)
	-b		Output data as a raw binary files (-o option will be ignored)
	-c <number>	Specify the code page to extract
	-r <range>	Specify a range of characters to extract. Multiple
			ranges can be specified seprated by commas eg: -r 32-167,57,2-4
	-d		Print debug information about file headers
