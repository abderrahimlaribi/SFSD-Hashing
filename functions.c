#include "functions.h"

const int head_size = sizeof(Header);

// Global variables
int K = 5;  // Default number of fragments
int M = 3;  // Default number of buffers (M < K)

// Hash function: h(x) = x mod K
int hash_function(int key) {
    return key % K;
}

// Initialize file header
void initial_head(FILE* file) {
    Header head;
    head.nb_block = 0;
    head.nb_eng = 0;
    head.nb_delete = 0;
    fseek(file, 0, SEEK_SET);
    fwrite(&head, sizeof(Header), 1, file);
}

// Open/create file
void Open(FILE** file, const char* name, char mode) {
    if (mode == 'N' || mode == 'n') {
        *file = fopen(name, "wb+");
        if (*file != NULL) {
            initial_head(*file);
        }
    } else if (mode == 'A' || mode == 'a') {
        *file = fopen(name, "rb+");
        if (*file == NULL) {
            *file = fopen(name, "wb+");
            if (*file != NULL) {
                initial_head(*file);
            }
        }
    }
}

// Read block from file
bool readBlock(FILE* file, int i, Buffer* buff) {
    if (file == NULL || i <= 0) return false;
    if (fseek(file, (i - 1) * sizeof(Buffer) + head_size, SEEK_SET) != 0) return false;
    if (fread(buff, sizeof(Buffer), 1, file) != 1) return false;
    return true;
}

// Write block to file
bool writeBlock(FILE* file, int i, Buffer* buff) {
    if (file == NULL || i <= 0) return false;
    if (fseek(file, (i - 1) * sizeof(Buffer) + head_size, SEEK_SET) != 0) return false;
    if (fwrite(buff, sizeof(Buffer), 1, file) != 1) return false;
    return true;
}

// Get header field
int getHeader(FILE* file, int i) {
    if (file != NULL) {
        Header head;
        fseek(file, 0, SEEK_SET);
        if (fread(&head, sizeof(Header), 1, file) == 1) {
            if (i == 1) return head.nb_block;
            if (i == 2) return head.nb_eng;
            if (i == 3) return head.nb_delete;
        }
    }
    return 0;
}

// Set header field
void setHeader(FILE* file, int NB, int i) {
    if (file != NULL) {
        Header head;
        fseek(file, 0, SEEK_SET);
        if (fread(&head, sizeof(Header), 1, file) == 1) {
            if (i == 1) head.nb_block = NB;
            if (i == 2) head.nb_eng = NB;
            if (i == 3) head.nb_delete = NB;
        }
        fseek(file, 0, SEEK_SET);
        fwrite(&head, sizeof(Header), 1, file);
    }
}

// Create initial TnOF file with n records
void create_initial_file(FILE* file, int n, Buffer* buff) {
    int i = 1, j = 0, key;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      Enter %d keys (integers): ", n);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    for (int k = 0; k < n; k++) {
        printf("\n      Key %d: ", k + 1);
        scanf("%d", &key);
        
        if (j < B) {
            buff->tab[j].key = key;
            buff->tab[j].deleted = false;
            j++;
        } else {
            buff->nbr = j;
            writeBlock(file, i, buff);
            buff->tab[0].key = key;
            buff->tab[0].deleted = false;
            i++;
            j = 1;
        }
    }
    
    buff->nbr = j;
    writeBlock(file, i, buff);
    setHeader(file, i, 1);
    setHeader(file, n, 2);
}

