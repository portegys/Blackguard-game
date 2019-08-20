/*
 * Functions for dealing with linked lists of goodies
 *
 * @(#) list.c	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include "rogue.h"

/*
 * detach:
 *	Takes an item out of whatever linked list it might be in
 */
void
_detach(list, item)
struct linked_list **list, *item;
{
	if (*list == item)
		*list = next(item);
	if (prev(item) != NULL)
		item->l_prev->l_next = next(item);
	if (next(item) != NULL)
		item->l_next->l_prev = prev(item);
	item->l_next = NULL;
	item->l_prev = NULL;
}

/*
 * _attach:	add an item to the head of a list
 */
void
_attach(list, item)
struct linked_list **list, *item;
{
	if (*list != NULL) 	{
		item->l_next = *list;
		(*list)->l_prev = item;
		item->l_prev = NULL;
	}
	else 	{
		item->l_next = NULL;
		item->l_prev = NULL;
	}
	*list = item;
}

/*
 * _free_list:	Throw the whole blamed thing away
 */
void
_free_list(ptr)
struct linked_list **ptr;
{
	register struct linked_list *item;

	while (*ptr != NULL) {
		item = *ptr;
		*ptr = next(item);
		discard(item);
	}
}

/*
 * discard:  free up an item
 */
void
discard(item)
struct linked_list *item;
{
	total -= 2;
	FREE(item->l_data);
	FREE(item);
}

/*
 * new_item:	get a new item with a specified size
 */
struct linked_list *
new_item(size)
int size;
{
	register struct linked_list *item;

	item = (struct linked_list *) alloc(sizeof *item);
	item->l_data = alloc(size);
	item->l_next = item->l_prev = NULL;
	return item;
}

char *
alloc(size)
int size;
{
	register char *space = ALLOC(size);

	if (space == NULL) {
#ifdef WIN32
		sprintf(prbuf,"Rogue ran out of memory.");
#else
		sprintf(prbuf,"Rogue ran out of memory (%d).",sbrk(0));
#endif
		fatal(prbuf);
	}
	total++;
	return space;
}
