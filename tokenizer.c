#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"
#include "debug.h"

static int get_character(void);
static void unget_character(int c);
static token get_word_token(int c);

//単語の区切り文字
static const char separator_chars[] = "\n;\t\f\r ()|&><";
static FILE *in_file;
char isInQuotation;

void init_tokenizer(FILE *fp)
{
    in_file = fp;
    isInQuotation = 0;
}

token get_token()
{
    int c;

    for (;;)
    {
        c = get_character();
        if (ferror(stdin) != 0)
        {
            fprintf(stderr,"input from stdin error\n");
            clearerr(stdin);
        }
        switch (c)
        {
        case '\n':
        case ';':
            return token_eol;
        case -1:
            return token_eof;
        case '\t':
        case '\f':
        case '\r':
        case ' ':
            break;

        case '(':
            return token_left_brackets_operator;
        case ')':
            return token_right_brackets_operator;

        case '|':
            return token_pipe_operetor;
        case '&':
            return token_ampersand;
        case '>':
            return token_redirect_right_operator;
        case '<':
            return token_redirect_left_operator;

            //コマンドなどの文字列など
        default:
            if (isprint(c))
            {
                return get_word_token(c);
            }
            else
            {
                p_warn(5, "Cannot use that char:%d\n", c);
                //exit(1);
            }
        }
    }
}

static int get_character(void)
{
    int c;
    c = getc(in_file);
    c = ((c == EOF) ? (-1) : (c));
    return c;
}

static void unget_character(int c)
{
    if (c == EOF){
        return;
    }

    ungetc(c, in_file);
}

static token get_word_token(int c)
{
    token newtoken;
    char buf[MAX_WORD_LENGTH + 1];
    {
        int i = 0;
        do
        {
            //引用符で囲まれているかどうか
            if (c == '"')
            {
                isInQuotation = (isInQuotation == 0);
            }
            else
            {
                buf[i++] = c;
            }

            if (i >= MAX_WORD_LENGTH)
            {
                fprintf(stderr, "Error : input word is too long");
                exit(1);
            }
            c = get_character();
        }
        while (isprint(c) && (isInQuotation || (strchr(separator_chars, c) == NULL)) );

        buf[i] = '\0';
        unget_character(c);
    }

    newtoken.kind = token_type_word;
    strcpy(newtoken.raw_value, buf);
    return newtoken;
}

void dump_token(token *t)
{
    if (t->kind == token_type_word)
    {
        printf("kind:%d raw.value:%s\n", t->kind, t->raw_value);
    }
    else
    {
        printf("kind:%d raw.value:%s\n", t->kind, t->raw_value);
    }
}
