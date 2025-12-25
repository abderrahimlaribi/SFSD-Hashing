#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// TTHIS IS 
// Constants
#define B 6  // Block capacity (max records per block)

// Data structure for a record
typedef struct {
    int key;
    bool deleted;
} Data;

// Block structure
typedef struct {
    Data tab[B];
    int nbr;  // Number of records in block
} Tblock, Buffer;

// File header structure
typedef struct {
    int nb_block;   // Number of blocks
    int nb_eng;     // Number of records
    int nb_delete;  // Number of deleted records
} Header;

// Global constants for hashing
extern int K;  // Number of fragments
extern int M;  // Number of buffers in memory

// Function declarations for basic file operations
void Open(FILE** file, const char* name, char mode);
void initial_head(FILE* file);
bool readBlock(FILE* file, int i, Buffer* buff);
bool writeBlock(FILE* file, int i, Buffer* buff);
int getHeader(FILE* file, int i);
void setHeader(FILE* file, int NB, int i);

// Hash function
int hash_function(int key);

// Initial file creation and display
void create_initial_file(FILE* file, int n, Buffer* buff);
void display_file(FILE* file, const char* filename);

// Partitioning functions
void partition_file_by_hashing(const char* source_file, int K, int M);
char* get_fragment_name(int fragment_index);

// Operations on fragmented structure
void search_in_fragments(int key, int K);
void insert_in_fragments(int key, int K);
void delete_in_fragments(int key, int K);

// Helper functions for TnOF operations on individual fragments
void search_TnOF(FILE* file, bool* found, int* j, int* i, int c, Buffer* buff);
void insert_TnOF(int c, FILE* file, Buffer* buff);
void physical_deletion_TnOF(FILE* file, int c, Buffer* buff1, Buffer* buff2);

// Display all fragments
void display_all_fragments(int K);


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