/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

/* Constructor for the file handle. Set the ’current position’ to be at the 
       beginning of the file. */
File::File(FileSystem *_fs_pointer, int _id) {
    Console::puts("Opening file.\n");
    
    curr_position = 0;
    fs_pointer = _fs_pointer;
    f_id = _id;

    unsigned int i = 0;
    bool found_block = false;
    unsigned int max_inodes = fs_pointer->MAX_INODES;

    for (i = 0;i < max_inodes ;i < i++){
        
        if (fs_pointer->inodes[i].id == _id){
            inode_index = i;
            
            block_no = fs_pointer->inodes[inode_index].block_no;
            file_size = fs_pointer->inodes[inode_index].file_size;
            
            found_block = true;
        }
    }

    if(!found_block){
        Console::puts("Block to open file not found\n");
        assert(false);
    }
    Console::puts("File opened successfully.\n");
    return;
}

/* Closes the file. Deletes any data structures associated with the file handle. */
File::~File() {
    Console::puts("Closing file.\n");

    fs_pointer->disk->write(block_no, block_cache);
    fs_pointer->inodes[inode_index].file_size = file_size;
    
    Inode * tmp_inodes = fs_pointer->inodes;
    unsigned char * tmp = (unsigned char *)tmp_inodes;
    fs_pointer->disk->write(1,tmp);
    
    Console::puts("Closed file successfullfy\n");
    return;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/
/* Read _n characters from the file starting at the current position and
       copy them in _buf.  Return the number of characters read. 
       Do not read beyond the end of the file. */
int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    
    int char_count = 0;
    unsigned int i = 0;

    for(i = curr_position;i < file_size;i++){
        if (char_count == _n){
            break;
        }
        _buf[char_count] = block_cache[i];

        char_count++;
    }

    Console::puts("read from file successfully with char count of:");Console::puti(char_count);Console::puts("\n");
    return char_count;
}

/* Write _n characters to the file starting at the current position. If the write
       extends over the end of the file, extend the length of the file until all data is 
       written or until the maximum file size is reached. Do not write beyond the maximum
       length of the file.  
       Return the number of characters written. */
int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");

    int start_pos = curr_position;
    int char_count = 0;

    unsigned int i = 0;

    for (i = start_pos;i < start_pos+_n;i++){
        
        if (i == SimpleDisk::BLOCK_SIZE){
            break;
        }

        block_cache[curr_position] = _buf[char_count];
        
        file_size++;curr_position++;char_count++;

    }
    Console::puts("written to file successfully with char count of:");Console::puti(char_count);Console::puts("\n");
    return char_count;
}

/* Set the ’current position’ to the beginning of the file. */
void File::Reset() {
    Console::puts("resetting file\n");
    
    curr_position = 0;
    
    Console::puts("Reset the file current postion back to 0.\n");
}

/* Is the current position for the file at the end of the file? */
bool File::EoF() {
    Console::puts("checking for EoF\n");
    
    if(curr_position < file_size){
        Console::puts("Not EoF\n");
        return false;
    }else{
        Console::puts("Reached EoF\n");
        return true;
    }
}
