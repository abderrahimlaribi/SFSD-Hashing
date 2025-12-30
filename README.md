# File Partitioning by Hashing - Practical Work

## Project Structure

```
PW1_LARIBI_DORGHAM_model/
├── main.c           # Main program with menu
├── functions.h      # Function declarations
├── functions.c      # Core implementation and Display implementations with Windows styling
└── README.md        # This file
```

## Compilation Instructions

### Using GCC (MinGW on Windows):
```bash
gcc -o program main.c functions.c -luser32
```

The `-luser32` flag is needed for the `MessageBox` function.

### Using Code::Blocks or Dev-C++:
1. Create a new project
2. Add all `.c` files to the project
3. Add all `.h` files to the project
4. Build and run

## How to Use the Program

### Step 1: Create Initial File
- Choose option 1 from the main menu
- Enter the number of records you want to create
- Enter the integer keys one by one

### Step 2: Partition the File
- Choose option 3 from the main menu
- Enter K (number of fragments, e.g., 5)
- Enter M (number of buffers, must be < K, e.g., 3)
- The program will partition your file into K fragments using h(x) = x mod K

### Step 3: Perform Operations
After partitioning, you can:
- **Option 5**: Search for a key (finds it in the correct fragment)
- **Option 6**: Insert a new key (inserts into the correct fragment)
- **Option 7**: Delete a key (removes from the correct fragment)
- **Option 8**: Display all fragments to see the distribution

## Algorithm Explanation

### Partitioning Algorithm:
1. **Input**: Initial TnOF file with N blocks, K fragments, M buffers
2. **Constraint**: M < K (fewer buffers than fragments)
3. **Process**:
   - Process fragments in batches of M
   - For each batch:
     - Read entire source file
     - Distribute records to M buffers based on hash
     - Write full buffers to disk
   - Repeat until all K fragments processed
4. **Complexity**: O(N × ⌈K/M⌉) block reads

### Hash Function:
```
h(key) = key mod K
```
Where K is the number of fragments (0 to K-1)

### Operations After Partitioning:
- **Search**: O(1) to find fragment + O(n) to search within fragment
- **Insert**: O(1) to find fragment + O(1) to insert at end
- **Delete**: O(1) to find fragment + O(n) physical deletion

## Features

✅ TnOF (Unordered Array File) structure
✅ Efficient partitioning with M < K constraint
✅ Hash-based distribution: h(x) = x mod K
✅ All three operations: Search, Insert, Delete
✅ Windows-style colored display with box-drawing
✅ Error handling and validation
✅ Complete abstract machine implementation

## Display Features

- **Colored Output**:
  - Blue/Cyan (3): Headers and menus
  - Green (10): Success messages
  - Red (12): Error messages
  - Yellow (14): Prompts
  - Magenta (13): Fragment displays

- **Box-Drawing Characters**: Uses ASCII extended characters (201, 205, 186, etc.) for professional appearance

- **MessageBox Alerts**: Windows popup messages for important events

## Notes

- The program creates fragment files named `fragment_0.bin`, `fragment_1.bin`, etc.
- All files are binary TnOF files with headers
- Block capacity (B) is set to 6 records by default
- You can change B in `functions.h` if needed

## Example Run

```
1. Create file with 20 records
2. Partition into K=5 fragments with M=3 buffers
3. Records distributed: h(10)=0, h(11)=1, h(12)=2, etc.
4. Search for key 15 → Fragment 0 (15 mod 5 = 0)
5. Insert key 23 → Fragment 3 (23 mod 5 = 3)
6. Delete key 10 → Fragment 0 (10 mod 5 = 0)
```

## Author

- **Laribi Abderrahim - Dorgham Abdelillah**
- **Group**: 11
- **Date**: December 2025
- **Institution**: ESI (Higher School of Computer Science)
