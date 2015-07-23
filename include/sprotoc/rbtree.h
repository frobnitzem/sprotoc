/*    Copyright (C) David M. Rogers, 2015
 *    
 *    David M. Rogers <predictivestatmech@gmail.com>
 *    Nonequilibrium Stat. Mech. Research Group
 *    Department of Chemistry
 *    University of South Florida
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#ifndef _RBTREE_INC
#define _RBTREE_INC
/* The cmp function operates between nodes (void *N)-s.
 * These must store L, R (void *)-s at N + coff.
 * The black (0) / red (1) bit is used at
 * the masked bit of N+boff.
 */
typedef struct {
    int (*cmp)(const void *, const void *);
    unsigned int coff, boff;
    unsigned char mask; // contains a one where red/black bit is set.
    void *nil;
} rbop_t;

void new_tree(void *N, const rbop_t *o);
void *add_node(void **N, void *A, const rbop_t *o);
void *del_node(void **N, const void *A, const rbop_t *o);
void *lookup_node(void *N, const void *A, const rbop_t *o);

// returns mask or 0
unsigned char get_mask(const void *N, const rbop_t *o);
#endif
