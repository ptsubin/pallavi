/* 
 * ml_transliterate.c
 *
 * Copyright 2014 Pallavi Project <pallavi.malayalam@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <inttypes.h>

#include <ml_transliterate.h>

/**
 * utf8_to_uint16() converts a UTF8 encoded character to its corresponding 16bit
 * unsigned integer hexadecimal value. This function processes only standard 
 * ASCII characters and characters in 0x00000800 - 0x0000FFFF. The encodings are
 * 0x00000000 - 0x0000007F:	0xxxxxxx
 * 0x00000800 - 0x0000FFFF:	1110xxxx 10xxxxxx 10xxxxxx
 * If a character that is not in the mentioned range is encountered, it will be
 * skipped and the conversion would continue untill either a '\0' is seen in the
 * source string or till 'size' amount of conversions are complete. Number of
 * conversions performed will be returned and number of skipped characters are
 * not included in this.
 */
int utf8_to_uint16(const char *src, uint16_t *dest, int size)
{
	uint16_t hex = 0;
	int count = 0;
	unsigned char *ch;

	/* We take all symbols and non malayalam characters and translate them in
	 * to one space. Ignore the rest of the sequence if more than one character
	 * To be decided on what to do with the numbers */
	int seq_flag = 0;

	ch = (unsigned char*)src;

	while ((*ch != '\0') && (count <= size)) {
		if ((*ch < 128U) && (!seq_flag)) {
			*dest = ' ';
			seq_flag = 1;
		} else if (!(*ch ^ 0xe0)) { /* 3 byte sequence */
			hex = *ch & 0x0f;
			ch++;
			hex = (hex << 6) | (*ch & 0x3f);
			ch++;
			hex = (hex << 6) | (*ch & 0x3f);
			*dest = hex;
			seq_flag = 0;
		} else {
			ch++;
			continue;
		}

		count++;
		ch++;
		dest++;
	}

	return count;
}

void transliterate_one_fp(FILE *ifp, FILE *ofp)
{
	char buf[1024], *nl = NULL;
	uint16_t hexchars[1024];
	int nos = 0, i;

	while (fgets(buf, sizeof(buf), ifp) != NULL) {
		nl = strchr(buf, '\n');
		if ((nl != NULL) && (*nl == '\n'))
			*nl = '\0';
		
		nos = utf8_to_uint16(buf, hexchars, 1024);
		if (nos <= 0)
		    continue;
		
		i = 0;
		while (i < nos) {
			if ((hexchars[i] >= 0xD00) && (hexchars[i] <= 0xD7F)) {
				if (char_set[hexchars[i]-0xD00].phone != NULL)
					fprintf(ofp, "%s", char_set[hexchars[i]-0xD00].phone);
			} else if (hexchars[i] == 0x20) {
				fprintf(ofp, "%c", hexchars[i]);
			}
			i++;
		}

		if (nl != NULL) {
			fprintf(ofp, "\n");
		}
	}

	fflush(ofp);
	fclose(ifp);
	fclose(ofp);
}

void print_help(char *progname)
{
	printf("\n\tUsage: %s <infile1> [infile2] .. \n\n", progname);

	printf("%s takes input files in UTF8 encoding and generates\n", progname);
	printf("a .trans file with the UTF8 characters replaced with phonems\n"
		"as per the Malayalam characters-phonems mapping. This\n"
		"program will not work for languages other than Malayalam\n"
		"and file encodings other than UTF8.\n\n"
	);
}

int main(int argc, char *argv[])
{
	FILE *ifp = NULL, *ofp = NULL;
	char *outfile = NULL, buf[128], *dot = NULL;
	int args = 1;

	if (argc < 2) {
		print_help(argv[0]);
		return -1;
	}

	while (args < argc) {
		strncpy(buf, argv[args], sizeof(buf));
		dot = strchr(buf, '.');
		if ((dot != NULL) && (*dot == '.'))
			*dot = '\0';
		strncat(buf, ".trans", sizeof(buf) - strlen(argv[args]));
		outfile = strdup(buf);
		if (outfile == NULL) {
			printf("Memory allocation error\n");
			args++;
			continue;
		}

		ifp = fopen(argv[args], "r");
		if (ifp == NULL) {
			perror(argv[args]);
			goto cleanup;
		}

		ofp = fopen(outfile, "w");
		if (ofp == NULL) {
			perror(outfile);
			fclose(ifp);
			goto cleanup;
		}

		printf("Processing input file %s. Output file is %s\n", 
				argv[args], outfile);

		transliterate_one_fp(ifp, ofp);
cleanup:
		if (outfile)
			free(outfile);
		args++;
	}

	return 0;
}

