#include <editlog.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// void free_edits_t(struct edits_t* input){
//     for(int i = 0; i< input->record.count; i++){
//         free_record_t(input->record.elem + i);
//     }
//     free(input->record.elem);
//     return;
// }

// void free_record_t(struct record_t* input){
//     free(input->opcode.elem);
//     return;
// }
// void free_data_t(struct data_t* input){
//     free(input->length);
//     free(input->inodeid);
//     free(input->timestamp);
//     if(input->path != NULL) free(input->path->elem);
//     free(input->path);
//     if(input->src != NULL) free(input->src->elem);
//     free(input->src);
//     free_premission_status_t(input->permission_status);
//     return;
// }
// void free_premission_status_t(struct permission_status_t* input){
//     free(input->username.elem);
//     free(input->groupname.elem);
//     return;
// }

struct permission_status_t* parsePermission_status (xmlDocPtr doc, xmlNodePtr cur) {
	struct permission_status_t* ret = malloc(sizeof(struct permission_status_t));
    xmlChar *tmpstring;
    memset(ret, 0, sizeof(struct permission_status_t));
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"USERNAME"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->username.elem = malloc(strlen(tmpstring) + 1);
            memcpy(ret->username.elem, tmpstring, strlen(tmpstring) + 1);
            ret->username.count = strlen(tmpstring) + 1;
            // printf("username=%s\n", ret->username.elem);          
            // printf("len(username)=%d\n\n", ret->username.count);          
		    xmlFree(tmpstring);
 	    }
	    else if ((!xmlStrcmp(cur->name, (const xmlChar *)"GROUPNAME"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->groupname.elem = malloc(strlen(tmpstring) + 1);        
            memcpy(ret->groupname.elem, tmpstring, strlen(tmpstring) + 1);
            ret->groupname.count = strlen(tmpstring) + 1;
            // printf("groupname=%s\n", tmpstring);          
            // printf("len(groupname)=%d\n\n", ret->groupname.count);          

		    xmlFree(tmpstring);
 	    }
		else if ((!xmlStrcmp(cur->name, (const xmlChar *)"MODE"))){
            xmlChar* tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint16_t tmp = strtoul(tmpstring, NULL, 10);
            ret->mode = tmp;
            // printf("mode=%d\n\n", ret->mode);          
            xmlFree(tmpstring);
            // printf("edit_version = %d, %s\n", tmp, tmpstring);     
		}
	cur = cur->next;
	}
    return ret;
}

struct data_t* parseData (xmlDocPtr doc, xmlNodePtr cur) {
	struct data_t* ret = malloc(sizeof(struct data_t));
    xmlChar *tmpstring;
    memset(ret, 0, sizeof(struct data_t));
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"TXID"))) {
            tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint64_t tmp = strtoull(tmpstring, NULL, 10);
            ret->txid = tmp;
            xmlFree(tmpstring);
 	    }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"LENGTH"))) {
            tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint64_t tmp = strtoull(tmpstring, NULL, 10);
            ret->length = malloc(sizeof(uint64_t));
            *(ret->length) = tmp;
            xmlFree(tmpstring);
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"INODEID"))) {
            tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint64_t tmp = strtoull(tmpstring, NULL, 10);
            ret->inodeid = malloc(sizeof(uint64_t));
            *(ret->inodeid) = tmp;
            xmlFree(tmpstring);
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"TIMESTAMP"))) {
            tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint64_t tmp = strtoull(tmpstring, NULL, 10);
            ret->timestamp = malloc(sizeof(uint64_t));
            *(ret->inodeid) = tmp;
            xmlFree(tmpstring);
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"PATH"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->path = malloc(sizeof(struct{int8_t;size_t;}));
            ret->path->elem = malloc(strlen(tmpstring) + 1);        
            memcpy(ret->path->elem, tmpstring, strlen(tmpstring) + 1);
            ret->path->count = strlen(tmpstring) + 1;
		    xmlFree(tmpstring);
 	    }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"SRC"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->src = malloc(sizeof(struct{int8_t;size_t;}));
            ret->src->elem = malloc(strlen(tmpstring) + 1);        
            memcpy(ret->src->elem, tmpstring, strlen(tmpstring) + 1);
            ret->src->count = strlen(tmpstring) + 1;
		    xmlFree(tmpstring);
 	    }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"PERMISSION_STATUS"))) {
            struct permission_status_t* tmp_data;
            tmp_data = parsePermission_status(doc, cur);
            ret->permission_status = tmp_data;     
        }
	cur = cur->next;
	}
    return ret;
}


struct record_t* parseRecord (xmlDocPtr doc, xmlNodePtr cur) {
	struct record_t* ret = malloc(sizeof(struct record_t));
    xmlChar *tmpstring;
    memset(ret, 0, sizeof(struct record_t));
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"OPCODE"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->opcode.elem = malloc(strlen(tmpstring) + 1);         
            memcpy(ret->opcode.elem, tmpstring, strlen(tmpstring) + 1);
            ret->opcode.count = strlen(tmpstring) + 1;
		    xmlFree(tmpstring);
 	    }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"DATA"))) {
            struct data_t * tmp_data;
            tmp_data = parseData(doc, cur);
            ret->data = *tmp_data;     
            // free_data_t(tmp_data);
            // free(tmp_data);
        }
	cur = cur->next;
	}
    return ret;
}

static edits_t* parseEdits(char *docname) {

    struct edits_t* ret;
    ret = malloc(sizeof(struct edits_t));
    memset(ret, 0, sizeof(struct edits_t));

	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);
	
	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return NULL;
	}
	
	cur = xmlDocGetRootElement(doc);
	
	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return NULL;
	}
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "EDITS")) {
		fprintf(stderr,"document of the wrong type, root node != EDITS");
		xmlFreeDoc(doc);
		return NULL;
	}
	
	cur = cur->xmlChildrenNode;
    struct record_t * tmp_record;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"RECORD"))){
			tmp_record = parseRecord(doc, cur);
            // printf("size = %d\n", sizeof(struct record_t) * (ret->record.count + 1));
            ret->record.elem = realloc(ret->record.elem, sizeof(struct record_t) * (ret->record.count + 1));
            if(ret->record.elem == NULL){
                fprintf(stderr,"memory allocation failed - record\n");
            }
            memcpy(ret->record.elem + ret->record.count, tmp_record, sizeof(struct record_t));
            ret->record.count = ret->record.count + 1;
            // free_record_t(tmp_record);
            // free(tmp_record);
		}
		else if ((!xmlStrcmp(cur->name, (const xmlChar *)"EDITS_VERSION"))){
            xmlChar* tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            int16_t tmp = strtol(tmpstring, NULL, 10);
            ret->edits_version = tmp;
            xmlFree(tmpstring);
            // printf("edit_version = %d, %s\n", tmp, tmpstring);     
		}
		 
	cur = cur->next;
	}
	xmlFreeDoc(doc);
	return ret;
}

int main(int argc, char **argv) {

	char *docname;
		
	if (argc <= 1) {
		printf("Usage: %s docname\n", argv[0]);
		return(0);
	}

	docname = argv[1];
	struct edits_t* myedits = parseEdits (docname);


    // free_edits_t(myedits);
    // free(myedits);
	return (1);
}