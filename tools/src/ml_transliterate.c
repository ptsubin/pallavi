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

	ch = (unsigned char*)src;
	
	while ((*ch != '\0') && (count <= size)) {
		if (*ch == 0x20) {
			*dest = *ch;
		} else if (!(*ch ^ 0xe0)) { /* 3 byte sequence */
			hex = *ch & 0x0f;
			ch++;
			hex = (hex << 6) | (*ch & 0x3f);
			ch++;
			hex = (hex << 6) | (*ch & 0x3f);
			*dest = hex;
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

/**
 * print_help() shows a help message */
void print_help(char *argv[], char opt)
{
	switch (opt) {
		case 0:
		case 'h': printf("\n\tUsage: %s <infile>\n\n", argv[0]);
				break;
		default: printf("\n\tInvalid option %c", opt);
			    printf("\n\tUse %s -h for help\n\n", argv[0]);
	}

	printf("\t%s takes an input file in UTF8 encoding and generates", argv[0]);
	printf("\n\ta .trans file with the UTF8 characters replaced with phonems");
	printf("\n\tas per the Malayalam characters-phonems mapping. This");
	printf("\n\tprogram will not work for languages other than Malayalam");
	printf("\n\tand file encodings other than UTF8.\n\n");
}

int main(int argc, char *argv[])
{
	char buf[1024];
	uint16_t hexchars[1024];
	FILE *ifp = NULL, *ofp = NULL;
	int nos = 0, i;
	char *nl, *outfile = NULL;

	if (argc != 2) {
		print_help(argv, 0);
		return -1;
	}

	if (argv[1][0] == '-') {
		print_help(argv, argv[1][1]);
		return 0;
	}

	strncpy(buf, argv[1], sizeof(buf));
	nl = strchr(buf, '.');
	if ((nl != NULL) && (*nl == '.'))
		*nl = '\0';
	strncat(buf, ".trans", sizeof(buf) - strlen(argv[1]));
	outfile = strdup(buf);
	if (outfile == NULL) {
		printf("Memory allocation error\n");
		return -2;
	}

	ifp = fopen(argv[1], "r");
	if (ifp == NULL) {
		perror(argv[1]);
		goto cleanup;
	}

	ofp = fopen(outfile, "w");
	if (ofp == NULL) {
		perror(outfile);
		goto cleanup;
	}

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
	
cleanup:
	if (ifp)
		fclose(ifp);
	if (ofp)
		fclose(ofp);
	if (outfile)
		free(outfile);

	return 0;
}

