/* 
 * ml_texttowordfreq.c
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
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>

/* We are not interested in word frequency. Just extract unique words and dump
 * to a file. The aim is to chain the transliterator, word list generator and 
 * dictionary generator operations */

/* Holds a single word and a pointer to the next */
struct word_node {
	char *word;
	struct word_node *next;
};

/* Holds a header for words that contain same characters */
struct wordinfo_node {
	uint32_t sum;
	uint32_t num_words;
	struct word_node *wordlist;
	struct wordinfo_node *next;
	struct wordinfo_node *prev;
};

struct wordinfo_node *wordinfo_node_alloc(void)
{
	struct wordinfo_node *info_node;

	info_node = (struct wordinfo_node*)malloc(sizeof(struct wordinfo_node));
	memset((void*)info_node, 0, sizeof(struct wordinfo_node));

	return info_node;
}

struct word_node *word_node_alloc(void)
{
	struct word_node *node;

	node = (struct word_node*)malloc(sizeof(struct word_node));
	memset((void*)node, 0, sizeof(struct word_node));

	return node;
}

uint32_t calculate_sum(char *word)
{
	uint32_t sum = 0;

	while (*word) {
		sum += *word;
		word++;
	}

	return sum;
}

/* If a word is found, returns found = 1 and the node where the word is. If the
 * sum is found, returns the node and found = 0. Else returns NULL
 */
struct wordinfo_node *search_word(char *word, struct wordinfo_node *info,
		int *found)
{
	struct wordinfo_node *winode = NULL;
	struct word_node *wnode = NULL;
	int sum = 0;

	if (word == NULL)
		return NULL;

	if (info == NULL)
		return NULL;

	*found = 0;
	sum = calculate_sum(word);
	winode = info;

	while (winode) {
		if (winode->sum == sum)
			break;
		winode = winode->next;
	}

	if (winode == NULL)
		return NULL;

	/* Unlikely */
	if (winode->wordlist == NULL)
		return NULL;

	wnode = winode->wordlist;
	while (wnode) {
		if (!strcmp(wnode->word, word)) {
			*found = 1;
			break;
		}
		wnode = wnode->next;
	}

	return winode;
}

int wordlist_insert(char *word, struct wordinfo_node *wordinfo)
{
	struct wordinfo_node *info_node = NULL, *tinfo_node = NULL;
	struct word_node *node = NULL, *tnode = NULL;
	uint32_t sum = 0;
	int present = 0;

	if (wordinfo == NULL)
		return -EFAULT;

	if (word == NULL)
		return -EINVAL;

	sum = calculate_sum(word);
	if (sum == 0)
		return -EINVAL;

	node = word_node_alloc();
	if (node == NULL) {
		return -ENOMEM;
	}
	node->word = strdup(word);
	node->next = NULL;

	if (wordinfo->wordlist == NULL) {
		wordinfo->wordlist = node;
		wordinfo->num_words = 1;
		wordinfo->sum = sum;
		return 0;
	}

	info_node = search_word(word, wordinfo, &present);
	if (info_node != NULL) {
		if (present)
			return -EEXIST;
		else {
			tnode = info_node->wordlist;
			while (tnode->next)
				tnode = tnode->next;
			tnode->next = node;
			info_node->num_words += 1;
			return 0;
		}
	}

	tinfo_node = wordinfo;
	while (tinfo_node->next)
		tinfo_node = tinfo_node->next;

	info_node = wordinfo_node_alloc();
	if (info_node == NULL){
		free(node->word);
		free(node);
		return -ENOMEM;
	}

	info_node->wordlist = node;
	info_node->sum = sum;
	info_node->num_words = 1;
	info_node->next = NULL;
	info_node->prev = tinfo_node;
	tinfo_node->next = info_node;

	return 0;
}

void wordlist_dump(struct wordinfo_node *wordinfo, FILE *fp)
{
	struct word_node *wnode = NULL;

	if (wordinfo == NULL || fp == NULL)
		return;

	while (wordinfo) {
		wnode = wordinfo->wordlist;
		while (wnode) {
			if (wnode->word)
				fprintf(fp, "%s\n", wnode->word);
			wnode = wnode->next;
		}
		wordinfo = wordinfo->next;
	}
}

void wordlist_free(struct wordinfo_node *wordinfo)
{
	struct word_node *wnode = NULL, *twnode = NULL;
	struct wordinfo_node *winode = NULL;

	if (wordinfo == NULL)
		return;

	while (wordinfo) {
		winode = wordinfo;
		wnode = wordinfo->wordlist;
		while (wnode) {
			twnode = wnode;
			if (wnode->word)
				free(wnode->word);
			wnode = wnode->next;
			free(twnode);
		}
		wordinfo = wordinfo->next;
		free(winode);
	}
}

/* Put anything to be filtered out in the delimiter string.
 * Convert uppercase to lowercase
 */
void process_token(char *token, struct wordinfo_node *info)
{
	char *ptr = token;

	while (ptr && *ptr) {
		if (isupper(*ptr))
			*ptr = tolower(*ptr);
		ptr++;
	}

	wordlist_insert(token, info);
}

int main(int argc, char *argv[])
{
	FILE *ifp = NULL, *ofp = NULL;
	char *buf = NULL, *tok = NULL;
	char buffer[4096];
	char delim[] = " (){}[]\t\n,./\\;:\"\'~!@#$%^&*-_`+=<>?0123456789";
	struct wordinfo_node *info_head = NULL;

	/* Make sure the arguments are correct */
	if (argc != 3) {
		printf("\n\tUsage: %s <in file> <out file>\n\n", argv[0]);
		return -1;
	}

	ifp = fopen(argv[1], "r");
	if (ifp == NULL) {
		perror(argv[1]);
		return -1;
	}

	ofp = fopen(argv[2], "w+");
	if (ofp == NULL) {
		perror(argv[2]);
		fclose(ifp);
		return -1;
	}

	info_head = wordinfo_node_alloc();
	if (info_head == NULL)
		goto cleanup;

	while(fgets(buffer, sizeof(buffer), ifp) != 0) {
		/* Skip empty lines */
		if (buffer[0] == '\n')
			continue;
		/* Remove the trailing new line */
		buffer[strlen(buffer)-1] = '\0';
		buf = buffer;
		tok = strtok(buf, delim);
		if (tok == NULL)
			continue;
		process_token(tok, info_head);
		do {
			tok = strtok(NULL, delim);
			if (tok)
				process_token(tok, info_head);
		} while (tok);
	}

	wordlist_dump(info_head, ofp);

cleanup:
	wordlist_free(info_head);
	fclose(ifp);
	fclose(ofp);

	return 0;
}
