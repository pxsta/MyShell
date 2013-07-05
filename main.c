#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "list.h"
#include "parser.h"
#include "job.h"
#include "debug.h"


//字句解析結果のリストを表示する
void dump_token_list_data(list_node *token_list_head){
    list_node *currentNode = token_list_head->next;
    while (currentNode)
    {
        dump_token((token *)(currentNode->data));
        currentNode = currentNode->next;
    }
}

int main()
{
    list_node *token_list;
    token current_token;
    job *job_list;

    init_tokenizer(stdin);
    init_shell();

    while (1)
    {
        fprintf(stdout, "myshell>");
        token_list = create_list(sizeof(token));
        //字句解析を行なってリストに追加する
        while ((current_token = get_token()).kind != token_type_eol && current_token.kind != token_type_eof)
        {
            add_list(token_list, &current_token);
        }

        request_job_notification ();
        if (token_list->next == NULL)
        {
            p_debug("continue");
            continue;
        }

        //字句解析結果を表示
        //dump_token_list_data(token_list);

        //構文解析を行う
        job_list = parse(token_list);

        //実行
        launch_job(job_list);

        free_list(token_list);
    }
}
