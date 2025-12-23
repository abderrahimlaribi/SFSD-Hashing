#include "functions.h"

int main() {
     SetConsoleOutputCP(437);     // Force CP437
    SetConsoleCP(437);
    system("cls");

    int choice;
    int n, key, K_val, M_val;
    FILE* initial_file = NULL;
    Buffer buff;
    char initial_filename[] = "initial_file.bin";
    bool file_created = false;
    bool file_partitioned = false;
    int current_K = 5;  // Default K value
    int continueChoice;
    
    // Display welcome screen
    welcome();
    
    // Main interface loop
    while (1) {
        main_interface();
        scanf("%d", &continueChoice);
        
        if (continueChoice == 2) {
            exiting_program();
            return 0;
        } else if (continueChoice != 1) {
            invalid_choice();
            clear_screen();
            continue;
        }
        
        // Main program loop
        while (1) {
            clear_screen();
            main_menu();
            scanf("%d", &choice);
            clear_screen();
            
            switch (choice) {
                case 1:
                    // Create initial TnOF file
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
                    printf("\n      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                           201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
                           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187);
                    printf("      %c                      CREATE INITIAL TnOF FILE                                   %c\n", 186, 186);
                    printf("      %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                           200,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
                           205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,188);
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
                    
                    enter_number_prompt();
                    scanf("%d", &n);
                    
                    if (n <= 0) {
                        error_message("Number of records must be positive!");
                        operation_complete();
                        break;
                    }
                    
                    Open(&initial_file, initial_filename, 'N');
                    if (initial_file == NULL) {
                        error_message("Cannot create file!");
                        operation_complete();
                        break;
                    }
                    
                    create_initial_file(initial_file, n, &buff);
                    fclose(initial_file);
                    file_created = true;
                    file_partitioned = false;
                    
                    success_message("Initial file created successfully!");
                    printf("      File: %s, Records: %d\n", initial_filename, n);
                    operation_complete();
                    break;
                    
                case 2:
                    // Display initial file
                    if (!file_created) {
                        error_message("No initial file exists! Please create one first (option 1).");
                        operation_complete();
                        break;
                    }
                    
                    Open(&initial_file, initial_filename, 'A');
                    if (initial_file == NULL) {
                        error_message("Cannot open file!");
                        operation_complete();
                        break;
                    }
                    
                    display_file(initial_file, initial_filename);
                    fclose(initial_file);
                    operation_complete();
                    break;
                    
                case 3:
                    // Partition file by hashing
                    if (!file_created) {
                        error_message("No initial file exists! Please create one first (option 1).");
                        operation_complete();
                        break;
                    }
                    
                    enter_K_prompt();
                    scanf("%d", &K_val);
                    
                    if (K_val <= 0) {
                        error_message("K must be positive!");
                        operation_complete();
                        break;
                    }
                    
                    enter_M_prompt();
                    scanf("%d", &M_val);
                    
                    if (M_val <= 0) {
                        error_message("M must be positive!");
                        operation_complete();
                        break;
                    }
                    
                    if (M_val >= K_val) {
                        error_message("M must be strictly less than K!");
                        operation_complete();
                        break;
                    }
                    
                    partition_file_by_hashing(initial_filename, K_val, M_val);
                    file_partitioned = true;
                    current_K = K_val;
                    break;
                    
                case 4:
                    // Display initial file (duplicate of case 2 for menu consistency)
                    if (!file_created) {
                        error_message("No initial file exists! Please create one first (option 1).");
                        operation_complete();
                        break;
                    }
                    
                    Open(&initial_file, initial_filename, 'A');
                    if (initial_file == NULL) {
                        error_message("Cannot open file!");
                        operation_complete();
                        break;
                    }
                    
                    display_file(initial_file, initial_filename);
                    fclose(initial_file);
                    operation_complete();
                    break;
                    
                case 5:
                    // Search for a key
                    if (!file_partitioned) {
                        error_message("File not partitioned yet! Please partition first (option 3).");
                        operation_complete();
                        break;
                    }
                    
                    enter_key_prompt();
                    scanf("%d", &key);
                    search_in_fragments(key, current_K);
                    break;
                    
                case 6:
                    // Insert a key
                    if (!file_partitioned) {
                        error_message("File not partitioned yet! Please partition first (option 3).");
                        operation_complete();
                        break;
                    }
                    
                    enter_key_prompt();
                    scanf("%d", &key);
                    insert_in_fragments(key, current_K);
                    break;
                    
                case 7:
                    // Delete a key
                    if (!file_partitioned) {
                        error_message("File not partitioned yet! Please partition first (option 3).");
                        operation_complete();
                        break;
                    }
                    
                    enter_key_prompt();
                    scanf("%d", &key);
                    delete_in_fragments(key, current_K);
                    break;
                    
                case 8:
                    // Display all fragments
                    if (!file_partitioned) {
                        error_message("File not partitioned yet! Please partition first (option 3).");
                        operation_complete();
                        break;
                    }
                    
                    display_all_fragments(current_K);
                    break;
                    
                case 0:
                    // Exit
                    exiting_program();
                    return 0;
                    
                default:
                    invalid_choice();
                    break;
            }
        }
    }
    
    return 0;
}