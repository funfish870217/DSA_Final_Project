#include "api.h"
#include "keys.code.h"
#include <stdio.h>
#include <string.h>

// The testdata only contains the first 100 mails (mail1 ~ mail100)
// and 2000 queries for you to debug.
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
    newtoken->len = len; //modify
    MailSite *site = addSite(key);
    newtoken->first = site; //第幾封信有這個token
    newtoken->tail = site;
    return newtoken;
}

TokenHead *NewHead(char *word, int key, int len){
    TokenHead *newhead = (TokenHead*)malloc(sizeof(TokenHead));
    newhead->head = addToken(word, key, len); //modify
    newhead->tail = newhead->head;
    return newhead;
}


int hash(const char *key, int* len_ptr){
    printf("單詞 = %s\n", key);
    int i, f1 = 0, f2 = 0;
    for (i = 0; key[i] != '\0' && i < NS; i++) {
        //調整大小寫
        printf("ascII = %d\n", key[i]);
        if(key[i] <= 90 || key[i] >= 65){
            f1 += S1[i] * (key[i] + 20);
            printf("%d " , f1);
            f2 += S2[i] * (key[i] + 20);
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
    printf("i = %d\n", i);
    printf("k[%d] = %s\n", i ,K[i]);
    
    if (i < NK && strcmp(key, K[i]) == 0){
        return i;
    }
    
    return -1;
}

void tokenize(mail **mails, int n_mails, TokenHead **tokenhead){  //modify
    for(int i = 0; i < n_mails ; i++){
        mail *m = &(*mails)[i];
        //printf("%d\n", strlen(m->content));
        char delimit[]=" ,.-':;?()+*/%$#!\"@^&][~{}|<>=_`";
        char *string[1000]; //存放content
        char *string2[256]; //存放subject
        
        strtok(m->subject, delimit);
        int idx = 0, j = 0, idx2 = 0;

        string[idx] = strtok(m->content, delimit);    
        //string[idx] = strtok(m->content, delimit);
        
        //content
        while(string[idx] != NULL){
            int h = 0;
            int len = 0; //token長度
            h = hash(string[idx], &len);
            printf("%d ", h);
            if(tokenhead[h] == NULL)  //目前hash_table這格是空的
                tokenhead[h] = NewHead(string[idx], i, len);
            
            else{
                //printf("collision\n");
                Token *curr = tokenhead[h]->head;
                //printf("目前在位的單詞 %s\n", curr->token);
 
                int is_diff = 0;
                printf("%s ", curr->token);
                printf("%s\n", string[idx]);
            
                is_diff = strncasecmp(curr->token, string[idx], len);
                
                if(is_diff == 0){
                    //printf("雖然collision但是檢查到同一個單詞: ");
                    idx++;
                    string[idx] = strtok(NULL, delimit);
					if(i != curr->tail->key){
                        tokenhead[h]->head->tail->next = addSite(i);
                        tokenhead[h]->head->tail = tokenhead[h]->head->tail->next;
                    }
                    continue;
                }
                
                while(is_diff != 0){
                    printf("發生collision且是不同的單詞：\n: ");
                    //如果直到最後都沒有檢查到符合者
                    if(curr->next == NULL){
                        tokenhead[h]->tail->next = addToken(string[idx], i, len);
                        tokenhead[h]->tail = tokenhead[h]->tail->next; //update tail
                        break;
                    }
                    else{
                        curr = curr->next;
                        is_diff = strncasecmp(curr->token, string[idx], len);
                        
                        if (is_diff == 0){
                            if(i != curr->tail->key){
                                curr->tail->next = addSite(i);
                                curr->tail = curr->tail->next;
                            }
                            break;
                        }          
                    }
                }   
            }
            printf("string [%d] = %s\n", idx, string[idx]); 
            //printf("content [%d] = %s\n", idx, string[idx]); 
            idx++;
            string[idx] = strtok(NULL, delimit);
        }


        //subject
        string2[idx2] = strtok(m->subject, delimit);
        while(string2[idx2] != NULL){
            // printf("來到subject");
            int h = 0;
            int len = 0; //token長度
            h = hash(string2[idx2], &len);
            printf("%d ", h);
            if(tokenhead[h] == NULL)  //目前hash_table這格是空的
                tokenhead[h] = NewHead(string2[idx2], i, len);

            else{
                //printf("collision\n");
                Token *curr = tokenhead[h]->head;
                //printf("目前在位的單詞 %s\n", curr->token);

                int is_diff = 0;
                is_diff = strncasecmp(curr->token, string2[idx2], len);

                if(is_diff == 0){
                    //printf("雖然collision但是檢查到同一個單詞: ");
                    idx2++;
                    string2[idx2] = strtok(NULL, delimit);

					if(i != curr->tail->key){
                        tokenhead[h]->head->tail->next = addSite(i);
                        tokenhead[h]->head->tail = tokenhead[h]->head->tail->next;
                    }
                    continue;
                }

                while(is_diff != 0){
                    printf("發生collision且是不同的單詞：\n: ");
                    //如果直到最後都沒有檢查到符合者
                    if(curr->next == NULL){
                        tokenhead[h]->tail->next = addToken(string2[idx2], i, len);
                        tokenhead[h]->tail = tokenhead[h]->tail->next; //update tail
                        break;
                    }
                    else{
                        curr = curr->next;
                        is_diff = strncasecmp(curr->token, string2[idx2], len);
						
                        if (is_diff == 0){
                            if(i != curr->tail->key){
                                curr->tail->next = addSite(i);
                                curr->tail = curr->tail->next;
                            }
                            break;
                        }                           
                    }
                }   
            }
            printf("subject [%d] = %s\n", idx2, string2[idx2]); 
            idx2++;
            string2[idx2] = strtok(NULL, delimit);
        }
    } 
}

int main(void){
	api.init(&n_mails, &n_queries, &mails, &queries);
	//前置作業1. tokenize
    TokenHead **tokenhead = calloc(NK, sizeof(TokenHead*));  //modify
    tokenize(&mails, n_mails, tokenhead);
    printf("結束功能");
    printf("-----------------");
    
    int num = 1;
    for(int i = 0; i < NK; i++){
        if(tokenhead[i] != NULL){
            printf("%d ", i);
            Token *curr = tokenhead[i]->head;
            printf("num = %d ", num);
            num++;
            //printf("%d %d\n", num, i);
            printf("%s\n", curr->token);
            while (curr->next != NULL){
                 curr = curr->next;
                 printf("%s\n", curr->token);
                 num++;
            }
        }
    }

  return 0;
}
