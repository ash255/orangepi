#include "tlv.h"
#include <stdlib.h>

struct tlv_t* tlv_parser(uint8_t *data, int len)
{
    int idx = 0;
    struct tlv_t *node = NULL, *header = NULL, *prev = NULL;
    
    while(idx < len)
    {
        if(data[idx] == TAG_INVAILD)
            break;
        node = calloc(1, sizeof(struct tlv_t));
        node->tag = data[idx];
        node->len = data[idx+1];
        node->value = &data[idx+2];
        
        if(prev != NULL)
            prev->next = node;
        else
            header = node;
        prev = node;
        idx += (2 + node->len);
    }
    
    return header;
}

struct tlv_t* tlv_get(struct tlv_t *tlv, uint8_t tag)
{
    struct tlv_t* temp = NULL;
    
    if(tlv != NULL)
    {
        temp = tlv;
        do
        {
            if(temp->tag == tag)
            {
                return temp;
            }
            temp = temp->next;
        }while(temp != NULL);
    }
    return NULL;
}

void tlv_free(struct tlv_t* tlv)
{
    if(tlv != NULL)
    {
        struct tlv_t* temp;
        while(tlv->next != NULL)
        {
            temp = tlv->next;
            tlv->next = tlv->next->next;
            free(temp);
        }
        free(tlv);
    }
    
}