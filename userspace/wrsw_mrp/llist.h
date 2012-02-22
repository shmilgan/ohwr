/*
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Generic functions to handle linked lists.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __WHITERABBIT_LIST_H
#define __WHITERABBIT_LIST_H

#include "stdlib.h"

typedef struct list_node {
    void *content;
    struct list_node *next;
} NODE;

inline static NODE *list_create(void *content)
{
    NODE *n = (NODE*)malloc(sizeof(NODE));
    if (n) {
        n->content = content;
        n->next = NULL;
    }
    return n;
}

inline static void list_destroy(NODE *n)
{
    free(n);
}

inline static int list_add(NODE **head, NODE *n)
{
    if (!n)
        return -1;
    n->next = *head;
    *head = n;
    return 0;
}

inline static void list_delete(NODE **head, void *content)
{
    NODE *prev, *tmp;

    if (!*head || !content)
        return;

    prev = tmp = *head;

    if (tmp->content == content) {
        *head = tmp->next;
        free(tmp);
    } else {
        for (tmp = tmp->next; tmp; prev = tmp, tmp = tmp->next){
            if (tmp->content == content) {
                prev->next = tmp->next;
                free(tmp);
            }
        }
    }
}

inline static void list_delete_all(NODE **head)
{
    NODE *node;

    for (node = *head; node; *head = node) {
        node = (*head)->next;
        free(*head);
    }
}

inline static NODE *list_find(NODE *head, void *content)
{
    for(; head && (head->content != content); head = head->next);
    return head;
}

#endif /*__WHITERABBIT_LIST_H*/
