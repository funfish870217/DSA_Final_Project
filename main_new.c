#include "api.h"
#include "keys.code.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int n_mails;
int n_queries;
mail *mails;
query *queries;

typedef struct MailSite{
	int key;
    struct MailSite* next; //連接下一個token
}MailSite;

typedef struct TokenHead{
	struct Token* head; //連接第一個token
	struct Token* tail; //連接最後一個token
}TokenHead;

typedef struct Token{
    char *token; //最常的英文單字應該也只有45個字？
    int len; // token的長度 modify
	struct Token* next; //連接下一個token
	struct MailSite* first; //連到第一個有該token的mail
    struct MailSite* tail; //連到最後一個token的mail
}Token;

typedef struct MailSet{
	char* tokenSet[1000]; //該封信所有token
    int token_len[1000]; //各個token的長度
	int tokenSet_size; //tokenSet總大小
}MailSet;

MailSet addSet(MailSet mailSet, char *word, int len, int size){
    mailSet.tokenSet[size] = word;
    mailSet.token_len[size] = len;
    return mailSet;
}

MailSite *addSite(int key){
    MailSite *newsite = (MailSite*)malloc(sizeof(MailSite));
    newsite->key = key;
    newsite->next = NULL;
    return newsite;
}

Token *addToken(char *word, int key, int len){
    Token *newtoken = (Token*)malloc(sizeof(Token));
    newtoken->next = NULL;
    newtoken->token = word;
    newtoken->len = len;
    MailSite *site = addSite(key);
    newtoken->first = site; //第幾封信有這個token
    newtoken->tail = site;
    return newtoken;
}

TokenHead *NewHead(char *word, int key, int len){
    TokenHead *newhead = (TokenHead*)malloc(sizeof(TokenHead));
    newhead->head = addToken(word, key, len);
    newhead->tail = newhead->head;
    return newhead;
}


int hash(const char *key, int* len_ptr){
    int i, f1 = 0, f2 = 0;
    for (i = 0; key[i] != '\0' && i < NS; i++) {
        //調整大小寫
        //printf("ascII = %d\n", key[i]);
        if(key[i] <= 90 && key[i] >= 65){
            int Key = tolower(key[i]);
            f1 += S1[i] * Key;
            f2 += S2[i] * Key;
        }
        else{
            f1 += S1[i] * key[i];
            f2 += S2[i] * key[i];
        }
        f1 %= NG;
        f2 %= NG;
        *len_ptr += 1;
    }
    i = (G[f1] + G[f2]) % NG;    
    if (i < NK && strcasecmp(key, K[i]) == 0)
        return i;
    return -1;
}


void tokenize(mail **mails, int n_mails, TokenHead **tokenhead, MailSet mailSet[]){
    for(int i = 0; i < n_mails ; i++){
        mail *m = &(*mails)[i];
        //printf("%d\n", strlen(m->content));
        char delimit[]=" ,.-':;?()+*/%$#!\"@^&][~{}|<>=_`";
        char *string[1000]; //存放content
        char *string2[256]; //存放subject
        int idx = 0, j = 0, idx2 = 0;
        int size = 0; //tokenSet大小
        string[idx] = strtok(m->content, delimit);    

        //content
        while(string[idx] != NULL){
            int h = 0;
            int len = 0; //token長度
            h = hash(string[idx], &len);
            // printf("%d ", h);
            if(tokenhead[h] == NULL){ //目前hash_table這格是空的
                tokenhead[h] = NewHead(string[idx], i, len);
                mailSet[i] = addSet(mailSet[i], string[idx], len, size);
                size += 1;
            }
            
            else{
                //printf("collision\n");
                Token *curr = tokenhead[h]->head;
                //printf("目前在位的單詞 %s\n", curr->token);
 
                int is_diff = 0;
                is_diff = strncmp(curr->token, string[idx], len);
                
                if(is_diff == 0){
                    //printf("雖然collision但是檢查到同一個單詞: ");
					if(i != curr->tail->key){
                        curr->tail->next = addSite(i);
                        curr->tail = curr->tail->next;
                        mailSet[i] = addSet(mailSet[i], string[idx], len, size);
                        size += 1;
                    }
                    
                    idx++;
                    string[idx] = strtok(NULL, delimit);
                    continue;
                }
            }
            // printf("string [%d] = %s\n", idx, string[idx]); 
            idx++;
            string[idx] = strtok(NULL, delimit);
        }


        //subject
        string2[idx2] = strtok(m->subject, delimit);
        
        while(string2[idx2] != NULL){
            int h = 0;
            int len = 0; //token長度
            h = hash(string2[idx2], &len);
            // printf("%d ", h);
            if(tokenhead[h] == NULL){ //目前hash_table這格是空的
                tokenhead[h] = NewHead(string2[idx2], i, len);
                mailSet[i] = addSet(mailSet[i], string2[idx2], len, size);
                size += 1;
            }
            
            else{
                //printf("collision\n");
                Token *curr = tokenhead[h]->head;
                //printf("目前在位的單詞 %s\n", curr->token);
 
                int is_diff = 0;
                is_diff = strncmp(curr->token, string2[idx2], len);
                
                if(is_diff == 0){
                    //printf("雖然collision但是檢查到同一個單詞: ");
					if(i != curr->tail->key){
                        curr->tail->next = addSite(i);
                        curr->tail = curr->tail->next;
                        mailSet[i] = addSet(mailSet[i], string2[idx2], len, size);
                        size += 1;
                    }
                    
                    idx2++;
                    string2[idx2] = strtok(NULL, delimit);
                    continue;
                }
            }
            // printf("string2 [%d] = %s\n", idx2, string2[idx2]); 
            idx2++;
            string2[idx2] = strtok(NULL, delimit);
        }
        mailSet[i].tokenSet_size = size; //update size
    } 
}

