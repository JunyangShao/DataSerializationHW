#include <editlog.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void debug_edits_t(struct edits_t* input){
    printf("edits_versions = %d\n", input->edits_version);
    for(int i = 0; i< input->record.count; i++){
        printf("entering record ---\n");
        debug_record_t(input->record.elem + i);
        printf("exiting  record ---\n");
    }
    return;
}

void debug_record_t(struct record_t* input){
    printf("opcode=%s\n", input->opcode.elem);
    printf("entering data --------------\n");
    debug_data_t(&(input->data));
    printf("exiting  data --------------\n");
    return;
}
void debug_data_t(struct data_t* input){
    printf("txid=%llu\n", input->txid);
    if(input->length) printf("length=%llu\n", *(input->length));
    if(input->inodeid) printf("inodeid=%llu\n", *(input->inodeid));
    if(input->datamode) printf("datamode=%llu\n", *(input->datamode));
    if(input->path) printf("path=%s\n", input->path->elem);
    if(input->src) printf("src=%s\n", input->src->elem);
    if(input->timestamp) printf("timestamp=%llu\n", *(input->timestamp));
    if(input->permission_status) {
        printf("entering pstat --------------\n");
        debug_permission_status_t(input->permission_status);
        printf("exiting  pstat --------------\n");
    }
}
void debug_permission_status_t(struct permission_status_t* input){
    printf("username=%s\n", input->username.elem);
    printf("groupname=%s\n", input->groupname.elem);
    printf("mode=%d\n", input->mode);
    return;
}

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
            ret->username.count = strlen(tmpstring);
            // printf("username=%s\n", ret->username.elem);          
            // printf("len(username)=%d\n\n", ret->username.count);          
		    xmlFree(tmpstring);
 	    }
	    else if ((!xmlStrcmp(cur->name, (const xmlChar *)"GROUPNAME"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->groupname.elem = malloc(strlen(tmpstring) + 1);        
            memcpy(ret->groupname.elem, tmpstring, strlen(tmpstring) + 1);
            ret->groupname.count = strlen(tmpstring);
            // printf("groupname=%s\n", tmpstring);          
            // printf("len(groupname)=%d\n\n", ret->groupname.count);          

		    xmlFree(tmpstring);
 	    }
		else if ((!xmlStrcmp(cur->name, (const xmlChar *)"MODE"))){
            xmlChar* tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint16_t tmp = strtol(tmpstring, NULL, 10);
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
            *(ret->timestamp) = tmp;
            xmlFree(tmpstring);
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"MODE"))) {
            tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            uint16_t tmp = strtoull(tmpstring, NULL, 10);
            ret->datamode = malloc(sizeof(uint16_t));
            *(ret->datamode) = tmp;
            xmlFree(tmpstring);
        }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"PATH"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->path = malloc(sizeof(struct{int8_t;size_t;}));
            ret->path->elem = malloc(strlen(tmpstring) + 1);        
            memcpy(ret->path->elem, tmpstring, strlen(tmpstring) + 1);
            ret->path->count = strlen(tmpstring);
		    xmlFree(tmpstring);
 	    }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"SRC"))) {
		    tmpstring = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            ret->src = malloc(sizeof(struct{int8_t;size_t;}));
            ret->src->elem = malloc(strlen(tmpstring) + 1);        
            memcpy(ret->src->elem, tmpstring, strlen(tmpstring) + 1);
            ret->src->count = strlen(tmpstring);
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
            ret->opcode.count = strlen(tmpstring);
		    xmlFree(tmpstring);
 	    }
        else if ((!xmlStrcmp(cur->name, (const xmlChar *)"DATA"))) {
            struct data_t * tmp_data;
            tmp_data = parseData(doc, cur);
            ret->data = *tmp_data;     
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
		}
		else if ((!xmlStrcmp(cur->name, (const xmlChar *)"EDITS_VERSION"))){
            xmlChar* tmpstring = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            int16_t tmp = strtol(tmpstring, NULL, 10);
            ret->edits_version = tmp;
            // printf("edit_version = %d\n", ret->edits_version);     
            xmlFree(tmpstring);
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

    // ** reading XML
	docname = argv[1];
	struct edits_t* myedits = parseEdits (docname);
    // debug_edits_t(myedits);

    // ** generating
    NailArena arena;
    NailOutStream out;
    jmp_buf err;

    NailArena_init(&arena, 4096, &err);
    NailOutStream_init(&out,4096);

    size_t b_myedits_size;
    printf("gen_edits_t=%d\n", gen_edits_t(&arena, &out, myedits));
    const char* b_myedits = NailOutStream_buffer(&out, &b_myedits_size);
    printf("size = %d, b_myedits= %llu\n", b_myedits_size, (uint64_t)(b_myedits));

    // ** writing to file
    FILE * myfile;
    myfile = fopen("./myfile","w");
    printf("fwrite=%d\n", fwrite(b_myedits, 1, b_myedits_size, myfile));
    fclose(myfile);

    // ** reading from file
    char* b_myedits2 = malloc(b_myedits_size + 1);

    FILE * myfile2;
    myfile2 = fopen("./myfile","r");
    printf("fread=%d\n", fread(b_myedits2, 1, b_myedits_size, myfile));
    fclose(myfile2); 

    // ** parsing
    NailArena arena2;
    jmp_buf err2;

    NailArena_init(&arena2, 4096, &err2);
    struct edits_t * myedits2 = parse_edits_t(&arena2, b_myedits2, b_myedits_size);
    printf("parse_edits_t=%llu\n",(uint64_t)(myedits2));

	return (1);
}