#if !defined _PARSER_H
#define _PARSER_H

#include "list.h"
#include "job.h"

typedef struct parse_result_node
{
	void* value;
	int accepted;
}parse_result_node;

job* parse(list_node *t_list);
#endif
