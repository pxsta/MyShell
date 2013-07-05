#if !defined _TOKENIZER_H_
#define _TOKENIZER_H_


#include <stdio.h>
#define MAX_WORD_LENGTH 256

typedef struct token {
	int kind;
	char raw_value[MAX_WORD_LENGTH];
} token;


typedef enum token_type {
	token_type_eol,
	token_type_eof,
	token_type_word,
	token_type_left_brackets_operator,
	token_type_right_brackets_operator,
	token_type_pipe_operator,
	token_type_ampersand,
	token_type_redirect_right_operator,
	token_type_redirect_left_operator
} token_type;

static token const token_eol        =  {token_type_eol,       {0}};
static token const token_eof        =  {token_type_eof,       {0}};
static token const token_left_brackets_operator   =  {token_type_left_brackets_operator,       {'('}};
static token const token_right_brackets_operator   =  {token_type_right_brackets_operator,       {')'}};

static token const token_pipe_operetor = {token_type_pipe_operator,{'|'}};
static token const token_ampersand ={token_type_ampersand,{'&'}};
static token const token_redirect_right_operator ={token_type_redirect_right_operator,{'>'}};
static token const token_redirect_left_operator ={token_type_redirect_left_operator,{'<'}};

void init_tokenizer(FILE *fp);
token get_token();
void dump_token(token* t);

#endif
