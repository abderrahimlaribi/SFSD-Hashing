#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <windows.h>

// Screen management
void clear_screen();

// Welcome and main interface
void welcome();
void main_interface();

// Menu displays
void main_menu();
void partition_menu();
void operation_menu();

// Operation displays
void display_partition_header();
void display_search_header();
void display_insert_header();
void display_delete_header();
void display_fragments_header();

// Result messages
void success_message(const char* message);
void error_message(const char* message);
void info_message(const char* message);

// Input prompts
void enter_number_prompt();
void enter_K_prompt();
void enter_M_prompt();
void enter_key_prompt();

// Status displays
void exiting_program();
void invalid_choice();
void operation_complete();

#endif