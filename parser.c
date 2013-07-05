#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include "tokenizer.h"
#include "list.h"

#include "parser.h"
#include "tokenizer.h"
#include "list.h"
#include "job.h"

list_node *token_list_head;
list_node *token_list;
token *current_token;
job *root_job;
job *current_job;
process *current_process;
int token_list_index = 0;
int argcount = 0;

void create_new_process();
//パイプが来たら新しいプロセスを作る
void pipe_process();

//<command> ::=  <simple_command> | <pipeline_command>
job *command();

//<pipeline_command> ::= <<simple_command> '|' <simple_command>
job *pipeline_command();
job *pipeline_command_cycle();

//<simple_command> ::= <simple_command_element><simple_command_cycle>
//<simple_command_cycle> ::= <simple_command_element><simple_command_cycle> | ?
job *simple_command();
job *simple_command_cycle();

//<simple_command_element> ::= <word> | <redirection>
job *simple_command_element();
//<redirection> ::=  '>' <word> | '<' <word>
job *redirection();
//<word> ::= <letter> | <digit> | <word> <letter> | <word> <digit> | <word> '_'
parse_result_node *word();
//<letter> ::= a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z| A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z

//<digit> ::= 0|1|2|3|4|5|6|7|8|9


#if VS_DEBUG
int open(const char *param, int flag, int mode){
    return 10;
}
#endif



void read_next()
{
    //1つ内部的に進める
    token_list = (list_node *)get_next(token_list);
    current_token = (token *)elementAt(token_list, 0);
    token_list_index++;
}
void push_back()
{
    //1つ内部的に戻す
    token_list = (list_node *)get_prev(token_list);
    current_token = (token *)elementAt(token_list, 0);
    token_list_index--;
}


job *parse(list_node *t_list)
{
    //初期化
    token_list_head = t_list;
    token_list = t_list;
    current_token = (token *)elementAt(token_list, 0);
    //パース開始
    command();
    return root_job;
}


//<command> ::=  <simple_command>
job *command()
{
    //まずはジョブとプロセスを作る
    root_job = create_raw_job_node();
    current_job = root_job;
    root_job->first_process = create_raw_process_node();
    argcount = 0;
    current_process = root_job->first_process;

    //BNFにしたがってパース
    simple_command();
    pipeline_command();

    if (current_job != NULL && current_job->name == NULL)
    {
        current_job->name = generate_job_name(current_job);
    }
    return root_job;
}

//<pipeline_command> ::= <simple_command> '|' <simple_command><pipeline_command_cycle>
//<pipeline_command_cycle> ::= '|' <simple_command><pipeline_command_cycle>| ?
job *pipeline_command()
{
    simple_command();
    if (current_token != NULL && current_token->kind == token_type_pipe_operator)
    {
        read_next();

        //パイプがきたら次の新しいプロセスを作る
        pipe_process();

        simple_command();
        return pipeline_command_cycle();
    }
    return NULL;
}

job *pipeline_command_cycle()
{
    simple_command();
    if (current_token == NULL)
    {
        return NULL;
    }
    if (current_token->kind == token_type_pipe_operator)
    {
        read_next();

        //パイプがきたら次の新しいプロセスを作る
        pipe_process();

        simple_command();
        pipeline_command_cycle();
    }
    return NULL;
}


//<simple_command> ::= <simple_command_element><simple_command_cycle>
//<simple_command_cycle> ::= <simple_command_element><simple_command_cycle> | & <simple_command_cycle> | ?
job *simple_command()
{
    if (simple_command_element() != NULL)
    {
        return simple_command_cycle();
    }
    return NULL;
}
job *simple_command_cycle()
{
    if (current_token == NULL)
    {
        return NULL;
    }
    else if (current_token != NULL && current_token->kind == token_type_ampersand)
    {
        read_next();
        current_job->foreground = 0;

        //ジョブを新規に作る
        //TODO:やっつけ
        if (current_token != NULL && current_token ->kind == token_type_word)
        {
            current_job->name = generate_job_name(current_job);

            current_job->next = create_raw_job_node();
            current_job = current_job->next;
            argcount = 0;
            current_job->first_process = create_raw_process_node();
            current_process = current_job->first_process;
        }
        return simple_command();
    }
    else if (simple_command_element() != NULL)
    {
        return simple_command_cycle();
    }
    return NULL;
}

//<simple_command_element> ::= <word> | <redirection>
job *simple_command_element()
{
    parse_result_node *res_word;
    if (current_token != NULL && current_token->kind == token_type_word)
    {
        res_word = word();
        read_next();
        if (current_process->argv == NULL)
        {
            current_process->argv = (char **)malloc(sizeof(char *) * 5);
        }
        current_process->argv[argcount++] = (char *)res_word->value;
        current_process->argv[argcount] = NULL;
        return root_job;
    }
    else if (redirection() != NULL)
    {
        return root_job;
    }
    return NULL;
}

//<redirection> ::=  '>' <word> | '<' <word>
job *redirection()
{
    if (current_token != NULL && strcmp(current_token->raw_value, ">") == 0)
    {
        read_next();
        current_job->out = open((char *)current_token->raw_value, O_CREAT | O_WRONLY, 0666);
        read_next();
        return current_job;
    }
    else if (current_token != NULL && strcmp(current_token->raw_value, "<") == 0)
    {
        read_next();
        current_job->in = open((char *)current_token->raw_value, O_RDONLY, 0666);
        read_next();
        return current_job;
    }
    return NULL;
}

//<word> ::= <letter> | <digit> | <word> <letter> | <word> <digit> | <word> '_'
parse_result_node *word()
{
    parse_result_node *result = (parse_result_node *)calloc(1, sizeof(parse_result_node));
    result->value = &current_token->raw_value;
    result->accepted = 1;
    return result;
}

void create_new_process()
{
    current_process->next = create_raw_process_node();
    current_process = current_process->next;
    argcount = 0;
}
void pipe_process()
{
    create_new_process();
}