int Find_Similar(int mid, float threshold, TokenHead** tokenhead, MailSet mailSet[], int* answer, int n_mails){
    int hit[10000];
    // float similarity[10000];
    int idx = 0;

    for (int i = 0; i < mailSet[mid].tokenSet_size; i++){  //mid的所有token跑過一遍
        int h = 0;
        int len = 0;
        char* element = mailSet[mid].tokenSet[i];  //tokenSet[mid]的集合元素
        h = hash(element, &len);
        Token *curr = tokenhead[h]->head;
        int is_diff = 0;
        is_diff = strncmp(curr->token, element, len);

        if(is_diff == 0){
            MailSite* tmp = curr->first;
            while(tmp != NULL){
                if (tmp->key != mid) //排除自己
                    hit[tmp->key] += 1;
                tmp = tmp->next;
            }
            continue;
        }

        while(is_diff != 0){
            curr = curr->next;
            is_diff = strncmp(curr->token, element, len);
        }

        MailSite* tmp = curr->first;
        while(tmp != NULL){
            if (tmp->key != mid) //排除自己
                hit[tmp->key] += 1;
            tmp = tmp->next;
        }
    }

    for(int i = 0; i < n_mails; i++){  //輸出答案
        float similarity = (float)(hit[i]) / (float)(mailSet[i].tokenSet_size + mailSet[mid].tokenSet_size - hit[i]);
        if (similarity > threshold){
            answer[idx] = i;
            idx ++;
        }
    }
    return idx;
}

int main(void){
	api.init(&n_mails, &n_queries, &mails, &queries);
	//前置作業1. tokenize
    TokenHead **tokenhead = calloc(NK, sizeof(TokenHead*));
    MailSet mailSet[100];
    tokenize(&mails, n_mails, tokenhead, mailSet);

    for(int i = 0; i < n_queries; i++){
		if(queries[i].type == find_similar)
		{
			int mid = queries[i].data.find_similar_data.mid;
            float threshold = queries[i].data.find_similar_data.threshold;

            int* answer = calloc(10000, sizeof(int));
            int answer_len = Find_Similar(mid, threshold, tokenhead, mailSet, answer, n_mails);
			api.answer(queries[i].id, answer, answer_len);
            free(answer);

            // printf("mid len : %d\n", mailSet[mid].tokenSet_size);
            // printf("other len : %d\n", mailSet[1].tokenSet_size);
		}
	}

    // int num = 1;
    // for(int i = 0; i < 999983; i++){
    //     if(tokenhead[i] != NULL){
    //         printf("%d ", i);
    //         Token *curr = tokenhead[i]->head;
    //         printf("num = %d ", num);
    //         num++;
    //         //printf("%d %d\n", num, i);
    //         printf("%s\n", curr->token);
    //         while (curr->next != NULL){
    //              curr = curr->next;
    //              printf("%s\n", curr->token);
    //              num++;
    //         }
    //     }
    // }
	//printf("%s ", queries[1]);
	// for(int i = 0; i < n_queries; i++)
	// 	if(queries[i].type == expression_match){
	// 		char to_prefix[2048];
	// 		Expr *expression_list[109973];
	// 		//Expr *expression = expression_list;
	// 		token_exist(&queries, i, expression_list, to_prefix);
	// 		//printf("%s\n", expression_list[7970]);
	//    		//api.answer(queries[i].id, NULL, 0);
	// 	}

  return 0;
}
