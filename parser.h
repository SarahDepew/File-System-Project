//
// Created by Sarah Depew on 5/7/18.
//

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

enum{START, MIDDLE, END};

#define MAX_BUFFER 4096

int perform_parse();
void free_all_jobs();

#endif
