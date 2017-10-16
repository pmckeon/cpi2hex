/********************************************************************************************
* cpi2hex
* A utility to extract code page fonts as 1 bit per pixel hex data
* that can be loaded into bitmapped displays.
*
* Description of CPI file format sourced from:
* http://www.seasip.info/DOS/CPI/cpi.html
*
* cpi2hex is free and open source software provided under the terms of The MIT License (MIT)
* See accompanying license file for details.
*
* Copyright (C) 2017 by Peter McKeon 15/10/2017
*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct
{
	unsigned char  id0;
	char  id[7];
	char  reserved[8];
	short pnum;
	char  ptyp;
	long  fih_offset;
} FontFileHeader;

struct
{
	char num_fonts_per_codepage;
	char *font_cellsize;
	long *dfd_offset;
} DRDOSExtendedFontFileHeader;

struct
{
	short num_codepages;
} FontInfoHeader;

struct
{
	short cpeh_size;
	long next_cpeh_offset;
	short device_type;
	char device_name[8];
	short codepage;
	char reserved[6];
	long cpih_offset;
} CodePageEntryHeader;

struct
{
	short version;
	short num_fonts;
	short size;
} CodePageInfoHeader;

struct
{
	char height;
	char width;
	char yaspect;
	char xaspect;
	short num_chars;
} ScreenFontHeader;

struct
{
	short FontIndex[256];
} CharacterIndexTable;

struct
{
	unsigned int info : 1;
	unsigned int debug : 1;
	unsigned int binary : 1;
	short codepage;
	int range[20][2];
	int num_ranges;
} options;

int main(int argc, char *argv[])
{
	FILE *fp;
	char infile[256];
	char outfile[256] = "font.h";

	if (argc < 2)
	{
		printf(
			"Extracts code page fonts from a CPI file into a hex byte array.\n\n"
			"cpi2hex <file>\n\n"
			"Options:\n"
			"\t-i\t\tList information only, don't output to file\n"
			"\t-o <name>\tSpecify an output file name (font.h by default)\n"
			"\t-b\t\tOutput data as a raw binary files (-o option will be ignored)\n"
			"\t-c <number>\tSpecify the code page to extract\n"
			"\t-r <range>\tSpecify a range of characters to extract. Multiple\n"
			"\t\t\tranges can be specified seprated by commas eg: -r 32-167,57,2-4\n"
			"\t-d\t\tPrint debug information about file headers\n"
		);
		exit(0);
	}

	options.num_ranges = 0;
	for (int n = 1; n < argc; n++)
	{
		switch ((int)argv[n][0])
		{
		case '-':
		case '/':
			switch ((char)argv[n][1])
			{
			case 'o':
				if (n+1 == argc)
				{
					printf("Error: No output file specified after -o\n");
					exit(1);
				}
				strcpy(outfile, argv[++n]);
				break;
			case 'b':
				options.binary = 1;
				break;
			case 'i':
				options.info = 1;
				break;
			case 'c':
				if (n + 1 == argc)
				{
					printf("Error: No code page specified after -c\n");
					exit(1);
				}
				options.codepage = atoi(argv[++n]);
				break;
			case 'r':
				if (n + 1 == argc)
				{
					printf("Error: No range specified after -r\n");
					exit(1);
				}
				
				char *pair = strtok(argv[++n], ",");
				while (pair != NULL)
				{
					int start, end;
					int num = sscanf(pair, "%d-%d", &start, &end);
					if (num <= 0)
					{
						printf("Error: Invalid argument '%s' after -r\n", pair);
						exit(1);
					}
					if (num == 1)
						end = start;

					if (start < 0)
						start = 0;
					if (start > 255)
						start = 255;
					if (end > 255)
						end = 255;

					if (end < start)
					{
						printf("Error: Ending range can not be smaller than starting range\n");
						exit(1);
					}

					options.range[options.num_ranges][0] = start;
					options.range[options.num_ranges][1] = end;

					options.num_ranges++;
					pair = strtok(NULL, ",");
				}
				break;
			case 'd':
				options.debug = 1;
				break;
			}
			break;
		default:
			strcpy(infile, argv[n]);
			break;
		}
	}

	fp = fopen(infile, "rb");
	if(fp == NULL)
	{
		printf("Error: Could not open file %s\n", infile);
		exit(1);
	}

	fread(&FontFileHeader.id0, 1, 1, fp);
	if (FontFileHeader.id0 != 0xFF && FontFileHeader.id0 != 0x7F)
	{
		printf("Error: Unsupported file type\n");
		exit(1);
	}
	fread(&FontFileHeader.id, 1, 7, fp);
	fread(&FontFileHeader.reserved, 1, 8, fp);
	fread(&FontFileHeader.pnum, 2, 1, fp);
	fread(&FontFileHeader.ptyp, 1, 1, fp);
	fread(&FontFileHeader.fih_offset, 4, 1, fp);

	if(options.debug)
		printf("== FontFileHeader ==\n0x%X\n%.*s\n%i\n%i\n0x%X\n\n", FontFileHeader.id0, 7, FontFileHeader.id, FontFileHeader.pnum, FontFileHeader.ptyp, FontFileHeader.fih_offset);
	
	if (FontFileHeader.id0 == 0x7F)
	{
		fread(&DRDOSExtendedFontFileHeader.num_fonts_per_codepage, 1, 1, fp);
		DRDOSExtendedFontFileHeader.font_cellsize = (char *)malloc(sizeof(char) * DRDOSExtendedFontFileHeader.num_fonts_per_codepage);
		DRDOSExtendedFontFileHeader.dfd_offset = (long *)malloc(sizeof(long) * DRDOSExtendedFontFileHeader.num_fonts_per_codepage);
		for (int i = 0; i < DRDOSExtendedFontFileHeader.num_fonts_per_codepage; ++i)
		{
			fread(&DRDOSExtendedFontFileHeader.font_cellsize[i], 1, 1, fp);
		}
		for (int i = 0; i < DRDOSExtendedFontFileHeader.num_fonts_per_codepage; ++i)
		{
			fread(&DRDOSExtendedFontFileHeader.dfd_offset[i], 4, 1, fp);
		}
		
		if (options.debug)
		{
			printf("== DRDOSExtendedFontFileHeader ==\n");
			printf("Fonts: %i\n", DRDOSExtendedFontFileHeader.num_fonts_per_codepage);
			for (int i = 0; i < DRDOSExtendedFontFileHeader.num_fonts_per_codepage; ++i)
			{
				printf("Size: %i\nOffset: 0x%X\n", DRDOSExtendedFontFileHeader.font_cellsize[i], DRDOSExtendedFontFileHeader.dfd_offset[i]);
			}
			printf("\n");
		}
	}

	fseek(fp, FontFileHeader.fih_offset, SEEK_SET);
	fread(&FontInfoHeader.num_codepages, 2, 1, fp);

	if(options.debug)
		printf("== FontInfoHeader ==\n%i\n\n", FontInfoHeader.num_codepages);

	if(!options.debug && !options.binary)
		remove(outfile);

	for (int cp = 0; cp < FontInfoHeader.num_codepages; ++cp)
	{
		long cpeh_start = ftell(fp); // Store CodePageEntryHeader start for FONT.NT files

		fread(&CodePageEntryHeader.cpeh_size, 2, 1, fp);
		fread(&CodePageEntryHeader.next_cpeh_offset, 4, 1, fp);
		fread(&CodePageEntryHeader.device_type, 2, 1, fp);
		fread(&CodePageEntryHeader.device_name, 1, 8, fp);
		fread(&CodePageEntryHeader.codepage, 2, 1, fp);
		fread(&CodePageEntryHeader.reserved, 1, 6, fp);
		fread(&CodePageEntryHeader.cpih_offset, 4, 1, fp);

		if (CodePageEntryHeader.device_type == 2)
		{
			printf("Printer font, skipping...\n\n");
			continue;
		}

		if (options.codepage && options.codepage != CodePageEntryHeader.codepage)
		{
			fseek(fp, CodePageEntryHeader.next_cpeh_offset, SEEK_SET);
			continue;
		}

		if(options.debug)
			printf("== CodePageEntryHeader ==\n0x%X\n%i\n%.*s\n%i\n\n", CodePageEntryHeader.cpeh_size, CodePageEntryHeader.device_type, 8, CodePageEntryHeader.device_name, CodePageEntryHeader.codepage);
		else
			printf("Code Page: %i\n", CodePageEntryHeader.codepage);

		fread(&CodePageInfoHeader.version, 2, 1, fp);
		fread(&CodePageInfoHeader.num_fonts, 2, 1, fp);
		fread(&CodePageInfoHeader.size, 2, 1, fp);

		if(options.debug)
			printf("== CodePageInfoHeader ==\n%i\n%i\n0x%X\n\n", CodePageInfoHeader.version, CodePageInfoHeader.num_fonts, CodePageInfoHeader.size);

		for (int font = 0; font < CodePageInfoHeader.num_fonts; ++font)
		{
			unsigned char *data;
			FILE *out;

			fread(&ScreenFontHeader.height, 1, 1, fp);
			fread(&ScreenFontHeader.width, 1, 1, fp);
			fread(&ScreenFontHeader.yaspect, 1, 1, fp);
			fread(&ScreenFontHeader.xaspect, 1, 1, fp);
			fread(&ScreenFontHeader.num_chars, 2, 1, fp);

			if(options.debug)
				printf("== ScreenFontHeader ==\n%i\n%i\n%i\n", ScreenFontHeader.height, ScreenFontHeader.width, ScreenFontHeader.num_chars);
			else
				printf("%ix%i\t%i characters\n", ScreenFontHeader.width, ScreenFontHeader.height, ScreenFontHeader.num_chars);

			if (FontFileHeader.id0 == 0x7F) continue;

			long offset = ScreenFontHeader.num_chars * ScreenFontHeader.height;
			if(options.debug)
				printf("Bitmap length: 0x%X\n", offset);

			if(options.num_ranges == 0)
			{
				options.range[0][0] = 0;
				options.range[0][1] = ScreenFontHeader.num_chars - 1;

				options.num_ranges = 1;
			}

			if (!options.info)
			{
				data = (unsigned char *)malloc(sizeof(unsigned char) * offset);
				fread(data, 1, offset, fp);
				if (options.binary)
				{
					sprintf(outfile, "CP%i_%ix%i__1bpp.bin", CodePageEntryHeader.codepage, ScreenFontHeader.width, ScreenFontHeader.height);
					out = fopen(outfile, "wb");
					if (out == NULL)
					{
						printf("Error: Could not open output file %s\n", outfile);
						exit(1);
					}
					for (int num = 0; num < options.num_ranges; ++num)
					{
						for (int r = options.range[num][0]; r < (options.range[num][1] + 1); ++r)
						{
							int start = r * ScreenFontHeader.height;
							int end = start + ScreenFontHeader.height;
							for (int i = start; i < end; ++i)
								fwrite(&data[i], 1, 1, out);
						}
					}
					fclose(out);
				}
				else
				{
					out = fopen(outfile, "a");
					if (out == NULL)
					{
						printf("Error: Could not open output file %s\n", outfile);
						exit(1);
					}
					int count = 0;
					for (int num = 0; num < options.num_ranges; ++num)
					{
						count += (options.range[num][1] + 1) - options.range[num][0];
					}
					int bytes = (ScreenFontHeader.height * count);
					fprintf(out, "const unsigned char CP%i_%ix%i__1bpp[%i] = {\n", CodePageEntryHeader.codepage, ScreenFontHeader.width, ScreenFontHeader.height, bytes);
					for (int num = 0; num < options.num_ranges; ++num)
					{
						int col = 0;
						for (int r = options.range[num][0]; r < (options.range[num][1] + 1); ++r)
						{
							int start = r * ScreenFontHeader.height;
							int end = start + ScreenFontHeader.height;
							for (int i = start; i < end; ++i)
							{
								if (r == options.range[num][1] && i == (end - 1) && num == (options.num_ranges - 1))
									fprintf(out, "0x%02X};\n", data[i]);
								else
									fprintf(out, "0x%02X,", data[i]);
								if (++col == ScreenFontHeader.height)
								{
									fprintf(out, "\n");
									col = 0;
								}
							}
						}
					}
					fclose(out);
				}
				free(data);
			}
			else
				fseek(fp, offset, SEEK_CUR);
		}
		printf("\n");

		if (FontFileHeader.id0 == 0x7F && !options.info)
		{
			unsigned char buf[32];
			FILE *out;

			if (options.num_ranges == 0)
			{
				options.range[0][0] = 0;
				options.range[0][1] = 255;

				options.num_ranges = 1;
			}

			fread(&CharacterIndexTable.FontIndex, 2, 256, fp);

			if (options.binary)
			{
				for (int num_fonts = 0; num_fonts < DRDOSExtendedFontFileHeader.num_fonts_per_codepage; ++num_fonts)
				{
					sprintf(outfile, "CP%i_8x%i__1bpp", CodePageEntryHeader.codepage, DRDOSExtendedFontFileHeader.font_cellsize[num_fonts]);
					out = fopen(outfile, "wb");
					if (out == NULL)
					{
						printf("Error: Could not open output file %s\n", outfile);
						exit(1);
					}
					for (int num = 0; num < options.num_ranges; ++num)
					{
						for (int i = options.range[num][0]; i < (options.range[num][1] + 1); ++i)
						{
							long bitmap_offset = (CharacterIndexTable.FontIndex[i] * DRDOSExtendedFontFileHeader.font_cellsize[num_fonts]) + DRDOSExtendedFontFileHeader.dfd_offset[num_fonts];
							fseek(fp, bitmap_offset, SEEK_SET);
							fread(&buf, 1, DRDOSExtendedFontFileHeader.font_cellsize[num_fonts], fp);
							for (int height = 0; height < DRDOSExtendedFontFileHeader.font_cellsize[num_fonts]; ++height)
								fwrite(&buf[height], 1, 1, out);
						}
					}
					fclose(out);
				}
			}
			else
			{
				out = fopen(outfile, "a");
				if (out == NULL)
				{
					printf("Error: Could not open output file %s\n", outfile);
					exit(1);
				}
				for (int num_fonts = 0; num_fonts < DRDOSExtendedFontFileHeader.num_fonts_per_codepage; ++num_fonts)
				{
					int count = 0;
					for (int num = 0; num < options.num_ranges; ++num)
					{
						count += (options.range[num][1] + 1) - options.range[num][0];
					}
					int bytes = (DRDOSExtendedFontFileHeader.font_cellsize[num_fonts] * count);
					fprintf(out, "const unsigned char CP%i_8x%i__1bpp[%i] = {\n", CodePageEntryHeader.codepage, DRDOSExtendedFontFileHeader.font_cellsize[num_fonts], bytes);
					for (int num = 0; num < options.num_ranges; ++num)
					{
						for (int i = options.range[num][0]; i < (options.range[num][1] + 1); ++i)
						{
							long bitmap_offset = (CharacterIndexTable.FontIndex[i] * DRDOSExtendedFontFileHeader.font_cellsize[num_fonts]) + DRDOSExtendedFontFileHeader.dfd_offset[num_fonts];
							fseek(fp, bitmap_offset, SEEK_SET);
							fread(&buf, 1, DRDOSExtendedFontFileHeader.font_cellsize[num_fonts], fp);
							for (int height = 0; height < DRDOSExtendedFontFileHeader.font_cellsize[num_fonts]; ++height)
							{
								fprintf(out, "0x%02X", buf[height]);
								if (height < (DRDOSExtendedFontFileHeader.font_cellsize[num_fonts] - 1))
									fprintf(out, ",");
							}
							if (i == options.range[num][1] && num == (options.num_ranges - 1))
								fprintf(out, "};\n\n");
							else
								fprintf(out, ",\n");
						}
					}
				}
				fclose(out);
			}
		}

		if (strncmp(FontFileHeader.id, "FONT.NT", 7) == 0)
		{
			fseek(fp, cpeh_start, SEEK_SET);
			fseek(fp, CodePageEntryHeader.next_cpeh_offset, SEEK_CUR);
		}
		else
			fseek(fp, CodePageEntryHeader.next_cpeh_offset, SEEK_SET);
	}

	fclose(fp);

	return 0;
}