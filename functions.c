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
void initialize_file(FILE** file, const char* name, char mode) {
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
bool read_block(FILE* file, int i, Buffer* buff) {
    if (file == NULL || i <= 0) return false;
    if (fseek(file, (i - 1) * sizeof(Buffer) + head_size, SEEK_SET) != 0) return false;
    if (fread(buff, sizeof(Buffer), 1, file) != 1) return false;
    return true;
}

// Write block to file
bool write_block(FILE* file, int i, Buffer* buff) {
    if (file == NULL || i <= 0) return false;
    if (fseek(file, (i - 1) * sizeof(Buffer) + head_size, SEEK_SET) != 0) return false;
    if (fwrite(buff, sizeof(Buffer), 1, file) != 1) return false;
    return true;
}

// Get header field
int get_head(FILE* file, int i) {
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
void set_head(FILE* file, int NB, int i) {
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
            write_block(file, i, buff);
            buff->tab[0].key = key;
            buff->tab[0].deleted = false;
            i++;
            j = 1;
        }
    }
    
    buff->nbr = j;
    write_block(file, i, buff);
    set_head(file, i, 1);
    set_head(file, n, 2);
}

// Display a TnOF file
void display_file(FILE* file, const char* filename) {
    Buffer buff;
    int N = get_head(file, 1);
    
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
    printf("\n      ===== File: %s =====\n", filename);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    printf("      Total blocks: %d, Total records: %d\n", N, get_head(file, 2));
    
    for (int i = 1; i <= N; i++) {
        read_block(file, i, &buff);
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
    initialize_file(&source, source_file, 'A');
    if (source == NULL) {
        error_message("Cannot open source file!");
        operation_complete();
        return;
    }
    
    // Initialize all fragment files
    FILE* fragments[K];
    for (int i = 0; i < K; i++) {
        initialize_file(&fragments[i], get_fragment_name(i), 'N');
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
    int N = get_head(source, 1);
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
            read_block(source, block_num, &source_buff);
            
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
                            int frag_blocks = get_head(fragments[hash_val], 1);
                            write_block(fragments[hash_val], frag_blocks + 1, &buffers[buffer_index]);
                            set_head(fragments[hash_val], frag_blocks + 1, 1);
                            int frag_records = get_head(fragments[hash_val], 2);
                            set_head(fragments[hash_val], frag_records + buffers[buffer_index].nbr, 2);
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
                int frag_blocks = get_head(fragments[frag_idx], 1);
                write_block(fragments[frag_idx], frag_blocks + 1, &buffers[i]);
                set_head(fragments[frag_idx], frag_blocks + 1, 1);
                int frag_records = get_head(fragments[frag_idx], 2);
                set_head(fragments[frag_idx], frag_records + buffers[i].nbr, 2);
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
    int N = get_head(file, 1);
    *i = 1;
    *j = 0;
    *found = false;
    
    while (*i <= N && !(*found)) {
        if (read_block(file, *i, buff)) {
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
    initialize_file(&fragment, get_fragment_name(hash_val), 'A');
    
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
        int N = get_head(file, 1);
        if (N != 0) {
            read_block(file, N, buff);
        } else {
            N = 1;
            set_head(file, N, 1);
            buff->nbr = 0;
        }
        
        if (buff->nbr < B) {
            buff->tab[buff->nbr] = e;
            buff->nbr++;
            write_block(file, N, buff);
        } else {
            buff->nbr = 1;
            buff->tab[0] = e;
            write_block(file, N + 1, buff);
            set_head(file, N + 1, 1);
        }
        
        int total = get_head(file, 2);
        set_head(file, total + 1, 2);
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
    initialize_file(&fragment, get_fragment_name(hash_val), 'A');
    
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
        int N = get_head(file, 1);
        if (i != N) {
            // Record not in last block
            read_block(file, i, buff1);
            read_block(file, N, buff2);
            buff1->tab[j] = buff2->tab[buff2->nbr - 1];
            buff2->nbr--;
            write_block(file, i, buff1);
            
            if (buff2->nbr > 0) {
                write_block(file, N, buff2);
            } else {
                set_head(file, N - 1, 1);
            }
        } else {
            // Record in last block
            read_block(file, N, buff1);
            buff1->tab[j] = buff1->tab[buff1->nbr - 1];
            buff1->nbr--;
            
            if (buff1->nbr > 0) {
                write_block(file, N, buff1);
            } else {
                set_head(file, N - 1, 1);
            }
        }
        
        int total = get_head(file, 2);
        set_head(file, total - 1, 2);
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
    initialize_file(&fragment, get_fragment_name(hash_val), 'A');
    
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
        initialize_file(&fragment, get_fragment_name(i), 'A');
        
        if (fragment != NULL) {
            display_file(fragment, get_fragment_name(i));
            fclose(fragment);
        }
    }
    
    operation_complete();
}