// Display a TnOF file
void display_file(FILE* file, const char* filename) {
    Buffer buff;
    int N = getHeader(file, 1);
    
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
    printf("\n      ===== File: %s =====\n", filename);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    printf("      Total blocks: %d, Total records: %d\n", N, getHeader(file, 2));
    
    for (int i = 1; i <= N; i++) {
        readBlock(file, i, &buff);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
        printf("      Block %d: ", i);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
        for (int j = 0; j < buff.nbr; j++) {
            if (!buff.tab[j].deleted) {
                printf("%d ", buff.tab[j].key);
            } else {
                printf("[DEL] ");
            }
        }
        printf("\n");
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
    printf("      =========================\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Get fragment filename
char* get_fragment_name(int fragment_index) {
    static char filename[50];
    sprintf(filename, "fragment_%d.bin", fragment_index);
    return filename;
}

// Partition file by hashing into K fragments
void partition_file_by_hashing(const char* source_file, int K_param, int M_param) {
    K = K_param;
    M = M_param;
    
    display_partition_header();
    info_message("Source file: initial_file.bin");
    printf("      Number of fragments (K): %d\n", K);
    printf("      Number of buffers (M): %d\n", M);
    
    // Validate M < K
    if (M >= K) {
        error_message("Number of buffers (M) must be < number of fragments (K)");
        operation_complete();
        return;
    }
    
    // Open source file
    FILE* source;
    Open(&source, source_file, 'A');
    if (source == NULL) {
        error_message("Cannot open source file!");
        operation_complete();
        return;
    }
    
    // Initialize all fragment files
    FILE* fragments[K];
    for (int i = 0; i < K; i++) {
        Open(&fragments[i], get_fragment_name(i), 'N');
        if (fragments[i] == NULL) {
            error_message("Cannot create fragment");
            operation_complete();
            return;
        }
    }
    
    // Allocate M buffers for fragments
    Buffer* buffers = (Buffer*)malloc(M * sizeof(Buffer));
    for (int i = 0; i < M; i++) {
        buffers[i].nbr = 0;
    }
    
    // Read source file and distribute records
    int N = getHeader(source, 1);
    Buffer source_buff;
    int total_records = 0;
    
    info_message("Reading and distributing records...");
    
    // Process in batches of M fragments at a time
    for (int batch_start = 0; batch_start < K; batch_start += M) {
        int batch_end = (batch_start + M < K) ? batch_start + M : K;
        int batch_size = batch_end - batch_start;
        
        // Initialize buffers for this batch
        for (int i = 0; i < batch_size; i++) {
            buffers[i].nbr = 0;
        }
        
        // Read all blocks from source file
        for (int block_num = 1; block_num <= N; block_num++) {
            readBlock(source, block_num, &source_buff);
            
            // Process each record in the block
            for (int j = 0; j < source_buff.nbr; j++) {
                if (!source_buff.tab[j].deleted) {
                    int key = source_buff.tab[j].key;
                    int hash_val = hash_function(key);
                    
                    // Check if this hash belongs to current batch
                    if (hash_val >= batch_start && hash_val < batch_end) {
                        int buffer_index = hash_val - batch_start;
                        
                        // Add record to buffer
                        buffers[buffer_index].tab[buffers[buffer_index].nbr] = source_buff.tab[j];
                        buffers[buffer_index].nbr++;
                        
                        // If buffer is full, write to fragment
                        if (buffers[buffer_index].nbr >= B) {
                            int frag_blocks = getHeader(fragments[hash_val], 1);
                            writeBlock(fragments[hash_val], frag_blocks + 1, &buffers[buffer_index]);
                            setHeader(fragments[hash_val], frag_blocks + 1, 1);
                            int frag_records = getHeader(fragments[hash_val], 2);
                            setHeader(fragments[hash_val], frag_records + buffers[buffer_index].nbr, 2);
                            buffers[buffer_index].nbr = 0;
                        }
                        total_records++;
                    }
                }
            }
        }
        
        // Write remaining records in buffers to fragments
        for (int i = 0; i < batch_size; i++) {
            if (buffers[i].nbr > 0) {
                int frag_idx = batch_start + i;
                int frag_blocks = getHeader(fragments[frag_idx], 1);
                writeBlock(fragments[frag_idx], frag_blocks + 1, &buffers[i]);
                setHeader(fragments[frag_idx], frag_blocks + 1, 1);
                int frag_records = getHeader(fragments[frag_idx], 2);
                setHeader(fragments[frag_idx], frag_records + buffers[i].nbr, 2);
            }
        }
    }
    
    // Close all files
    fclose(source);
    for (int i = 0; i < K; i++) {
        fclose(fragments[i]);
    }
    free(buffers);
    
    success_message("Partitioning complete!");
    printf("      Total records distributed: %d\n", total_records);
    operation_complete();
}

// Search in TnOF fragment
void search_TnOF(FILE* file, bool* found, int* j, int* i, int c, Buffer* buff) {
    int N = getHeader(file, 1);
    *i = 1;
    *j = 0;
    *found = false;
    
    while (*i <= N && !(*found)) {
        if (readBlock(file, *i, buff)) {
            *j = 0;
            while (*j < buff->nbr && !(*found)) {
                if (c == buff->tab[*j].key && !buff->tab[*j].deleted) {
                    *found = true;
                    break;
                } else {
                    (*j)++;
                }
            }
        }
        if (!(*found)) {
            (*i)++;
        }
    }
}

// Search for a key in the fragmented structure
void search_in_fragments(int key, int K_param) {
    K = K_param;
    int hash_val = hash_function(key);
    
    display_search_header();
    printf("      Searching for key: %d\n", key);
    printf("      Hash value: h(%d) = %d\n", key, hash_val);
    printf("      Target fragment: fragment_%d.bin\n", hash_val);
    
    FILE* fragment;
    Open(&fragment, get_fragment_name(hash_val), 'A');
    
    if (fragment == NULL) {
        error_message("Cannot open fragment!");
        operation_complete();
        return;
    }
    
    Buffer buff;
    bool found;
    int i, j;
    
    search_TnOF(fragment, &found, &j, &i, key, &buff);
    
    if (found) {
        success_message("Key found!");
        printf("      Fragment: %d, Block: %d, Position: %d\n", hash_val, i, j + 1);
    } else {
        error_message("Key NOT FOUND");
    }
    
    fclose(fragment);
    operation_complete();
}

// Insert into TnOF fragment
void insert_TnOF(int c, FILE* file, Buffer* buff) {
    bool found;
    int i, j;
    Data e;
    e.key = c;
    e.deleted = false;
    
    search_TnOF(file, &found, &j, &i, c, buff);
    
    if (!found) {
        int N = getHeader(file, 1);
        if (N != 0) {
            readBlock(file, N, buff);
        } else {
            N = 1;
            setHeader(file, N, 1);
            buff->nbr = 0;
        }
        
        if (buff->nbr < B) {
            buff->tab[buff->nbr] = e;
            buff->nbr++;
            writeBlock(file, N, buff);
        } else {
            buff->nbr = 1;
            buff->tab[0] = e;
            writeBlock(file, N + 1, buff);
            setHeader(file, N + 1, 1);
        }
        
        int total = getHeader(file, 2);
        setHeader(file, total + 1, 2);
    }
}

// Insert a key into the fragmented structure
void insert_in_fragments(int key, int K_param) {
    K = K_param;
    int hash_val = hash_function(key);
    
    display_insert_header();
    printf("      Inserting key: %d\n", key);
    printf("      Hash value: h(%d) = %d\n", key, hash_val);
    printf("      Target fragment: fragment_%d.bin\n", hash_val);
    
    FILE* fragment;
    Open(&fragment, get_fragment_name(hash_val), 'A');
    
    if (fragment == NULL) {
        error_message("Cannot open fragment!");
        operation_complete();
        return;
    }
    
    Buffer buff;
    bool found;
    int i, j;
    
    // Check if key already exists
    search_TnOF(fragment, &found, &j, &i, key, &buff);
    
    if (found) {
        error_message("Key already exists! Cannot insert duplicate.");
    } else {
        insert_TnOF(key, fragment, &buff);
        success_message("Key inserted successfully!");
        printf("      Fragment: %d\n", hash_val);
    }
    
    fclose(fragment);
    operation_complete();
}

// Physical deletion in TnOF fragment
void physical_deletion_TnOF(FILE* file, int c, Buffer* buff1, Buffer* buff2) {
    bool found;
    int i, j;
    
    search_TnOF(file, &found, &j, &i, c, buff1);
    
    if (found) {
        int N = getHeader(file, 1);
        if (i != N) {
            // Record not in last block
            readBlock(file, i, buff1);
            readBlock(file, N, buff2);
            buff1->tab[j] = buff2->tab[buff2->nbr - 1];
            buff2->nbr--;
            writeBlock(file, i, buff1);
            
            if (buff2->nbr > 0) {
                writeBlock(file, N, buff2);
            } else {
                setHeader(file, N - 1, 1);
            }
        } else {
            // Record in last block
            readBlock(file, N, buff1);
            buff1->tab[j] = buff1->tab[buff1->nbr - 1];
            buff1->nbr--;
            
            if (buff1->nbr > 0) {
                writeBlock(file, N, buff1);
            } else {
                setHeader(file, N - 1, 1);
            }
        }
        
        int total = getHeader(file, 2);
        setHeader(file, total - 1, 2);
    }
}

// Delete a key from the fragmented structure
void delete_in_fragments(int key, int K_param) {
    K = K_param;
    int hash_val = hash_function(key);
    
    display_delete_header();
    printf("      Deleting key: %d\n", key);
    printf("      Hash value: h(%d) = %d\n", key, hash_val);
    printf("      Target fragment: fragment_%d.bin\n", hash_val);
    
    FILE* fragment;
    Open(&fragment, get_fragment_name(hash_val), 'A');
    
    if (fragment == NULL) {
        error_message("Cannot open fragment!");
        operation_complete();
        return;
    }
    
    Buffer buff1, buff2;
    bool found;
    int i, j;
    
    // Check if key exists
    search_TnOF(fragment, &found, &j, &i, key, &buff1);
    
    if (!found) {
        error_message("Key not found! Cannot delete.");
    } else {
        physical_deletion_TnOF(fragment, key, &buff1, &buff2);
        success_message("Key deleted successfully!");
        printf("      Fragment: %d\n", hash_val);
    }
    
    fclose(fragment);
    operation_complete();
}

// Display all fragments
void display_all_fragments(int K_param) {
    K = K_param;
    display_fragments_header();
    
    for (int i = 0; i < K; i++) {
        FILE* fragment;
        Open(&fragment, get_fragment_name(i), 'A');
        
        if (fragment != NULL) {
            display_file(fragment, get_fragment_name(i));
            fclose(fragment);
        }
    }
    
    operation_complete();
}

// Wrapper for close if needed
void Close(FILE* file) {
    if (file != NULL) {
        fclose(file);
    }
}

// Allocate a new block 
int allocBlock(FILE* file) {
    int N = getHeader(file, 1);
    setHeader(file, N + 1, 1);
    return N + 1;
}

// Clear screen function
void clear_screen() {
    system("cls||clear");
}

// Welcome screen
void welcome() {
       system("cls");
       printf("\n\n");
       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);
   
       // Top border
       printf("     %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
              201,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,203,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              187);
   
       // Content
       printf("     %c     FILE STRUCTURES AND DATA STRUCTURES (FSDS)     %c     ESI - 2025/2026       %c\n", 186, 186, 186);
       printf("     %c     ==========================================     %c    LARIBI ABDERRAHIM      %c\n", 186, 186, 186);
       printf("     %c       PRACTICAL WORK: FILE PARTITIONING BY         %c    DORGHAM ABDELILLAH     %c\n", 186, 186, 186);
       printf("     %c              HASHING METHODS                       %c    GROUP: 11              %c\n", 186, 186, 186);
   
       // Middle separator
       printf("     %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
              204,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,206,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              185);
   
       // Footer content
       printf("     %c   HIGHER SCHOOL OF COMPUTER SCIENCE - ALGIERS      %c                           %c\n", 186, 186, 186);
   
       // Bottom border
       printf("     %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
              200,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,202,205,205,205,205,205,205,205,
              205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
              188);
   
       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
       printf("\n        THANKS FOR CHOOSING OUR PROGRAM!\n");
       printf("        Press ENTER to continue...");
       getchar();
   }
   
// Main interface
void main_interface() {
    system("cls");
    printf("\n\n\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);
    printf("     %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%C\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("     %c   This program demonstrates file partitioning using hashing       %c\n", 186, 186);
    printf("     %c   methods. You will be able to:                                   %c\n", 186, 186);
    printf("     %c   - Create an initial TnOF file with records                      %c\n", 186, 186);
    printf("     %c   - Partition the file into K fragments using h(x) = x mod K      %c\n", 186, 186);
    printf("     %c   - Perform search, insert, and delete operations                 %c\n", 186, 186);
    printf("     %c   - Display fragments and analyze the structure                   %c\n", 186, 186);
    printf("     %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%C%C\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    printf("\n        What do you want to do?\n");
    printf("            -----------> 1: Continue to the program\n");
    printf("            -----------> 2: Quit the program\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);
    printf("\n        Enter your choice: ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Main menu
void main_menu() {
       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);
   
       // Top border
       printf("      %c", 201);
       for (int i = 0; i < 87; i++) printf("%c", 205);
       printf("%c\n", 187);
   
       // Title
       printf("      %c    FILE PARTITIONING BY HASHING - MAIN MENU                                           %c\n", 186, 186);
   
       // Separator
       printf("      %c", 204);
       for (int i = 0; i < 87; i++) printf("%c", 205);
       printf("%c\n", 185);
   
       // Empty line
       printf("      %c                                                                                       %c\n", 186, 186);
   
       // Section titles
       printf("      %c    FILE CREATION & PARTITIONING                    OPERATIONS ON FRAGMENTS            %c\n", 186, 186);
   
       // Inner boxes top
       printf("      %c    %c", 186, 201);
       for (int i = 0; i < 32; i++) printf("%c", 205);
       printf("%c            %c", 187, 201);
       for (int i = 0; i < 32; i++) printf("%c", 205);
       printf("%c   %c\n", 187, 186);
   
       // Row 1
       printf("      %c    %c 1. Create initial TnOF file    %c            %c 5. Search for a key            %c   %c\n",
              186, 186, 186, 186, 186, 186);
   
       // Row 2
       printf("      %c    %c 2. Display initial file        %c            %c 6. Insert a key                %c   %c\n",
              186, 186, 186, 186, 186, 186);
   
       // Row 3
       printf("      %c    %c 3. Partition file by           %c            %c 7. Delete a key                %c   %c\n",
              186, 186, 186, 186, 186, 186);
   
       // Row 4
       printf("      %c    %c    hashing into K fragments    %c            %c 8. Display all fragments       %c   %c\n",
              186, 186, 186, 186, 186, 186);
   
       // Row 5
       printf("      %c    %c 4. Display initial file        %c            %c                                %c   %c\n",
              186, 186, 186, 186, 186, 186);
   
       // Inner boxes bottom
       printf("      %c    %c", 186, 200);
       for (int i = 0; i < 32; i++) printf("%c", 205);
       printf("%c            %c", 188, 200);
       for (int i = 0; i < 32; i++) printf("%c", 205);
       printf("%c   %c\n", 188, 186);
   
       // Empty line
       printf("      %c                                                                                       %c\n", 186, 186);
   
       // Exit
       printf("      %c                              0. EXIT PROGRAM                                          %c\n", 186, 186);
   
       // Bottom border
       printf("      %c", 200);
       for (int i = 0; i < 87; i++) printf("%c", 205);
       printf("%c\n", 188);
   
       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
   }
   
   

// Success message
void success_message(const char* message) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    printf("\n      %c SUCCESS: %s\n", 251, message);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Error message
void error_message(const char* message) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
    printf("\n      %c ERROR: %s\n", 88, message);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Info message
void info_message(const char* message) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
    printf("      %c INFO: %s\n", 187, message);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Enter number prompt
void enter_number_prompt() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                  Enter the number of records to create:                         %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    printf("      >> ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Enter K prompt
void enter_K_prompt() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c              Enter the number of fragments (K):                                 %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    printf("      >> ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Enter M prompt
void enter_M_prompt() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c        Enter the number of buffers in memory (M < K):                           %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    printf("      >> ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Enter key prompt
void enter_key_prompt() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      Enter the key value: ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Exiting program
void exiting_program() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                                                                                 %c\n", 186, 186);
    printf("      %c                     THANK YOU FOR USING OUR PROGRAM!                            %c\n", 186, 186);
    printf("      %c                            Exiting the program...                               %c\n", 186, 186);
    printf("      %c                                                                                 %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    MessageBox(NULL, "Thank you for using our program!", "EXIT", MB_OK | MB_ICONINFORMATION);
}

// Invalid choice
void invalid_choice() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                    INVALID CHOICE! Please try again.                          %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    MessageBox(NULL, "Invalid choice! Try again.", "INVALID", MB_OK | MB_ICONWARNING);
}

// Operation complete
void operation_complete() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    printf("\n      Press ENTER to continue...");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    getchar();
    getchar();
}

// Operation menu
void operation_menu() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      Enter your choice: ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Partition menu header
void display_partition_header() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                    PARTITIONING FILE BY HASHING                                 %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Search operation header
void display_search_header() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                         SEARCH OPERATION                                        %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Insert operation header
void display_insert_header() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                         INSERT OPERATION                                        %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Delete operation header
void display_delete_header() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                         DELETE OPERATION                                        %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

// Display fragments header
void display_fragments_header() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13);
    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
    printf("      %c                      DISPLAYING ALL FRAGMENTS                                   %c\n", 186, 186);
    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}